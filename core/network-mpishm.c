#include <ross.h>
#include <mpi.h>

MPI_Comm MPI_COMM_ROSS = MPI_COMM_WORLD;
int custom_communicator = 0;

static long id_tmp;

struct act_q
{
  const char	 *name;

  tw_event	**event_list;
  MPI_Request	 *req_list;
  int		 *idx_list;
  MPI_Status	 *status_list;

  unsigned int	  cur;
};

#define EVENT_TAG 1

#define EVENT_SIZE(e) g_tw_event_msg_sz

static struct act_q posted_sends;
static struct act_q posted_recvs;
static tw_eventq outq;

static unsigned int read_buffer = 50000;
static unsigned int send_buffer = 50000;
static int world_size = 1;

static const tw_optdef mpi_opts[] = {
  TWOPT_GROUP("ROSS MPI Kernel"),
  TWOPT_UINT("read-buffer",
	     read_buffer,
	     "Network read buffer size in number of events"),
  TWOPT_UINT("send-buffer",
	     send_buffer,
	     "Network send buffer size in number of events"),
  TWOPT_UINT( "enableshmem",
	      g_tw_shm_enabled,
	      "Enable use of shared memory event pools between ranks on same node"),
  TWOPT_UINT( "sharedmem",
	      g_tw_shm_events_per_pe,
	      "Number of shared memory events per PE/MPI rank"),
  TWOPT_UINT( "rankspernode",
		g_tw_ranks_per_node,
		"Number of MPI ranks per compute node"),
  TWOPT_END()
};

// Variables needed for shared memory pool init - global to only this module.
#define NETWORK_MPISHM_MAX_CPUS 64

MPI_Comm network_mpishm_comm = MPI_COMM_WORLD;
int network_mpishm_comm_size=0;
MPI_Comm network_mpishm_shmcomm = MPI_COMM_WORLD;    /* shm communicator  */ 
int network_mpishm_shmcomm_size = 0;                 /* shmcomm size */
unsigned int network_mpishm_ranks_per_node = 1;
int network_mpishm_color = 0;
int network_mpishm_group = 0;
int network_mpishm_group_counter = 0;
int network_mpishm_key = 0;
int network_mpishm_rank = 0 ;

unsigned int network_mpishm_num_cpus_per_node = 1;
cpu_set_t network_mpishm_cpu_set[NETWORK_MPISHM_MAX_CPUS];
int network_mpishm_my_cpu = -1;

void *network_mpishm_start_shared_memory_pool_address=(void *)((unsigned long long)0x00007fffff000000 - 
							       (unsigned long long) (1<<24));
void *network_mpishm_shared_memory_pool = NULL;      /* shm memory to be allocated on each node */

key_t network_mpishm_shared_memory_key = 0;      
unsigned long long network_mpishm_shared_memory_size = (size_t)(1<<24);       /* 16 MB; */
unsigned long long network_mpishm_event_length = 512; // default size
unsigned long long network_mpishm_align;
int network_mpishm_shared_memory_create_flag = (IPC_CREAT | 0600);
int network_mpishm_shared_memory_locate_flag = 0600;
int network_mpishm_shared_memory_id = 0;

char network_mpishm_ipcrm_command_string[80];

void tw_comm_set(MPI_Comm comm)
{
	MPI_COMM_ROSS = comm;
	network_mpishm_comm = comm;
	network_mpishm_shmcomm = comm;
	custom_communicator = 1;
}

const tw_optdef *
tw_net_init(int *argc, char ***argv)
{
   int my_rank, initialized;
   MPI_Initialized(&initialized);
   
   if (!initialized) {
       if (MPI_Init(argc, argv) != MPI_SUCCESS)
	   tw_error(TW_LOC, "MPI_Init failed.");
   }
   if (MPI_Comm_rank(MPI_COMM_ROSS, &my_rank) != MPI_SUCCESS)
       tw_error(TW_LOC, "Cannot get MPI_Comm_rank(MPI_COMM_ROSS)");
   
   g_tw_masternode = 0;
   g_tw_mynode = my_rank;
   
   return mpi_opts;
}

static void
init_q(struct act_q *q, const char *name)
{
  unsigned int n;

  if(q == &posted_sends)
    n = send_buffer;
  else
    n = read_buffer;

  q->name = name;
  q->event_list = (tw_event **) tw_calloc(TW_LOC, name, sizeof(*q->event_list), n);
  q->req_list = (MPI_Request *) tw_calloc(TW_LOC, name, sizeof(*q->req_list), n);
  q->idx_list = (int *) tw_calloc(TW_LOC, name, sizeof(*q->idx_list), n);
  q->status_list = (MPI_Status *) tw_calloc(TW_LOC, name, sizeof(*q->status_list), n);

}

tw_node * tw_net_onnode(tw_peid gid)
{
  id_tmp = gid;
  return &id_tmp;
}

unsigned int
tw_nnodes(void)
{
  return world_size;
}

void
tw_net_start(void)
{
  if (MPI_Comm_size(MPI_COMM_ROSS, &world_size) != MPI_SUCCESS)
    tw_error(TW_LOC, "Cannot get MPI_Comm_size(MPI_COMM_ROSS)");

  if( g_tw_mynode == 0)
    {
      printf("tw_net_start: Found world size to be %d \n", world_size );
    }

  // Check after tw_nnodes is defined
  if(tw_nnodes() == 1 && g_tw_npe == 1) {
      // force the setting of SEQUENTIAL protocol
      if (g_tw_synchronization_protocol == NO_SYNCH) {
          g_tw_synchronization_protocol = SEQUENTIAL;
      } else if(g_tw_synchronization_protocol == CONSERVATIVE || g_tw_synchronization_protocol == OPTIMISTIC) {
          g_tw_synchronization_protocol = SEQUENTIAL;
          fprintf(stderr, "Warning: Defaulting to Sequential Simulation, not enought PEs defined.\n");
      }
  }

  tw_pe_create(1);
  tw_pe_init(0, g_tw_mynode);

  //If we're in (some variation of) optimistic mode, we need this hash
  if (g_tw_synchronization_protocol == OPTIMISTIC ||
      g_tw_synchronization_protocol == OPTIMISTIC_DEBUG ||
      g_tw_synchronization_protocol == OPTIMISTIC_REALTIME) {
    g_tw_pe[0]->hash_t = tw_hash_create();
  } else {
    g_tw_pe[0]->hash_t = NULL;
  }

  if (send_buffer < 1)
    tw_error(TW_LOC, "network send buffer must be >= 1");
  if (read_buffer < 1)
    tw_error(TW_LOC, "network read buffer must be >= 1");

  init_q(&posted_sends, "MPI send queue");
  init_q(&posted_recvs, "MPI recv queue");

  g_tw_net_device_size = read_buffer;
}

void
tw_net_shmem_start(void)
{
  int i, j;
  size_t original_size=0; // part of thread test at end of setup.
  tw_clock mutex_time_start=0;
  tw_clock mutex_time_finish=0;
  tw_clock mutex_time_total=0;
  tw_clock mutex_total_count=0;
  
  if( !g_tw_shm_enabled )
  {
      printf("tw_net_shmem_start: --enableshmem=1 not on cmd line, not creating pools\n");
      return;
  }

  if( g_tw_ranks_per_node == 1 )
  {
      if( g_tw_mynode == 0)
      {
	  printf("tw_net_shmem_start: --rankspernode only 1, increase to use shared memory pools \n");
      }
      return;
  }
  else
  {
      if( g_tw_mynode == 0)
      {
	  printf("tw_net_shmem_start: Starting with %d ranks per node\n", g_tw_ranks_per_node);
      }
  }
     
  /******************************************************************************************************************/
  /* START SHARED MEMORY SETUP -- assumes tw_net_start already called   *********************************************/
  /******************************************************************************************************************/

  // used in CPU bindings
  network_mpishm_num_cpus_per_node = g_tw_ranks_per_node;
  // used in MPI rank and comm operations
  network_mpishm_ranks_per_node = g_tw_ranks_per_node;

  // determine size of pool
  network_mpishm_align = ROSS_MAX(sizeof(double), sizeof(void*));
  network_mpishm_event_length = sizeof(tw_event) + g_tw_msg_sz;
  if (network_mpishm_event_length & (network_mpishm_align - 1))
    {
      network_mpishm_event_length +=
	  network_mpishm_align - (network_mpishm_event_length & (network_mpishm_align - 1));
      tw_printf(TW_LOC, "REALIGNING EVENT MEMORY!\n");
    }
  
  network_mpishm_shared_memory_size = ((sizeof(tw_network_pe) * g_tw_ranks_per_node) +
				       ((sizeof(tw_event) + g_tw_msg_sz) *
					g_tw_shm_events_per_pe * g_tw_ranks_per_node));
  
  // shm init
  network_mpishm_color =  g_tw_mynode / network_mpishm_ranks_per_node;
  network_mpishm_group = network_mpishm_color+1;
  network_mpishm_key = g_tw_mynode % network_mpishm_ranks_per_node; 
  network_mpishm_shared_memory_key = network_mpishm_color+1;
  
  // setup cpu bindings
  CPU_ZERO( network_mpishm_cpu_set );
  network_mpishm_my_cpu = g_tw_mynode % network_mpishm_num_cpus_per_node;
  CPU_SET( network_mpishm_my_cpu, network_mpishm_cpu_set);
  
  if ( sched_setaffinity(getpid(), NETWORK_MPISHM_MAX_CPUS, network_mpishm_cpu_set) == 0 )
  {
      printf("Rank %ld, pid %d is now bound to cpu %d, confirmed by sched_getcpu = %d \n", 
	     g_tw_mynode, getpid(), network_mpishm_my_cpu, sched_getcpu() );
  }
  else
  {
      perror("sched_setaffinity failed: ");
      exit(-1);
  }
  
  /******************************************************************************************************************/
  /* Setup SHM Comms*************************************************************************************************/
  /******************************************************************************************************************/
  
  MPI_Comm_split(network_mpishm_comm, network_mpishm_color, network_mpishm_key, &network_mpishm_shmcomm); 
  MPI_Comm_size (network_mpishm_shmcomm, &network_mpishm_shmcomm_size);
  MPI_Comm_rank (network_mpishm_shmcomm, &g_tw_network_mpishm_shmcomm_rank);
  
  printf ("I'm Comm World Rank = %ld, Shm Comm Rank = %d with a Shm Comm Size of %d and Shared Memory Key %d\n",
	  g_tw_mynode, g_tw_network_mpishm_shmcomm_rank, network_mpishm_shmcomm_size, network_mpishm_shared_memory_key); 
  fflush(stdout);
  
  /******************************************************************************************************************/
  // Create and locate the shared memory segment
  /******************************************************************************************************************/
  MPI_Barrier(network_mpishm_comm);
    
  if( g_tw_network_mpishm_shmcomm_rank == 0 )
  {
      printf("Rank %ld/Shm Rank %d: Attempting shmget cmd with key %d, size %llu, flag %d \n",
	     g_tw_mynode,
	     g_tw_network_mpishm_shmcomm_rank,
	     network_mpishm_shared_memory_key,
	     network_mpishm_shared_memory_size,
	     network_mpishm_shared_memory_create_flag);
      
      if ((network_mpishm_shared_memory_id = shmget(network_mpishm_shared_memory_key,
						    network_mpishm_shared_memory_size,
						    network_mpishm_shared_memory_create_flag)) < 0)
      {
	  perror("create shmget failed: ");
	  exit(-1);
      }
      printf("Rank %ld/Shm Rank %d: Created Shm Pool with ID %d \n", g_tw_mynode, g_tw_network_mpishm_shmcomm_rank, network_mpishm_shared_memory_id);
      MPI_Barrier(network_mpishm_shmcomm);  
  }
  else
  {
      MPI_Barrier(network_mpishm_shmcomm);  // Complete barrier to ensure shmcomm rank 0 has created the segment
      
      if((network_mpishm_shared_memory_id = shmget(network_mpishm_shared_memory_key,
						   network_mpishm_shared_memory_size,
						   network_mpishm_shared_memory_locate_flag)) < 0)
      {
	  perror("locate shmget failed: ");
	  exit(-1);
      }
      printf("Rank %ld/Shm Rank %d: Located Shm Pool with ID %d \n",
	     g_tw_mynode, g_tw_network_mpishm_shmcomm_rank, network_mpishm_shared_memory_id);
  }
  
  /******************************************************************************************************************/
  // Now, let's find a common address to attach to
  /******************************************************************************************************************/
  MPI_Barrier(network_mpishm_shmcomm);  
  
  while(1)
  {
     int fd=0, offset=0;
     void *ret_addr=NULL;
     int counter=0;
     int found_valid_address=0;
     int sum_valid_addresses=0;
     int attach_succeeded=0;
     int sum_attach_succeeded=0;

     // rest each MPI rank test flags
     found_valid_address = 0;
     attach_succeeded = 0;
     
     // start with mmap test
     if( ((ret_addr = mmap( network_mpishm_start_shared_memory_pool_address, 
			    network_mpishm_shared_memory_size, 
			    (PROT_READ | PROT_WRITE), 
			    (MAP_ANONYMOUS | MAP_SHARED), 
			    fd, offset)) != MAP_FAILED) &&
	 (ret_addr == network_mpishm_start_shared_memory_pool_address))
     {
	 printf("Rank %ld: Mmap try %d found available VM region at address %p \n", 
		g_tw_mynode, counter, network_mpishm_start_shared_memory_pool_address );
	 fflush(stdout);
	 
	 found_valid_address = 1;
	 
	 if( munmap(ret_addr, network_mpishm_shared_memory_size) == -1)
	 {
	     perror("munmap: ");
	     exit(-1);
	 }
     }
     else
     {
	 if( ret_addr != MAP_FAILED )
	 {
	     if( munmap(ret_addr, network_mpishm_shared_memory_size) == -1)
	     {
		 perror("munmap: ");
		 exit(-1);
	     }
	 }
	 
	 printf("Rank %ld: Mmap try %d: found no free VM region at address %p, ret addr = %p \n", 
		g_tw_mynode, counter, network_mpishm_start_shared_memory_pool_address, ret_addr );
	 fflush(stdout);
     }
     
     MPI_Allreduce( &found_valid_address, &sum_valid_addresses, 1, MPI_INT, MPI_SUM, network_mpishm_shmcomm);
     
     if( sum_valid_addresses != network_mpishm_shmcomm_size )
     {
	 printf("Rank %ld: NOT all ranks agree on address %p, found valid address = %d TRY AGAIN!\n",
		g_tw_mynode, network_mpishm_start_shared_memory_pool_address, found_valid_address );
	 fflush(stdout);
	 goto continue_address_test;
     }
     else
     {
	 printf("Rank %ld: all ranks agree on address %p, found valid address = %d now try attach test\n",
		g_tw_mynode, network_mpishm_start_shared_memory_pool_address, found_valid_address );
     }
     
     // test attach the shared memory segment in serial fashion
     MPI_Barrier(network_mpishm_shmcomm);  
     
     if( (network_mpishm_shared_memory_pool = shmat( network_mpishm_shared_memory_id,
						     network_mpishm_start_shared_memory_pool_address,
						     SHM_RND)) == (void *)-1 )
     {
	 printf("Rank %ld: ", g_tw_mynode);
	 perror("shmat failed: ");
	 fflush(stdout);
     }
     else
     {
	 printf("Rank %ld: Completed shared memory pool attachment at location %p \n",
		g_tw_mynode, network_mpishm_shared_memory_pool );
	 fflush(stdout);
	 attach_succeeded = 1;
     }
     
     MPI_Allreduce( &attach_succeeded, &sum_attach_succeeded, 1, MPI_INT, MPI_SUM, network_mpishm_shmcomm);
     
     if( sum_attach_succeeded != network_mpishm_shmcomm_size)
     {
	 printf("Rank %ld: NOT all ranks attach on address %p, attach succeeded = %d TRY AGAIN!\n",
		g_tw_mynode, network_mpishm_start_shared_memory_pool_address, found_valid_address );
	 fflush(stdout);
     }
     else
     {
	 printf("Rank %ld: all ranks attached on address %p, found valid address = %d now try shared segment lock/unlock test\n",
		g_tw_mynode, network_mpishm_start_shared_memory_pool_address, found_valid_address );
	 break;
     }
     
  continue_address_test:
     network_mpishm_start_shared_memory_pool_address =
	 (void *)((unsigned long long) network_mpishm_start_shared_memory_pool_address -
		  (unsigned long long) network_mpishm_shared_memory_size);
  }

  // breakup memory pool and allocate events - let each local Rank 0 do the work across groups
  if( g_tw_network_mpishm_shmcomm_rank == 0 )
  {
      printf("Comm World Rank %ld, Shm Comm Rank %d: setting up pool, allocating %llu bytes \n",
	     g_tw_mynode, network_mpishm_rank, network_mpishm_shared_memory_size);
      
      g_tw_network_pe = (tw_network_pe *)network_mpishm_start_shared_memory_pool_address;
      network_mpishm_start_shared_memory_pool_address =
	  network_mpishm_start_shared_memory_pool_address + (sizeof(tw_network_pe) * g_tw_ranks_per_node);

      for( i = 0; i < g_tw_ranks_per_node; i++)
      {
	  // chunk up the event list and attach to free_q for each local MPI rank in this group
	  tw_eventq_shmem_alloc( &(g_tw_network_pe[i].free_q),
				 g_tw_shm_events_per_pe,
				 network_mpishm_start_shared_memory_pool_address, i);
	  // increment pool address pointer
	  network_mpishm_start_shared_memory_pool_address +=
	      network_mpishm_event_length * g_tw_shm_events_per_pe;

	  // init various event list queues to NULL
	  bzero( &(g_tw_network_pe[i].event_q), sizeof(tw_eventq));
	  g_tw_network_pe[i].cancel_q = NULL;
	  bzero( &(g_tw_network_pe[i].return_q), sizeof(tw_eventq));

	  // init the pthread locks located in the shmem pool
	  pthread_mutex_init( &(g_tw_network_pe[i].event_q_lock), NULL);
	  pthread_mutex_init( &(g_tw_network_pe[i].cancel_q_lock), NULL);
	  pthread_mutex_init( &(g_tw_network_pe[i].return_q_lock), NULL);
      }
  }
  else
  {
      g_tw_network_pe = (tw_network_pe *)network_mpishm_start_shared_memory_pool_address;
  }
  // Barrier everyone before returning to the initial output looks reasonable.
  MPI_Barrier(MPI_COMM_ROSS);
  original_size = g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].event_q.size;
  
  printf("Rank %ld: Found event q size to be %ld before pthread lock test \n",
	 g_tw_mynode, g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].event_q.size );

  MPI_Barrier(MPI_COMM_ROSS);
    
  for( i = 0; i < g_tw_ranks_per_node; i++ )
  {
      for( j = 0; j < (1024*1024); j++)
      {
	  mutex_time_start = tw_clock_read();
	  tw_mutex_lock(&(g_tw_network_pe[i].event_q_lock));
	  g_tw_network_pe[i].event_q.size++;
	  tw_mutex_unlock(&(g_tw_network_pe[i].event_q_lock));
	  mutex_time_finish = tw_clock_read();
	  mutex_time_total += mutex_time_finish - mutex_time_start;
	  mutex_total_count++;
      }
      for( j = 0; j < (1024*1024); j++)
      {
	  mutex_time_start = tw_clock_read();
	  tw_mutex_lock(&(g_tw_network_pe[i].event_q_lock));
	  g_tw_network_pe[i].event_q.size--;
	  tw_mutex_unlock(&(g_tw_network_pe[i].event_q_lock));
	  mutex_time_finish = tw_clock_read();
	  mutex_time_total += mutex_time_finish - mutex_time_start;
	  mutex_total_count++;

      }
  }

  MPI_Barrier(MPI_COMM_ROSS);
  
  printf("Rank %ld: Found event q size after test to be %ld, vs %ld original size, cycles per mutex = %lf (%ld cycles / %ld num mutexes) \n",
	 g_tw_mynode, g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].event_q.size, original_size,
	 ((double)(mutex_time_total) / (double)(mutex_total_count)), mutex_time_total, mutex_total_count );
  fflush(stdout);

  MPI_Barrier(MPI_COMM_ROSS);

// reset original size after test
  g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].event_q.size = original_size;

  MPI_Barrier(MPI_COMM_ROSS);
    
  printf("\n");

}

void
tw_net_abort(void)
{
    if( g_tw_ranks_per_node > 1 )
    {
	MPI_Barrier(network_mpishm_shmcomm);
	// detach segment
	shmdt( network_mpishm_shared_memory_pool );
	
	/******************************************************************************************************************/
	// Now, let's clean up our shared memory segments
	/******************************************************************************************************************/
	MPI_Barrier(network_mpishm_shmcomm);  
	if( g_tw_network_mpishm_shmcomm_rank == 0 )
	{
	    sprintf(network_mpishm_ipcrm_command_string, "ipcrm -m %d", network_mpishm_shared_memory_id);
	    system( network_mpishm_ipcrm_command_string );
	}
	/******************************************************************************************************************/
	// Now LEAVE THE MPI WORLD !!!
	/******************************************************************************************************************/
    }
    
    MPI_Abort(MPI_COMM_ROSS, 1);
    exit(1);
}

void
tw_net_stop(void)
{
      if( g_tw_ranks_per_node > 1 )
      {
	  MPI_Barrier(network_mpishm_shmcomm);
	  // detach segment
	  shmdt( network_mpishm_shared_memory_pool );
	  
	  /******************************************************************************************************************/
	  // Now, let's clean up our shared memory segments
	  /******************************************************************************************************************/
	  MPI_Barrier(network_mpishm_shmcomm);  
	  
	  if( g_tw_network_mpishm_shmcomm_rank == 0 )
	  {
	      sprintf(network_mpishm_ipcrm_command_string, "ipcrm -m %d", network_mpishm_shared_memory_id);
	      system( network_mpishm_ipcrm_command_string );
	      printf("Rank %ld leaving MPI World, cleaned up shm segment id %d \n", g_tw_mynode, network_mpishm_shared_memory_id);
	      fflush(stdout);
	  }
	  /******************************************************************************************************************/
	  // Now LEAVE THE MPI WORLD !!!
	  /******************************************************************************************************************/
      }
      
      MPI_Barrier(MPI_COMM_ROSS);
      if (!custom_communicator)
      {
	  if (MPI_Finalize() != MPI_SUCCESS)
	      tw_error(TW_LOC, "Failed to finalize MPI");
      }
}

void
tw_net_barrier(tw_pe * pe)
{
  if (MPI_Barrier(MPI_COMM_ROSS) != MPI_SUCCESS)
    tw_error(TW_LOC, "Failed to wait for MPI_Barrier");
}

tw_stime
tw_net_minimum(tw_pe *me)
{
  tw_stime m = DBL_MAX;
  tw_event *e;
  int i;

  // check network layer events
  for( e = g_tw_network_pe[g_tw_mynode].event_q.head; e != NULL; e = e->next )
  {
      if (m > e->recv_ts)
      {
	  m = e->recv_ts;
      }
  }

  for( e = g_tw_network_pe[g_tw_mynode].cancel_q; e != NULL; e = e->cancel_next )
  {
      if (m > e->recv_ts)
      {
	  m = e->recv_ts;
      }
  }

  // check former network layer events just added to PE's event_q and cancel_q but not processed
  for( e = me->event_q.head; e != NULL; e = e->next )
  {
      if (m > e->recv_ts)
      {
	  m = e->recv_ts;
      }
  }

  for( e = me->cancel_q; e != NULL; e = e->cancel_next )
  {
      if (m > e->recv_ts)
      {
	  m = e->recv_ts;
      }
  }

  // now check MPI remote layer
  e = outq.head;
  while (e) {
    if (m > e->recv_ts)
      m = e->recv_ts;
    e = e->next;
  }

  for (i = 0; i < posted_sends.cur; i++) {
    e = posted_sends.event_list[i];
    if (m > e->recv_ts)
      m = e->recv_ts;
  }

  return m;
}

static int
test_q(
       struct act_q *q,
       tw_pe *me,
       void (*finish)(tw_pe *, tw_event *, char *))
{
  int ready, i, n;

  if (!q->cur)
    return 0;

  if (MPI_Testsome(
		   q->cur,
		   q->req_list,
		   &ready,
		   q->idx_list,
		   q->status_list) != MPI_SUCCESS) {
    tw_error(
	     TW_LOC,
	     "MPI_testsome failed with %u items in %s",
	     q->cur,
	     q->name);
  }

  if (1 > ready)
    return 0;

  for (i = 0; i < ready; i++)
    {
      tw_event *e;

      n = q->idx_list[i];
      e = q->event_list[n];
      q->event_list[n] = NULL;

      finish(me, e, NULL);
    }

  /* Collapse the lists to remove any holes we left. */
  for (i = 0, n = 0; i < q->cur; i++) {
    if (q->event_list[i]) {
      if (i != n) {
	// swap the event pointers
	q->event_list[n] = q->event_list[i];

	// copy the request handles
	memcpy(
	       &q->req_list[n],
	       &q->req_list[i],
	       sizeof(q->req_list[0]));
      }
      n++;
    }
  }
  q->cur -= ready;

  return 1;
}

static int
recv_begin(tw_pe *me)
{
  MPI_Status status;

  tw_event	*e = NULL;

  int flag = 0;
  int changed = 0;

  while (posted_recvs.cur < read_buffer)
    {
      unsigned id = posted_recvs.cur;

      MPI_Iprobe(MPI_ANY_SOURCE,
		 MPI_ANY_TAG,
		 MPI_COMM_ROSS,
		 &flag,
		 &status);

      if(flag)
	{
	  if(!(e = tw_event_grab(me)))
	    {
	      if(tw_gvt_inprogress(me))
		tw_error(TW_LOC, "out of events in GVT!");

	      break;
	    }
	} else
	{
	  return changed;
	}

	if(!flag || 
	   MPI_Irecv(e,
		     (int)EVENT_SIZE(e),
		     MPI_BYTE,
		     MPI_ANY_SOURCE,
		     EVENT_TAG,
		     MPI_COMM_ROSS,
		     &posted_recvs.req_list[id]) != MPI_SUCCESS)
	  {
	    tw_event_free(me, e);
	    return changed;
	  }

      posted_recvs.event_list[id] = e;
      posted_recvs.cur++;
      changed = 1;
    }

  return changed;
}

static void
recv_finish(tw_pe *me, tw_event *e, char * buffer)
{
  tw_pe		*dest_pe;

  me->stats.s_nread_network++;
  me->s_nwhite_recv++;

  //  printf("recv_finish: remote event [cancel %u] FROM: LP %lu, PE %lu, TO: LP %lu, PE %lu at TS %lf \n",
  //	 e->state.cancel_q, (tw_lpid)e->src_lp, e->send_pe, (tw_lpid)e->dest_lp, me->id, e->recv_ts);

  e->dest_lp = tw_getlocal_lp((tw_lpid) e->dest_lp);
  dest_pe = e->dest_lp->pe;

  if(e->send_pe > tw_nnodes()-1)
    tw_error(TW_LOC, "bad sendpe_id: %d", e->send_pe);

  e->cancel_next = NULL;
  e->caused_by_me = NULL;
  e->cause_next = NULL;

  if(e->recv_ts < me->GVT)
    tw_error(TW_LOC, "%d: Received straggler from %d: %lf (%d)", 
	     me->id,  e->send_pe, e->recv_ts, e->state.cancel_q);

  if(tw_gvt_inprogress(me))
    me->trans_msg_ts = ROSS_MIN(me->trans_msg_ts, e->recv_ts);

  // if cancel event, retrieve and flush
  // else, store in hash table
  if(e->state.cancel_q)
    {
      tw_event *cancel = tw_hash_remove(me->hash_t, e, e->send_pe);

      // NOTE: it is possible to cancel the event we
      // are currently processing at this PE since this
      // MPI module lets me read cancel events during 
      // event sends over the network.

      cancel->state.cancel_q = 1;
      cancel->state.remote = 0;

      cancel->cancel_next = dest_pe->cancel_q;
      dest_pe->cancel_q = cancel;

      tw_event_free(me, e);

      return;
    }

  if (g_tw_synchronization_protocol == OPTIMISTIC ||
      g_tw_synchronization_protocol == OPTIMISTIC_DEBUG ||
      g_tw_synchronization_protocol == OPTIMISTIC_REALTIME ) {
    tw_hash_insert(me->hash_t, e, e->send_pe);
    e->state.remote = 1;
  }

  /* NOTE: the final check in the if conditional below was added to make sure
   * that we do not execute the fast case unless the cancellation queue is
   * empty on the destination PE.  Otherwise we need to invoke the normal
   * scheduling routines to make sure that a forward event doesn't bypass a
   * cancellation event with an earlier timestamp.  This is helpful for
   * stateful models that produce incorrect results when presented with
   * duplicate messages with no rollback between them.
   */
  if(me == dest_pe && e->dest_lp->kp->last_time <= e->recv_ts && !dest_pe->cancel_q) {
    /* Fast case, we are sending to our own PE and
     * there is no rollback caused by this send.
     */
    tw_pq_enqueue(dest_pe->pq, e);
    return;
  }

  if (me->node == dest_pe->node) {
    /* Slower, but still local send, so put into top
     * of dest_pe->event_q. 
     */
    e->state.owner = TW_pe_event_q;
    tw_eventq_push(&dest_pe->event_q, e);
    return;
  }

  /* Never should happen; MPI should have gotten the
   * message to the correct node without needing us
   * to redirect the message there for it.  This is
   * probably a serious bug with the event headers
   * not being formatted right.
   */
  tw_error(
	   TW_LOC,
	   "Event recived by PE %u but meant for PE %u",
	   me->id,
	   dest_pe->id);
}

static int
send_begin(tw_pe *me)
{
  int changed = 0;

  while (posted_sends.cur < send_buffer)
    {
      tw_event *e = tw_eventq_peek(&outq);
      tw_node	*dest_node = NULL;

      unsigned id = posted_sends.cur;

      if (!e)
	break;

      if(e == me->abort_event)
	tw_error(TW_LOC, "Sending abort event!");

      dest_node = tw_net_onnode((*e->src_lp->type->map)
				((tw_lpid) e->dest_lp));

      if(!e->state.cancel_q)
	e->event_id = (tw_eventid) ++me->seq_num;

      e->send_pe = (tw_peid) g_tw_mynode;

      if (MPI_Isend(e,
		    (int)EVENT_SIZE(e),
		    MPI_BYTE,
		    (int)*dest_node,
		    EVENT_TAG,
		    MPI_COMM_ROSS,
		    &posted_sends.req_list[id]) != MPI_SUCCESS) {
	return changed;
      }

      tw_eventq_pop(&outq);
      e->state.owner = e->state.cancel_q
	? TW_net_acancel
	: TW_net_asend;

      posted_sends.event_list[id] = e;
      posted_sends.cur++;
      me->s_nwhite_sent++;

      changed = 1;
    }
  return changed;
}

static void
send_finish(tw_pe *me, tw_event *e, char * buffer)
{
  me->stats.s_nsend_network++;

  if (e->state.owner == TW_net_asend) {
    if (e->state.cancel_asend) {
      /* Event was cancelled during transmission.  We must
       * send another message to pass the cancel flag to
       * the other node.
       */
      e->state.cancel_asend = 0;
      e->state.cancel_q = 1;
      tw_eventq_push(&outq, e);
    } else {
      /* Event finished transmission and was not cancelled.
       * Add to our sent event queue so we can retain the
       * event in case we need to cancel it later.  Note it
       * is currently in remote format and must be converted
       * back to local format for fossil collection.
       */
      e->state.owner = TW_pe_sevent_q;
      if( g_tw_synchronization_protocol == CONSERVATIVE )
	tw_event_free(me, e);
    }

    return;
  }

  if (e->state.owner == TW_net_acancel) {
    /* We just finished sending the cancellation message
     * for this event.  We need to free the buffer and
     * make it available for reuse.
     */
    tw_event_free(me, e);
    return;
  }

  /* Never should happen, not unless we somehow broke this
   * module's other functions related to sending an event.
   */

  tw_error(
	   TW_LOC,
	   "Don't know how to finish send of owner=%u, cancel_q=%d",
	   e->state.owner,
	   e->state.cancel_q);

}

static void
service_queues(tw_pe *me)
{
  int changed;
  do {
    changed  = test_q(&posted_recvs, me, recv_finish);
    changed |= test_q(&posted_sends, me, send_finish);
    changed |= recv_begin(me);
    changed |= send_begin(me);
  } while (changed);
}

/*
 * NOTE: Chris believes that this network layer is too aggressive at
 * reading events out of the network.. so we are modifying the algorithm
 * to only send events when tw_net_send it called, and only read events
 * when tw_net_read is called.
 */
void
tw_net_read(tw_pe *me)
{
    tw_event *head=NULL;
    tw_event *e=NULL;
    tw_event *e_next=NULL;
    tw_event *e_cnext=NULL;
    
    // service shmem layer
    if( g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].event_q.size )
    {
	tw_mutex_lock( &(g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].event_q_lock));
	head = tw_eventq_pop_list(&(g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].event_q));
	tw_mutex_unlock( &(g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].event_q_lock));

	// iterate over returned list starting with e and add to my pe's event q
	for( e = head; e != NULL; e = e_next)
	{
	    e_next = e->next; // need to grab early because e->next is modified below
	    
	    me->s_nwhite_recv++;

	    // set event state variables
	    e->dest_lp = tw_getlocal_lp((tw_lpid) e->dest_lp);
	    e->state.owner = TW_pe_event_q; // transition from TW_net_shmem_event_q to dest pe's event q.
	    e->caused_by_me = NULL;
	    e->cause_next = NULL;
	    // note, don't mess with e->next_cancel here!!
	    
	    // insert on PE's event q list.
	    tw_eventq_push( &me->event_q, e);
	    
	    /* printf("tw_net_read: Fwd Ev(%p) at TS=%lf, Dest LP %ld, Src Pool %d, Dest Shmcomm Rank %d, Dest Global Rank %ld, RECV Count %lld \n", */
            /*         e, e->recv_ts, e->dest_lp->gid, e->shmem_pool_id, g_tw_network_mpishm_shmcomm_rank, g_tw_mynode, me->s_nwhite_recv ); */
	}
    }

    if( g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].cancel_q != NULL )
    {
	tw_mutex_lock( &(g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].cancel_q_lock));
	head = g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].cancel_q;
	g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].cancel_q = NULL;
	tw_mutex_unlock( &(g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].cancel_q_lock));

	for( e = head; e != NULL; e = e_cnext )
	{
	    e_cnext = e->cancel_next; // need to grab early incase e->cancel_next is modified below
	    
	    if( e->state.owner == TW_net_shmem_event_q )
	    {
		// e is still in network layer event q and has not offically been read yet.
		// Re-insert back on cancel_q and let it be read first and make sure not to count it has having been recv'ed
		tw_mutex_lock( &(g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].cancel_q_lock));
		e->cancel_next = g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].cancel_q;
		g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].cancel_q = e;
		tw_mutex_unlock( &(g_tw_network_pe[g_tw_network_mpishm_shmcomm_rank].cancel_q_lock));
		
		/* printf("tw_net_read: Re-Insert Cancel Ev(%p) at TS=%lf as NOT YET read as FWD Ev, Dest LP %ld, Src Pool %d, Dest Shmcomm Rank %d, Dest Global Rank %ld, RECV Count %lld \n", */
		/*        e, e->recv_ts, (tw_lpid)e->dest_lp, e->shmem_pool_id, g_tw_network_mpishm_shmcomm_rank, g_tw_mynode, me->s_nwhite_recv ); */

		continue;
	    }

	    // now we can count it!!
	    me->s_nwhite_recv++;

	    // set event state - look for additional items that need setting - remember this is shared memory
	    e->state.cancel_q = 1;
	    e->state.remote = 1;
	    // e->dest_lp was set by tw_getlocal_lp((tw_lpid) e->dest_lp) when read as a forward event.
	    
	    // insert onto PE's cancel queue.
	    e->cancel_next = me->cancel_q;
	    me->cancel_q = e;

 	    // printf("tw_net_read: Cancel Ev(%p) at TS=%lf, Dest LP %ld, Src Pool %d, Dest Shmcomm Rank %d, Dest Global Rank %ld, RECV Count %lld \n",
	    //   e, e->recv_ts, e->dest_lp->gid, e->shmem_pool_id, g_tw_network_mpishm_shmcomm_rank, g_tw_mynode, me->s_nwhite_recv );
	}
    }
    // service the network layer
    service_queues(me);
}

void
tw_net_send(tw_event *e)
{
  tw_pe * me = e->src_lp->pe;
  int changed = 0;
  int remote_pool_id = 0;
  int rank_diff=0;
  
  if( e->shmem_pool_id != -1 )
  {
      // must be dest minus source for alogorithm to work right since we add back the diff to the shmcomm rank number.
      rank_diff = e->dest_pe - g_tw_mynode;
      remote_pool_id = g_tw_network_mpishm_shmcomm_rank + rank_diff;
      // don't have to check pool block here - we can assume they are the same at this point
      
      me->s_nwhite_sent++;
      e->event_id = (tw_eventid) ++me->seq_num;
      e->state.owner = TW_net_shmem_event_q; // Treat send as complete and not cancelled since it is instant.
      
      if( !e->state.cancel_q )
      {
	  tw_mutex_lock( &(g_tw_network_pe[remote_pool_id].event_q_lock));
	  tw_eventq_push(&(g_tw_network_pe[remote_pool_id].event_q), e);
	  
	  /* printf("tw_net_send: After push to pool %d, Event Q size = %ld, Ev Q Head = %p, Ev Q Tail = %p, e = %p, e->next = %p, e->prev = %p \n", */
	  /* 	 remote_pool_id, g_tw_network_pe[remote_pool_id].event_q.size, */
	  /* 	 g_tw_network_pe[remote_pool_id].event_q.head, */
	  /* 	 g_tw_network_pe[remote_pool_id].event_q.tail, e, e->next, e->prev); */
	  
	  tw_mutex_unlock( &(g_tw_network_pe[remote_pool_id].event_q_lock));
      }
      else
      {
	  tw_error( TW_LOC, "Atempting to send a SHMEM cancel message via tw_net_send\n");
      }
      /* printf("tw_net_send: Fwd Ev(%p) at TS=%lf, Dest LP %ld, Src Pool %d, Src Shmcomm Rank %d, Remote Pool ID %d, Src Global Rank %ld, SEND Count %lld \n", */
      /* 	     e, e->recv_ts, (tw_lpid) e->dest_lp, e->shmem_pool_id, g_tw_network_mpishm_shmcomm_rank, remote_pool_id, g_tw_mynode, me->s_nwhite_sent ); */
   
      // Do not need to check network/MPI layer at this time.
  }
  else
  {
      e->state.remote = 0;
      e->state.owner = TW_net_outq;
      tw_eventq_unshift(&outq, e);

      do
      {
	  changed = test_q(&posted_sends, me, send_finish);
	  changed |= send_begin(me);
      } while (changed);
  }
}

void
tw_net_cancel(tw_event *e)
{
  tw_pe *src_pe = e->src_lp->pe;
  int remote_pool_id = 0;
  int rank_diff = 0;
  
  // first see if this is a shmem cancel event
  if( e->shmem_pool_id != -1 )
  {
      // compute the dest pool - again must be dest - src for algo to work right.
     rank_diff = e->dest_pe - g_tw_mynode;
     remote_pool_id = g_tw_network_mpishm_shmcomm_rank + rank_diff;
     src_pe->s_nwhite_sent++;

     // set event state to cancel q - probably will get set as well on recv
     e->state.cancel_q = 1;
     // Note, don't mess with state.owner as it was set already via the original send.
     
     // lock and insert cancel event on shared memory queue
     tw_mutex_lock( &(g_tw_network_pe[remote_pool_id].cancel_q_lock));
     e->cancel_next = g_tw_network_pe[remote_pool_id].cancel_q;
     g_tw_network_pe[remote_pool_id].cancel_q = e;
     tw_mutex_unlock( &(g_tw_network_pe[remote_pool_id].cancel_q_lock));

     /* printf("tw_net_cancel: Canel Ev(%p) at TS=%lf, Dest LP %ld, Src Pool %d, Src Shmcomm Rank %d, Remote Pool ID %d, Src Global Rank %ld, SEND Count %lld \n", */
     /* 	    e, e->recv_ts, (tw_lpid)e->dest_lp, e->shmem_pool_id, g_tw_network_mpishm_shmcomm_rank, remote_pool_id, g_tw_mynode, src_pe->s_nwhite_sent ); */

     // also service queues for other network activity just incase
     service_queues(src_pe);
     
     return;
  }


  // if not, then do the regular network cancel
  switch (e->state.owner) {
  case TW_net_outq:
    /* Cancelled before we could transmit it.  Do not
     * transmit the event and instead just release the
     * buffer back into our own free list.
     */
    tw_eventq_delete_any(&outq, e);
    tw_event_free(src_pe, e);

    return;

    break;

  case TW_net_asend:
    /* Too late.  We've already let MPI start to send
     * this event over the network.  We can't pull it
     * back now without sending another message to do
     * the cancel.
     *
     * Setting the cancel_q flag will signal us to do
     * another message send once the current send of
     * this message is completed.
     */
    e->state.cancel_asend = 1;
    break;

  case TW_pe_sevent_q:
    /* Way late; the event was already sent and is in
     * our sent event queue.  Mark it as a cancel and
     * place it at the front of the outq.
     */
    e->state.cancel_q = 1;
    tw_eventq_unshift(&outq, e);
    break;

  default:
    /* Huh?  Where did you come from?  Why are we being
     * told about you?  We did not send you so we cannot
     * cancel you!
     */
    tw_error(
	     TW_LOC,
	     "Don't know how to cancel event owned by %u",
	     e->state.owner);
  }

  service_queues(src_pe);
}

/**
 * tw_net_statistics
 * @brief Function to output the statistics
 * @attention Notice that the MPI_Reduce "count" parameter is greater than one.
 * We are reducing on multiple variables *simultaneously* so if you change
 * this function or the struct tw_statistics, you must update the other.
 **/
tw_statistics	*
tw_net_statistics(tw_pe * me, tw_statistics * s)
{
  if(MPI_Reduce(&(s->s_max_run_time), 
		&me->stats.s_max_run_time,
		1,
		MPI_DOUBLE,
		MPI_MAX,
		(int)g_tw_masternode,
		MPI_COMM_ROSS) != MPI_SUCCESS)
    tw_error(TW_LOC, "Unable to reduce statistics!");

  if(MPI_Reduce(&(s->s_net_events), 
		&me->stats.s_net_events,
		16,
		MPI_UNSIGNED_LONG_LONG,
		MPI_SUM,
		(int)g_tw_masternode,
		MPI_COMM_ROSS) != MPI_SUCCESS)
    tw_error(TW_LOC, "Unable to reduce statistics!");

  if(MPI_Reduce(&s->s_total, 
		&me->stats.s_total,
		8,
		MPI_UNSIGNED_LONG_LONG,
		MPI_MAX,
		(int)g_tw_masternode,
		MPI_COMM_ROSS) != MPI_SUCCESS)
    tw_error(TW_LOC, "Unable to reduce statistics!");
    
  if(MPI_Reduce(&s->s_pe_event_ties,
        &me->stats.s_pe_event_ties,
        1,
        MPI_UNSIGNED_LONG_LONG,
        MPI_SUM,
        (int)g_tw_masternode,
        MPI_COMM_ROSS) != MPI_SUCCESS)
    tw_error(TW_LOC, "Unable to reduce statistics!");
     
  if(MPI_Reduce(&s->s_min_detected_offset,
        &me->stats.s_min_detected_offset,
        1,
        MPI_DOUBLE,
        MPI_MIN,
        (int)g_tw_masternode,
        MPI_COMM_ROSS) != MPI_SUCCESS)
    tw_error(TW_LOC, "Unable to reduce statistics!");
    
  if(MPI_Reduce(&s->s_avl,
        &me->stats.s_avl,
        1,
        MPI_UNSIGNED_LONG_LONG,
        MPI_MAX,
        (int)g_tw_masternode,
        MPI_COMM_ROSS) != MPI_SUCCESS)
    tw_error(TW_LOC, "Unable to reduce statistics!");

    if (MPI_Reduce(&s->s_buddy,
        &me->stats.s_buddy,
        1,
        MPI_UNSIGNED_LONG_LONG,
        MPI_MAX,
        (int)g_tw_masternode,
        MPI_COMM_ROSS) != MPI_SUCCESS)
    tw_error(TW_LOC, "Unable to reduce statistics!");

    if (MPI_Reduce(&s->s_lz4,
        &me->stats.s_lz4,
        1,
        MPI_UNSIGNED_LONG_LONG,
        MPI_MAX,
        (int)g_tw_masternode,
        MPI_COMM_ROSS) != MPI_SUCCESS)
    tw_error(TW_LOC, "Unable to reduce statistics!");

    if (MPI_Reduce(&s->s_events_past_end,
        &me->stats.s_events_past_end,
        1,
        MPI_UNSIGNED_LONG_LONG,
        MPI_SUM,
        (int)g_tw_masternode,
        MPI_COMM_ROSS) != MPI_SUCCESS)
    tw_error(TW_LOC, "Unable to reduce statistics!");
    
  return &me->stats;
}
