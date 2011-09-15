#ifndef _TIMED_CIRCLE_H_
#define _TIMED_CIRCLE_H_

/* store hits for a given second in here.
 * some bits of the logbits struct aren't used in the circle but
 * it's better than making an entirely new struct */
typedef struct logbits HIT;

class Timed_Circle : public Circle
{
public:

	int create(unsigned int size);
	int insert(struct logbits lb);
	int walk(struct logbits **lb);

	void updatestats(void);
	time_t oldest(void);

	double getreqcount(void) { return reqcount; }
	double getbytecount(void) { return bytecount; }
	double getsummary(int r_c) { return rc_summary[r_c]; }

private:
	int initbuckets(const unsigned int from, const unsigned int to);
	void resetbucketstats(const unsigned int r);

	void garbagecollection(void);

	double reqcount, bytecount;
	double rc_summary[6];

	unsigned int bucketsize; /* how many buckets (seconds) ? */
	unsigned int bucketpos; /* which bucket are we using now? */

	/* the timed_circle_struct is a set of buckets; each bucket contains
	 * data about hits for a given second. Each hit is stored in a
	 * HIT array inside the relevant bucket */
	struct timed_circle_struct
	{
		time_t time; /* second this bucket represents */

		/* stats for the HITs array */
		double reqcount, bytecount;
		double rc_summary[6];

		unsigned int hitsize; /* how big is the array for hits? */
		unsigned int hitpos; /* how far along hits array we are */
		HIT *hits; /* hits for this second go into array */

	} *tab;

	/* for walk() */
	unsigned int walk_bucketpos;
	unsigned int walk_hitpos;

};

#endif
