#include "apachetop.h"

/* Timed_Circle class; stores all the hits in the last $X seconds.
 *
 * tab is a timed_circle_struct pointer, which we malloc to be $X; thus each
 * struct is one second.
 *
 * tab[x].hits is a hit_struct pointer, malloc'ed to roughly how many
 * hits we expect to see per second. This can be intelligently guessed from
 * past data where possible. Each struct within this is then one hit.
*/

#define MAX_BUCKET (bucketsize)

extern time_t now; /* global ApacheTop-wide to save on time() calls */
extern struct gstat gstats;

extern map *hm, *um, *rm;

int Timed_Circle::create(unsigned int size)
{
	/* increment size by 1, because one bucket is always being used for
	 * a transitional second; thus could be empty. So we always have the
	 * requested amount of data (or perhaps slightly more) */
	size++;

	/* malloc the buckets */
	tab = (struct timed_circle_struct *)
	    malloc(size * sizeof(struct timed_circle_struct));

	if (tab == NULL)
		return -1;

	this->initbuckets(0, size);

	bucketsize = size;
	bucketpos = 0; /* start at the beginning */

	walk_hitpos = walk_bucketpos = 0;

	return 0;
}

int Timed_Circle::initbuckets(const unsigned int from, const unsigned int to)
{
	unsigned int x;
	struct timed_circle_struct *ptr;

	/* clear some values to 0 */
	for(x = from, ptr = &tab[x] ; x < to; ++x, ++ptr)
	{
		/* set up hits array for this bucket */

		ptr->hitsize = 5;
		ptr->hits = (HIT *)malloc(sizeof(HIT) * ptr->hitsize);
		if (!ptr->hits) abort();

		memset(ptr->hits, 0, sizeof(HIT) * ptr->hitsize);

		this->resetbucketstats(x);
	}

	return 0;
}

void Timed_Circle::resetbucketstats(const unsigned int r)
{
	int x;
	HIT *hit;
	struct timed_circle_struct *bucket;
	//dprintf("resetting bucket stats %d\n", r);

	bucket = &tab[r];

	/* reset statistics for this bucket */
	bucket->time = now;
	bucket->bytecount = 0;
	bucket->reqcount = 0;

	bucket->rc_summary[2] = 0;
	bucket->rc_summary[3] = 0;
	bucket->rc_summary[4] = 0;
	bucket->rc_summary[5] = 0;


	/* free up any storage associated with the hit previously in this
	 * circle position. At the moment this is basically removing a
	 * refcount from the maps in it */
	for(x = 0 ; x < bucket->hitsize ; x++)
	{
		hit = &bucket->hits[x];
		if (hit->host_pos) hm->sub_ref(hit->host_pos);
		if (hit->url_pos) um->sub_ref(hit->url_pos);
		if (hit->ref_pos) rm->sub_ref(hit->ref_pos);
	}

	/* start at the beginning of the HIT array */
	bucket->hitpos = 0;

	/* and ensure they're all zero'ed */
	memset(bucket->hits, 0, sizeof(HIT) * bucket->hitsize);
}

int Timed_Circle::insert(struct logbits lb)
{
	short rc_tmp;

	HIT *hit;
	struct timed_circle_struct *bucket;

	/* if we're in a new second than the current bucket is for.. */
	// also updates bucketpos
	this->garbagecollection();

	/* for the relevant bucket for this second.. */
	bucket = &tab[bucketpos];

	/* see if we have a free slot */
	if (bucket->hitpos == bucket->hitsize)
	{
		/* we don't, increase the size of the hits array */
		bucket->hits = (HIT *)
		    realloc(bucket->hits, sizeof(HIT) * bucket->hitsize*2);
		if (!bucket->hits)
		{
			dprintf("realloc: %s\n", strerror(errno));
			abort(); /* FIXME: handle a bit better */
		}

		/* clear out the new hit positions */
		memset(&bucket->hits[bucket->hitpos],
		    0, sizeof(HIT) * bucket->hitsize);

		bucket->hitsize *= 2;
	}

	/* we do (now) */
	hit = &(bucket->hits[bucket->hitpos]);


	/* free up any storage associated with the hit previously
	 * in this circle position. At the moment this is basically
	 * removing a refcount from the maps in it */
	if (hit->host_pos) hm->sub_ref(hit->host_pos);
	if (hit->url_pos) um->sub_ref(hit->url_pos);
	if (hit->ref_pos) rm->sub_ref(hit->ref_pos);

	/* store the data itself */
	memcpy(hit, &lb, sizeof(lb));

	/* update stats for this bucket */
	bucket->reqcount++;
	bucket->bytecount += lb.bytes;
	rc_tmp = (int)lb.retcode/100;
	bucket->rc_summary[rc_tmp]++;

	/* ready for our next hitpos */
	bucket->hitpos++;

	return 0;
}

//int Timed_Circle::walk(unsigned int *url_pos, unsigned int *ip_pos,
//   int *bytes, time_t *time, unsigned int *ipl, unsigned int *retcode)
int Timed_Circle::walk(struct logbits **lb)
{
	struct timed_circle_struct *bucket;

	*lb = NULL;

	//dprintf("entering walk at bucket %d, hit %d\n", walk_bucketpos, walk_hitpos);

	/* see if we're at the end of this bucket */
	bucket = &tab[walk_bucketpos];
	if (walk_hitpos == bucket->hitpos)
	{
		//dprintf("end of bucket %d\n", walk_bucketpos);
		/* next bucket */
		++walk_bucketpos;
		walk_hitpos = 0;
	}

	/* see if we're out of buckets;
	 * bucketsize-1 is the highest we'll ever use */
	if (walk_bucketpos == MAX_BUCKET)
	{
		/* done */
		walk_bucketpos = 0;
		walk_hitpos = 0;
		return -1;
	}

	bucket = &tab[walk_bucketpos];

	/* anything in this bucket? */
	while (walk_bucketpos < MAX_BUCKET && bucket->hitpos == 0)
	{
		//dprintf("skipping bucket %d\n", walk_bucketpos);
		/* try the next one */
		++walk_bucketpos;
		walk_hitpos = 0;
		bucket = &tab[walk_bucketpos];
		//dprintf("skipped to bucket %d\n", walk_bucketpos);

		/* see if we're out of buckets;
		 * bucketsize-1 is the highest we'll ever use */
		if (walk_bucketpos == MAX_BUCKET)
		{
			/* done */
			walk_bucketpos = 0;
			walk_hitpos = 0;
			return -1;
		}
	}

	*lb = &(bucket->hits[walk_hitpos]);

	++walk_hitpos;

	return 0;
}

void Timed_Circle::updatestats(void)
{
	unsigned int x;
	struct timed_circle_struct *bucket;

	// clear any buckets between last hit and now
	this->garbagecollection();

	/* now update stats from all buckets with something in them */
	reqcount = 0; bytecount = 0;
	rc_summary[2] = rc_summary[3] = rc_summary[4] = rc_summary[5] = 0;

	for(x = 0, bucket = &tab[0] ; x < bucketsize ; ++x, ++bucket)
	{
		reqcount += bucket->reqcount;
		bytecount += bucket->bytecount;

		rc_summary[2] += bucket->rc_summary[2];
		rc_summary[3] += bucket->rc_summary[3];
		rc_summary[4] += bucket->rc_summary[4];
		rc_summary[5] += bucket->rc_summary[5]; 
	}
}

void Timed_Circle::garbagecollection(void)
{
	unsigned int x, prevbucket;

	/* clear out the hits from any buckets which we've passed since our
	 * last garbage collection. Basically, if a period of time passes in
	 * between hits, we may go from bucket 1 .. 5 and skip 2/3/4. We
	 * need to empty them here.
	*/

	if (tab[bucketpos].time < now)
	{ 
		prevbucket = bucketpos;
		bucketpos = (now - gstats.start) % bucketsize;

		if (bucketpos < prevbucket)
		{
			/* we've gone from end of array to start again */

			/* so first, 0 to wherever */
			for (x = 0 ; x <= bucketpos ; ++x)
				resetbucketstats(x);

			/* now from old position to end */
			for (x = prevbucket+1 ; x < MAX_BUCKET ; ++x)
				resetbucketstats(x);
		}
		else
		{
			/* simpler, just (eg) 7 to 9 */
			for(x = prevbucket+1 ; x <= bucketpos ; ++x)
				resetbucketstats(x);
		}

		// if we're overrunning, time to loop round
	//	if (bucketpos == bucketsize)
	//		bucketpos = 0; // back to first bucket
	}
}


time_t Timed_Circle::oldest(void)
{
	time_t t = now;
	unsigned int x;
	struct timed_circle_struct *ptr;

	/* this may take up some considerable time with a large circle? */
	for(x = 0, ptr = &tab[0] ; x < MAX_BUCKET ; ++x, ++ptr)
		t = getMIN(ptr->time, t);

	return t;
}
