#ifndef _HITS_CIRCLE_H_
#define _HITS_CIRCLE_H_

class Hits_Circle : public Circle
{
public:
	int create(unsigned int passed_size);
	int insert(struct logbits lb);
	int walk(struct logbits **lb);

	void updatestats(void) {}
	time_t oldest(void);

	double getreqcount(void) { return reqcount; }
	double getbytecount(void) { return bytecount; }
	double getsummary(int r_c) { return rc_summary[r_c]; }

private:
	int resize(int newsize);

	double reqcount, bytecount;
	double rc_summary[6];

	typedef struct logbits circle_struct;
	circle_struct *tab;
	int size; /* total size of circle table */
	int pos; /* where are we now? */

	int walkpos;
};

#endif
