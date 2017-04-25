#ifndef _CIRCLE_H_
#define _CIRCLE_H_

class Circle
{
public:
	virtual int create(unsigned int size) = 0;

	virtual int insert(struct logbits lb) = 0;

	virtual int walk(struct logbits **lb) = 0;

    virtual time_t oldest(void) = 0;

	virtual void updatestats(void) = 0;

	virtual double getreqcount(void) = 0;
	virtual double getbytecount(void) = 0;
	virtual double getsummary(int r_c) = 0;
};

#endif
