/***************************************************************************/
/* Includes ****************************************************************/
/***************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<stdarg.h>
#include"tcp.h"

/***************************************************************************/
/* Defines *****************************************************************/
/***************************************************************************/

#define MAX_NODES 32000
#define MAX_LINKS_PER_NODE 128

#define MAX_LINE_WIDTH 2048
#define MAX_BUFFER_SIZE  80

#define MAX_LEVELS 16

#define MAX_KPS 256
#define MAX_PES 4

/***************************************************************************/
/* Type Decs ***************************************************************/
/***************************************************************************/

/* measured in Mb/sec */
int bandwith_classes[MAX_LEVELS] = {9920, 2480, 620, 155, 45, 45, 45, 45};

/***************************************************************************/
/* Function Decs ***********************************************************/
/***************************************************************************/

void init_node_list();
void print_node_list();
int read_rocketfuel_file( char *filename );
void parse_rocketfuel_line( char *buffer );
char *get_rocketfuel_item( char *item, char *buffer);
void color_topology( int levels );
void map_nodes_to_ross();


/***************************************************************************/
/* Global Vars *************************************************************/
/***************************************************************************/

//struct rocket_fuel_node g_node_list[MAX_NODES];
struct rocket_fuel_node *g_node_list;
unsigned long g_node_count=0;
FILE *g_rocketfuel_file = NULL;
unsigned long g_max_links =  0;
int g_level_count[MAX_LEVELS];
int g_kp_count[MAX_KPS];

/***************************************************************************/
/* Function Bodys **********************************************************/
/***************************************************************************/

/* Start: init_node_list ***************************************************/

void init_node_list()
{
  int i;
  g_node_list = malloc(sizeof(struct rocket_fuel_node) *32000);

  for( i = 0; i < MAX_NODES; i++ )
    {
      g_node_list[i].used = 0;
      g_node_list[i].is_bb = 0;
      g_node_list[i].level = -1;
      g_node_list[i].num_in_level = 0;
      g_node_list[i].num_links = 0;
      g_node_list[i].kp = -1;
      g_node_list[i].pe = -1;
    }
}

/* Start: print_node_list **************************************************/
void print_node_list()
{
  int i, j;

  for( i = 0; i < g_node_count; i++ )
    {
      if( g_node_list[i].used )
	{
	  printf("Node %d has the following links \n", i );

	  for( j = 0; j < g_node_list[i].num_links; j++ )
	    printf("   Link %d: %d \n", j, g_node_list[i].link_list[j].node_id );

	  if( g_node_list[i].num_links > g_max_links )
	    g_max_links = g_node_list[i].num_links;
	}
    }

  printf("max links is %lu\n", g_max_links);
}

/* Start: read_grade_file **************************************************/
int read_rocketfuel_file( char *filename )
{
  int i;
  char buffer[MAX_LINE_WIDTH];
  int number_of_nodes = 0;

  if( NULL == (g_rocketfuel_file = fopen( filename, "r" )) )
    {
      printf("Error in main: failed to open file for reason: %s \n",
	      strerror( errno ) );
      exit(1);
    }

  while( NULL != fgets( buffer, MAX_LINE_WIDTH, g_rocketfuel_file ) )
    {
      i = number_of_nodes;
      number_of_nodes++;

      if( number_of_nodes == MAX_NODES )
	{
	  printf("Error in read_rocketfuel_file: number of nodes > MAX \n" );
	  exit(1);
	}

      parse_rocketfuel_line( buffer );
    }

  printf("read_rocketfuel_file: File %s completely read, found %d nodes and ready for next task \n",
          filename, number_of_nodes );

  fclose( g_rocketfuel_file );
  g_rocketfuel_file = NULL;

  return( number_of_nodes );
}

/* Start: parse_rocketfuel_line ********************************************/
void parse_rocketfuel_line( char *buffer )
{
  int i, j;
  char item[MAX_LINE_WIDTH];
  char *position = buffer;
  int node = 0;

  /* factor out lines that are comments */
  if( buffer[0] == '#' )
    return;

  /* get uid */
  position = get_rocketfuel_item( item, position );
  node = atoi( item );

  /* nodes < 0 are external to this AS */
  if( node < 0 )
    return;

  /* 
   *nodes are not quite sequential but they are
   *monotonically increasing
   *so, we always record the last node found.
   */
  g_node_count = node;
  g_node_list[node].used = 1;

  /* consume location */
  position = get_rocketfuel_item( item, position );

  /* next check to see if next character is a
   * +, and or bb 
   */
  position = get_rocketfuel_item( item, position );
  if( item[0] == '+' )
    {

      /* consume the bb, if exists */
      position = get_rocketfuel_item( item, position );
      if( item[0] == 'b' )
	{
	  g_node_list[node].is_bb = 1;

	  /* get the number of links */
	  position = get_rocketfuel_item( item, position );
	}
    }

  for( i = 1; i < strlen(item); i++)
    {
      item[i-1] = item[i];
      if( item[i] == ')' )
	{
	  item[i-1] = '\n';
	  continue;
	}
    }
  g_node_list[node].num_links = atoi( item );
  /* consume -> or &# */
  position = get_rocketfuel_item( item, position );
  if( item[0] == '&')
    {
      /* consume the -> */
      position = get_rocketfuel_item( item, position );
    }

  for( j = 0; j < g_node_list[node].num_links; j++)
    {
      position = get_rocketfuel_item( item, position );
      
      for( i = 1; i < strlen(item); i++)
	{
	  item[i-1] = item[i];
	  if( item[i] == '>' || item[i] == '}')
	    {
	      item[i-1] = (char)0;
	      continue;
	    }
	}

      if( atoi( item ) >= 0 )
	g_node_list[node].link_list[j].node_id = atoi( item );
      else
	break;
    }
  g_node_list[node].num_links = j;
}

/* Start: get_rocketfuel_item **********************************************/
char *get_rocketfuel_item( char *item, char *buffer)
{
  int i;
  int n;
  char *position = buffer;
  int j = 0;

  n = strlen(position);

  /* consume initial tab or space characters */
  for( i = 0; position[i] == '\t' || position[i] == ' '; i++ );

  /* copy all non-space chars into item */
  for( ; i < n; i++ )
    {
      if( position[i] == '\t' || position[i] == ' ' )
	break;
      else
	{
	  item[j] = position[i];
	  j++;
	}
    }
  
  /* null terminate the item string */
  item[j] = (char)0;

  /* return start of next item in buffer */
  return( &position[i] );
}

/* Start: partition_topology ***********************************************/

void color_topology( int levels )
{
    int i, j, k;
    int max_levels = 0;
    
    /* first pass establish level 0, super core */
    
    g_level_count[0] = 0;
    for( i = 0; i < g_node_count; i++)
    {
        if( g_node_list[i].is_bb )
        {
            g_node_list[i].level = 0;
            g_level_count[0]++;
        }
    }
    
    for( i = 1; i < levels; i++ )
    {
        g_level_count[i] = 0;
        
        for( j = 0; j < g_node_count; j++ )
        {
            if( g_node_list[j].level < 0 )
            {
                for( k = 0; k < g_node_list[j].num_links; k++)
                {
                    if( g_node_list[g_node_list[j].link_list[k].node_id].level == i - 1 )
                    {
                        g_node_list[j].level = i;
                        g_node_list[j].num_in_level = g_level_count[i];
                        g_level_count[i]++;
                        
                        break;
                    }
                }
            }
        }
        if( g_level_count[i] == 0 )
        {
            printf("MAX levels is %d \n", i);
            max_levels = i;
            break;
        }
    }
    
    if( max_levels == levels -1)
    {
        printf("WARNING: max levels of topology may not have been reached!! \n");
        printf("         re-run again with higher level input to color function\n");
        exit(-1);
    }
    
    for( i = 0; i < levels; i++)
        printf("Level %d has %d Nodes \n", i, g_level_count[i] );
    
    printf("Nodes not reached from BB include...\n");
    printf("   note: nodes with No links have only external AS links\n");
    printf("         which are pruned \n");
    
    /*for( i = 0; i < g_node_count; i++)
     {
     if( (g_node_list[i].used) && (g_node_list[i].level == -1) )
     {
     printf("Node %d: ", i );
     for( j = 0; j < g_node_list[i].num_links; j++ )
     printf("<%d> ", g_node_list[i].link_list[j].node_id );
     printf("\n");
     }
     } */
}
/* Start: map_nodes_to_kps *************************************************/
void map_nodes_to_ross()
{
  int i;
  int num_kps;
  int num_pes;
  int kps_per_pe;
  int num_nodes_per_kp;
  int num_lps = 0;

 top:
  //printf("Please enter the number of KPs: \n");
  //scanf("%d", &num_kps);
  //g_nkp = num_kps;
  num_kps = g_nkp;
  num_pes = g_npe;
  //printf("Please enter the number of PEs: \n");
  //scanf("%d", &num_pes);
  //g_npe = num_pes;
  printf("Staring mapping process using %d KPs and %d PEs\n", num_kps, num_pes );

  if( num_kps > MAX_KPS )
    {
      printf("error: KPs > MAX(%d) \n", MAX_KPS );
      goto top;
    }
  if( num_pes > MAX_PES )
    {
      printf("error: PEs > MAX(%d) \n", MAX_PES );
      goto top;
    }
  if( num_kps % num_pes != 0 )
    {
      printf("error: uneven KP(%d) to PE(%d) mapping \n", num_kps, num_pes);
      goto top;
    }

  for( i = 0; i < num_kps; i++)
    g_kp_count[i] = 0;

  kps_per_pe = num_kps / num_pes;

  for( i = 0; i < g_node_count; i++ )
    {
      if( g_node_list[i].used && g_node_list[i].num_links > 0 )
	{
	  num_nodes_per_kp = g_level_count[g_node_list[i].level] / num_kps;
	  if( g_level_count[g_node_list[i].level] % num_kps )
	    num_nodes_per_kp++;
	  if( num_nodes_per_kp == 0)
	    num_nodes_per_kp = 1;
	  //g_node_list[i].kp = (g_node_list[i].num_in_level / 
	  //		       num_nodes_per_kp) % num_kps;
	  g_node_list[i].kp = g_node_list[i].num_in_level % num_kps;
	  g_node_list[i].pe = g_node_list[i].kp / kps_per_pe;

	  g_kp_count[g_node_list[i].kp]++;
	  num_lps++;
	}
    }
  
  for( i = 0; i < num_kps; i++ )
    printf("KP %d has %d Node \n", i, g_kp_count[i] );
  printf("Total Num LPs is %d  \n", num_lps);
}

/***************************************************************************/
/* Main ********************************************************************/
/***************************************************************************/

/*int main( int argc, char *argv[] )
{
  int number_of_nodes=0;

  if( 2 > argc )
    {
      printf("Error in main: missing grade file argument \n");
      exit(1);
    }

  init();

  number_of_nodes = read_rocketfuel_file( argv[1] );

  color_topology( MAX_LEVELS );

  map_nodes_to_ross();
  // print_node_list();

  return(1);
}*/


