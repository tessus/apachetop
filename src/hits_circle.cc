/* class to encapsulate set of functions to manage a circular array;
 * recent hit information is stored in the looping array; each hit
 * has information written into the next slot of structs. If
 * we reach the end, start over. Then the top-URL/IP is summed up from
 * this information. The bigger the table, the more hits you'll have
 * to summarise from.
*/

#include "apachetop.h"

extern map *hm, *um, *rm;

int Hits_Circle::create(unsigned int passed_size)
{
	size = passed_size;
	pos = 0;
	walkpos = 0;

	tab = (circle_struct *)
	    calloc(size, sizeof( circle_struct));
	if (!tab)
	{
		abort();
	}

	reqcount = bytecount = 0;
	memset(rc_summary, (char) NULL, sizeof(rc_summary));

	return 0;
}

int Hits_Circle::insert(struct logbits lb)
{
	circle_struct *posptr;
	short rc_tmp_old, rc_tmp_new;

	/* insert the given data into the current position,
	 * and update pos to point at next position */
	posptr = &tab[pos];

	if (posptr->time == 0)
		/* if this is a new insert, increment our count */
		++reqcount;
	else
	{
		/* if this is re-using an old slot, remove refcount for the
		 * previous data before we vape it */
		hm->sub_ref(posptr->host_pos);
		um->sub_ref(posptr->url_pos);
		rm->sub_ref(posptr->ref_pos);

	}
	
	/* maintain some stats */
	/* bytecount; remove the previous one and add the new one */
	bytecount -= posptr->bytes;
	bytecount += lb.bytes;
	/* retcodes, remember how many we have of each */
	rc_tmp_old = (int)posptr->retcode/100;
	rc_tmp_new = (int)lb.retcode/100;
	if (rc_tmp_old != rc_tmp_new)
	{
		--rc_summary[rc_tmp_old];
		++rc_summary[rc_tmp_new];
	}

	/* store the data */
	memcpy(posptr, &lb, sizeof(lb));

	++pos;

	/* see if we're running out of space. We'd like to keep however many
	 * hits of data the user has asked for; if we're not managing
	 * that, increase */
	if (pos == size)
	{
		/* loop round */
		pos = 0;
	}

	return 0;
}

//int Hits_Circle::walk(unsigned int *url_pos, unsigned int *ip_pos,
//    int *bytes, time_t *time, unsigned int *ipl, unsigned int *retcode)
int Hits_Circle::walk(struct logbits **lb)
{
	/* return each value in the circle one by one, starting at 0 and
	 * working up; return 0 when there are more to go, or -1 when we're
	 * done */

	*lb = NULL;

	if (walkpos == size || tab[walkpos].time == 0)
	{
		walkpos = 0;
		return -1;
	}

	*lb = &tab[walkpos];

	++walkpos;
	return 0;
}

time_t Hits_Circle::oldest(void)
{
	int tmp;

	/* return the first entry we have. normally this will be pos+1, but
	 * cater for circumstances where it is isn't; ie we're initially
	 * filling up the array (use 0), or we're at position size (use 0) */
	if (pos == size)
		tmp = 0; /* earliest will be 0 */
	else
		tmp = pos + 1; /* earliest is next element */

	if (tab[tmp].time > 0)
		return tab[tmp].time;

	return tab[0].time;
}

