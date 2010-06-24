#include <ross.h>

typedef struct tw_event_head tw_event_head;

/*
 * function calls 
 */

static tw_event *Delete(tw_pq * p_ptr, int index);
static void     Insert(tw_pq * p_ptr, int bucketindex, tw_event * NewE);
static void     LocalInit(tw_pq * p_ptr, int qbase, int nbuck, double bwidth, double startprio);
static double   NewWidth(tw_pq * p_ptr);
static void     ReSize(tw_pq * p_ptr, int);

static int MAX_EVENTS_PER_PE;

struct tw_event_head
{
	tw_event       *tw_volatile next;
	tw_event       *tw_volatile prev;
};

struct tw_pq
{
	tw_event_head  *OrigBucket;
	tw_event_head  *Bucket;
	tw_stime        Width;
	tw_stime        BucketTop;
	tw_stime        LastPriority;
	int             Nbuckets;
	int             LastBucket;
	int             Qsize;
	int             TopThreshold;
	int             BottomThreshold;
	char            ResizeEnabled;
	int             FirstSub;
};

tw_event       *
tw_pq_dequeue(tw_pq *p_ptr)
{
	int             i;
	int             smallest;
	tw_event       *E;
	double          tmp;

	/*
	 * pointer to Calender queue bucket 
	 */
	tw_event_head  *bucket;

	if (p_ptr->Qsize == 0)
		return NULL;

	bucket = p_ptr->Bucket;
	i = p_ptr->LastBucket;
	for (;;)
	{
		if ((bucket[i].next != NULL)
			&& (bucket[i].next->recv_ts < p_ptr->BucketTop))
		{
			p_ptr->LastBucket = i;
			p_ptr->LastPriority = bucket[i].next->recv_ts;
			E = Delete(p_ptr, i);
			p_ptr->Qsize--;

			if (p_ptr->ResizeEnabled && p_ptr->Qsize < p_ptr->BottomThreshold)
				ReSize(p_ptr, p_ptr->Nbuckets / 2);
			goto END;
		} else
		{
			++i;
			if (i == p_ptr->Nbuckets)
				i = 0;
			p_ptr->BucketTop += p_ptr->Width;
			if (i == p_ptr->LastBucket)
				break;
		}
	}

	/*
	 * Start Direct Search 
	 */
	for (smallest = 0; bucket[smallest].next == NULL; smallest++)
	{
		if (smallest >= p_ptr->Nbuckets)
		{
			tw_error(TW_LOC, "Grave inconsistency in calendar queue:\n"
					 "\t\tNbuckets = %d, p_ptr->Qsize = %d",
					 p_ptr->Nbuckets, p_ptr->Qsize);
		}
	}

	for (i = smallest + 1; i < p_ptr->Nbuckets; i++)
	{
		if (bucket[i].next == NULL)
			continue;
		else if (bucket[i].next->recv_ts < bucket[smallest].next->recv_ts)
			smallest = i;
	}

	p_ptr->Qsize--;
	E = Delete(p_ptr, smallest);
	p_ptr->LastBucket = smallest;
	p_ptr->LastPriority = E->recv_ts;
	tmp = p_ptr->Width * 0.5;
	p_ptr->BucketTop = p_ptr->Width *
		(double)((int)(p_ptr->LastPriority / p_ptr->Width) + 1);
	p_ptr->BucketTop = p_ptr->BucketTop + tmp;

  END:
	E->state.owner = 0;
	return E;
}

/*
 * deletes first element in Bucket[index]
 * Changes Bucket[index] 
 */
static tw_event *
Delete(tw_pq * p_ptr, int index)
{
	tw_event       *temp;
	tw_event_head  *bucket;

	bucket = p_ptr->Bucket;

	temp = bucket[index].next;
	if (temp != NULL)
	{
		bucket[index].next = temp->next;
		if (bucket[index].next != NULL)
			bucket[index].next->prev = (tw_event *) & bucket[index];
	}
	temp->next = NULL;
	temp->prev = NULL;

	return temp;
}

void
tw_pq_enqueue(tw_pq *p_ptr, tw_event * NewE)
{
	int             i;
	double          tmp;

	i = (int)(NewE->recv_ts / p_ptr->Width);
	i = i % p_ptr->Nbuckets;

	Insert(p_ptr, i, NewE);
	++(p_ptr->Qsize);

	/*
	 * Code for Out of Sequence EnQueues and DeQueues. Required for insertions
	 * after a rollback.
	 */

	if (NewE->recv_ts <= p_ptr->LastPriority)
	{
		tmp = p_ptr->Width * 0.5;
		p_ptr->LastPriority = NewE->recv_ts;
		p_ptr->BucketTop = p_ptr->Width *
			(double)((int)(p_ptr->LastPriority / p_ptr->Width) + 1);
		p_ptr->BucketTop = p_ptr->BucketTop + tmp;
		p_ptr->LastBucket = i;
	}
	if (p_ptr->ResizeEnabled && p_ptr->Qsize > p_ptr->TopThreshold)
	{
		ReSize(p_ptr, 2 * p_ptr->Nbuckets);
	}

	NewE->state.owner = TW_pe_pq;
}

static void
Insert(tw_pq * p_ptr, int bucketindex, tw_event * NewE)
{
	tw_event       *p;
	tw_event_head  *bucket;

	bucket = &p_ptr->Bucket[0];

	if (bucket[bucketindex].next == NULL)
	{
		bucket[bucketindex].next = NewE;
		NewE->next = NULL;
		NewE->prev = (tw_event *) & bucket[bucketindex];
	} else if (bucket[bucketindex].next->recv_ts >= NewE->recv_ts)
	{
		/*
		 * insert at the front of this bucket 
		 */
		NewE->next = bucket[bucketindex].next;
		NewE->next->prev = NewE;
		NewE->prev = (tw_event *) & bucket[bucketindex];
		bucket[bucketindex].next = NewE;
	} else
	{
		/*
		 * insert somewhere else, may be at the end 
		 */
		for (p = bucket[bucketindex].next; p->next != NULL; p = p->next)
		{
			if ((p->recv_ts < NewE->recv_ts)
				&& (p->next->recv_ts >= NewE->recv_ts))
				break;
		}
		NewE->next = p->next;
		if (p->next != NULL)
			p->next->prev = NewE;
		p->next = NewE;
		NewE->prev = p;
	}
}

static void
LocalInit(tw_pq * p_ptr, int qbase, int nbuck, double bwidth,
		  double startprio)
{
	int             i;
	int             n;
	double          tmp;

	/*
	 * Local variables 
	 */

	p_ptr->FirstSub = qbase;
	p_ptr->Bucket = &p_ptr->OrigBucket[p_ptr->FirstSub];
	p_ptr->Width = bwidth;
	p_ptr->Nbuckets = nbuck;
	p_ptr->Qsize = 0;

	for (i = 0; i < nbuck; i++)
		p_ptr->Bucket[i].next = NULL;

	p_ptr->LastPriority = startprio;
	n = (int)startprio / bwidth;
	p_ptr->LastBucket = (n % nbuck);
	p_ptr->BucketTop = bwidth * (double)(n + 1);
	tmp = bwidth * 0.5;
	p_ptr->BucketTop = p_ptr->BucketTop + tmp;
	p_ptr->BottomThreshold = nbuck / 2 - 2;
	p_ptr->TopThreshold = 2 * nbuck;
}

/*
 * does not change queue,
 * however, does dequeue nsamples and enqueues them back again 
 */

static double
NewWidth(tw_pq * p_ptr)
{
	int             i;
	int             nsamples;
	int             lastbucket;
	int             count;
	double          AVG[25];
	double          avg1;
	double          avg2;
	double          buckettop;
	double          lastprio;
	tw_event       *E[25];

	avg1 = avg2 = 0.0;

	if (p_ptr->Qsize < 2)
	{
		return 1.0;
	} else if (p_ptr->Qsize <= 5)
		nsamples = p_ptr->Qsize;
	else
		nsamples = 5 + (p_ptr->Qsize / 10);
	if (nsamples > 25)
		nsamples = 25;

	p_ptr->ResizeEnabled = 0;
	buckettop = p_ptr->BucketTop;
	lastprio = p_ptr->LastPriority;
	lastbucket = p_ptr->LastBucket;

	for (i = 0; i < nsamples; i++)
	{
		E[i] = tw_pq_dequeue(p_ptr);
	}

	for (i = 0; i < nsamples - 1; i++)
	{
		AVG[i] = E[i + 1]->recv_ts - E[i]->recv_ts;
		avg1 = avg1 + AVG[i];
	}

	for (i = 0; i < nsamples; i++)
	{
		tw_pq_enqueue(p_ptr, E[i]);
	}
	p_ptr->ResizeEnabled = 1;
	p_ptr->BucketTop = buckettop;
	p_ptr->LastPriority = lastprio;
	p_ptr->LastBucket = lastbucket;

	avg1 = avg1 / (double)(nsamples - 1);
	avg1 = avg1 * 3.0;

	count = 0;
	for (i = 0; i < nsamples - 1; i++)
	{
		if (AVG[i] < avg1)
		{
			avg2 = avg2 + AVG[i];
			count++;
		}
	}

	if ((count == 0) || (avg2 == 0))
	{
		return 1.0;
	}
	avg2 = avg2 / (double)count;
	avg2 = avg2 * 3.0;

	return (avg2);
}

/*
 * Resize computes a new calendar queue size, and copies the old calendar
 * queue into the new one. It is called when there are too many elements
 * in the calendar queue 
 */

static void
ReSize(tw_pq * p_ptr, int newsize)
{
	double          bwidth;
	int             i;
	int             oldnbuckets;
	tw_event_head  *oldbucket;
	tw_event_head  *temp;

	if (p_ptr->Nbuckets + newsize >= MAX_EVENTS_PER_PE)
	{
		printf
			("Warning: unable to resize calendar queue: performance may suffer. \n");
		printf("         ReSize being turned off \n");
		p_ptr->ResizeEnabled = 0;
		return;
	}
	bwidth = NewWidth(p_ptr);
	oldbucket = p_ptr->Bucket;
	oldnbuckets = p_ptr->Nbuckets;

	if (p_ptr->FirstSub == 0)
		LocalInit(p_ptr, MAX_EVENTS_PER_PE - newsize, newsize, bwidth,
				  p_ptr->LastPriority);
	else
		LocalInit(p_ptr, 0, newsize, bwidth, p_ptr->LastPriority);

	for (i = 0; i < oldnbuckets; i++)
	{
		while (oldbucket[i].next != NULL)
		{
			temp = (tw_event_head *) oldbucket[i].next;
			oldbucket[i].next = temp->next;
			if (temp->next != NULL)
				temp->next->prev = (tw_event *) & oldbucket[i];
			temp->next = NULL;
			temp->prev = NULL;
			tw_pq_enqueue(p_ptr, (tw_event *) temp);
		}
	}
}

/*
 * print calendar queue by going thru the buckets in turn 
 */

/*
static void
DumpBucket(tw_pq * p_ptr, FILE * fp)
{
	int             i;
	int             tot_events = 0;
	tw_event       *probe;

	fprintf(fp, "*----------- Dumping Buckets --------------*\n");
	fprintf(fp, "  ST==Send Time, RT==Receive Time\n");
	fprintf(fp, "  SLP==Source LP, DLP==Destination LP, P==Processed?\n");
	fprintf(fp, "*------------------------------------------------*\n");
	for (i = 0; i < p_ptr->Nbuckets; i++)
	{
		int             nevents = 0;

		probe = p_ptr->Bucket[i].next;
		fprintf(fp, "Bucket[%d]:\n", i);
		while (1)
		{
			if (probe == NULL)
			{
				fprintf(fp, "\tNULL\n");
				fprintf(fp, "#events[%d]: %d\n", i, nevents);
				break;
			} else
			{
				fprintf(fp, "\t{ RT=%5.3f, SLP=%d, DLP=%d, P=%c }\n",
						probe->recv_ts, probe->src_lp->id,
					  probe->dest_lp->id, probe->state.committed ? 'T' : 'F');
				probe = probe->next;
				nevents++;
				tot_events++;
			}
		}
	}
	fprintf(fp, "Total events on PE = %d\n", tot_events);
	fprintf(fp, "*----------- Finished Dumping --------------*\n");
}
*/

/*
 * tw_pq_create:
 *
 * init queue to size of 2, and a width of 1.0 per bucket 
 */
tw_pq *
tw_pq_create(void)
{
	tw_pq *q;

	MAX_EVENTS_PER_PE = g_tw_events_per_pe << 1;

	q = tw_calloc(TW_LOC, "calendar queue", sizeof(tw_pq), 1);
	q->OrigBucket = tw_calloc(
		TW_LOC,
		"calendar queue buckets",
		sizeof(tw_event_head),
		MAX_EVENTS_PER_PE);

	LocalInit(q, 0, 2, 1.0, 0.0);
	q->ResizeEnabled = 1;
	return (void *)q;
}

/*
 * Find the smallest element in the calq. Useful for debugging 
 */

/*
static double
Smallest(tw_pq * p_ptr)
{
	int             i;
	tw_event       *probe;
	double          smallest = DBL_MAX;
	int             cnt = 0;

	for (i = 0; i < p_ptr->Nbuckets; i++)
	{
		probe = p_ptr->Bucket[i].next;
		while (1)
		{
			if (probe == NULL)
			{
				break;
			} else
			{
				if (probe->recv_ts < smallest)
					smallest = probe->recv_ts;
				probe = probe->next;
				cnt++;
			}
		}
	}
	if (cnt != p_ptr->Qsize)
		printf("Wrong Qsize, Qsize=%d, found %d\n", p_ptr->Qsize, cnt);
	fflush(stdout);

	return (smallest);
}
*/

/*
 * Same as dequeue. But the smallest timestamped event is not deleted. 
 * Find minimum element in the queue 
 */
tw_stime
tw_pq_minimum(tw_pq *p_ptr)
{
	int             i, smallest;
	double          buckettop;

	buckettop = p_ptr->BucketTop;
	if (p_ptr->Qsize == 0)
	{
		return (DBL_MAX);
	}
	for (i = p_ptr->LastBucket;;)
	{
		if ((p_ptr->Bucket[i].next != NULL) &&
			(p_ptr->Bucket[i].next->recv_ts < buckettop))
		{
			return (p_ptr->Bucket[i].next->recv_ts);
		} else
		{
			++i;
			if (i == p_ptr->Nbuckets)
				i = 0;
			buckettop = buckettop + p_ptr->Width;
			if (i == p_ptr->LastBucket)
				break;
		}
	}

	/*
	 * Start Direct Search 
	 */
	for (smallest = 0; p_ptr->Bucket[smallest].next == NULL; smallest++)
		if (smallest >= p_ptr->Nbuckets)
		{
			tw_error(TW_LOC, "Grave inconsistency in calendar queue:\n"
					 "Nbuckets = %d, Qsize = %d \n", p_ptr->Nbuckets,
					 p_ptr->Qsize);
		}
	for (i = smallest + 1; i < p_ptr->Nbuckets; i++)
	{
		if (p_ptr->Bucket[i].next == NULL)
			continue;
		else if
			(p_ptr->Bucket[i].next->recv_ts <
			 p_ptr->Bucket[smallest].next->recv_ts)
			smallest = i;
	}


	return (tw_stime) (p_ptr->Bucket[smallest].next->recv_ts);
}


/*
 * Delete one element from the queue 
 */
void
tw_pq_delete_any(tw_pq *p_ptr, tw_event *q)
{
	if (q->prev != NULL)
		q->prev->next = q->next;
	if (q->next != NULL)
		q->next->prev = q->prev;

	p_ptr->Qsize--;
	q->next = q->prev = NULL;
	q->state.owner = 0;
}

unsigned int
tw_pq_get_size(tw_pq *p_ptr)
{
	return (unsigned int)p_ptr->Qsize;
}
