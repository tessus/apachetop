#include "apachetop.h"

#include "inlines.cc"

extern struct gstat gstats;
extern time_t now;

extern Circle *c;

extern map *um, /* urlmap */
           *im, /* ipmap */
           *hm, /* hostmap */
           *rm, /* referrermap */
           *fm; /* filemap */

extern struct config cf;

/* global, so we can keep it after display() finishes; the only reason for
 * this so far is so we know what's being displayed after this function
 * finishes. Hence we can translate selected_item_screen into a
 * selected_item_pos when the marker moves */
extern itemlist *items;
extern map *last_display_map;


bool display(time_t last_display) /* {{{ */
{
	struct gstat *w_gstats;
	Circle *w_c;

	if (cf.do_immed_display)
	{
		/* we've been asked to update immediately, whether we're
		 * paused or not */

		/* this flag enforces a clear() (why? forgot now) */
		clear();

		/* clear that flag */
		cf.do_immed_display = false;
	}
	else if (
	    /* do nothing if display is paused */
	    cf.display_paused
	    ||
	    /* or it's not time to update yet */
	    (now - last_display) < cf.refresh_delay  )
		return false;

	/* if we reach here, we're fine to update */

	display_header();

	//display_histogram();
	//return true;

	if (cf.display_mode != DISPLAY_DETAIL)
	{
		/* normal display */
		display_list();

		return true;
	}

	/* detailed display mode involves showing the item the user is
	 * interested in (complete stats, whether it be an URL or whatever),
	 * followed by alternate stats. Example. The user is interested in
	 * one particular URL, so we show that URL as it would be in
	 * DISPLAY_URLS, then split the rest of the screen between IPs
	 * hitting that URL, and referrers referring to that URL */

	/* first line is the pos of interest */
	display_list();

	/* now split the screen with all other stats */

	/* one line for header, one for main stat */
	#define FIRST_OFFSET 2

	/* figure out how much room we have */
	int size, cur_offset, num_sections;
	cur_offset = FIRST_OFFSET;

	switch(cf.display_mode_detail)
	{
		case DISPLAY_URLS:
			/* show IPs and Referrers */
			num_sections =
			    (cf.detail_display_hosts ? 1 : 0) +
			    (cf.detail_display_refs ? 1 : 0);

			/* anything to do? */
			if (num_sections == 0) break;

			size = (((LINES-LINES_RESERVED-2) - FIRST_OFFSET) /
			    num_sections);

			if (cf.detail_display_hosts)
			{
				display_sub_list(DISPLAY_HOSTS,cur_offset,size);
				cur_offset += size + 1;
			}

			if (cf.detail_display_refs)
			{
				display_sub_list(DISPLAY_REFS,cur_offset,size);
				cur_offset += size + 1;
			}
			break;


		case DISPLAY_HOSTS:
			/* show URLs and Referrers */
			num_sections =
			    (cf.detail_display_urls ? 1 : 0) +
			    (cf.detail_display_refs ? 1 : 0);

			/* anything to do? */
			if (num_sections == 0) break;

			size = (((LINES-LINES_RESERVED-2) - FIRST_OFFSET) /
			    num_sections);

			if (cf.detail_display_urls)
			{
				display_sub_list(DISPLAY_URLS,cur_offset,size);
				cur_offset += size + 1;
			}
			if (cf.detail_display_refs)
			{
				display_sub_list(DISPLAY_REFS,cur_offset,size);
				cur_offset += size + 1;
			}
			break;


		case DISPLAY_REFS:
			/* show URLs and IPs */
			num_sections =
			    (cf.detail_display_urls ? 1 : 0) +
			    (cf.detail_display_hosts ? 1 : 0);

			/* anything to do? */
			if (num_sections == 0) break;

			size = (((LINES-LINES_RESERVED-2) - FIRST_OFFSET) /
			    num_sections);

			if (cf.detail_display_urls)
			{
				display_sub_list(DISPLAY_URLS,cur_offset,size);
				cur_offset += size + 1;
			}
			if (cf.detail_display_hosts)
			{
				display_sub_list(DISPLAY_HOSTS,cur_offset,size);
				cur_offset += size + 1;
			}
			break;

		case DISPLAY_FILES:
			/* show URLs and IPs */
			num_sections =
			    (cf.detail_display_urls ? 1 : 0) +
			    (cf.detail_display_hosts ? 1 : 0) +
			    (cf.detail_display_refs ? 1 : 0);

			/* anything to do? */
			if (num_sections == 0) break;

			size = (((LINES-LINES_RESERVED-2) - FIRST_OFFSET) /
			    num_sections);

			if (cf.detail_display_urls)
			{
				display_sub_list(DISPLAY_URLS,cur_offset,size);
				cur_offset += size + 1;
			}
			if (cf.detail_display_hosts)
			{
				display_sub_list(DISPLAY_HOSTS,cur_offset,size);
				cur_offset += size + 1;
			}
			if (cf.detail_display_refs)
			{
				display_sub_list(DISPLAY_REFS,cur_offset,size);
				cur_offset += size + 1;
			}
			break;
	}
	refresh();

	return true;
} /* }}} */

void display_header() /* {{{ */
{
	int itmp;
	float bytes, bps, per_req, ftmp;
	unsigned int secs_offset, diff, d = 0, h = 0, m = 0, s = 0;
	char bytes_suffix, bps_suffix, per_req_suffix;


	move(0, 0);
	clrtoeol();

	/* last hit */
	secs_offset = gstats.alltime.last % 86400;
	mvprintw(0, 0, "last hit: %02d:%02d:%02d",
	  secs_offset / 3600, (secs_offset / 60) % 60, secs_offset % 60);

	/* uptime */
	diff = (unsigned int)difftime(now, gstats.start);
	if (diff > 86399) diff -= ((d = diff / 86400)*86400);
	if (diff > 3599) diff -= ((h = diff / 3600)*3600);
	if (diff > 59) diff -= ((m = diff / 60)*60);
	s = diff;
	mvprintw(0, 27, "atop runtime: %2d days, %02d:%02d:%02d", d, h, m, s);

	/* are we paused? */
	if (cf.display_paused)
	{
		DRAW_PAUSED(0,60); /* macro in display.h */
	}

	/* current time */
	secs_offset = now % 86400;
	mvprintw(0, 71, "%02d:%02d:%02d",
	  secs_offset /3600, (secs_offset/ 60) % 60, secs_offset % 60);


	//All: 1,140,532 requests (39.45/sec),  999,540,593 bytes (857,235/sec)
	ftmp = getMAX(now-gstats.alltime.first, 1); /* divide-by-zero hack */
	bytes = readableNum(gstats.alltime.bytecount, &bytes_suffix);
	bps = readableNum(gstats.alltime.bytecount/ftmp, &bps_suffix);
	per_req = readableNum(
	    gstats.alltime.bytecount/getMAX(gstats.alltime.reqcount, 1),
	    &per_req_suffix);
	attron(A_BOLD);
	mvprintw(1, 0,
	  "All: %12.0f reqs (%6.1f/sec)  %11.1f%c (%7.1f%c/sec)   %7.1f%c/req",
	  gstats.alltime.reqcount,
	  gstats.alltime.reqcount/ftmp,
	  bytes, bytes_suffix,
	  bps, bps_suffix,
	  per_req, per_req_suffix);
	attroff(A_BOLD);


	// 2xx 1,604,104 (95%) 3xx 1,000,000 ( 3%) 4xx 1,000,000 ( 1%)
	// 5xx 1,000,000 ( 1%)
	ftmp = gstats.r_codes[2].reqcount + gstats.r_codes[3].reqcount+
	       gstats.r_codes[4].reqcount + gstats.r_codes[5].reqcount;
	if (ftmp == 0) ftmp = 1; /* avoid NaN with no hits */
	mvprintw(2, 0,
	 "2xx: %7.0f (%4.*f%%) 3xx: %7.0f (%4.*f%%) "
	 "4xx: %5.0f (%4.*f%%) 5xx: %5.0f (%4.*f%%) ",

	 gstats.r_codes[2].reqcount,
	 (gstats.r_codes[2].reqcount/ftmp) == 1 ? 0 : 1,
	 (gstats.r_codes[2].reqcount/ftmp)*100,

	 gstats.r_codes[3].reqcount,
	 (gstats.r_codes[3].reqcount/ftmp) == 1 ? 0 : 1,
	 (gstats.r_codes[3].reqcount/ftmp)*100,

	 gstats.r_codes[4].reqcount,
	 (gstats.r_codes[4].reqcount/ftmp) == 1 ? 0 : 1,
	 (gstats.r_codes[4].reqcount/ftmp)*100,

	 gstats.r_codes[5].reqcount,
	 (gstats.r_codes[5].reqcount/ftmp) == 1 ? 0 : 1,
	 (gstats.r_codes[5].reqcount/ftmp)*100

	);

	/* housecleaning on the circle, if its required in this class */
	c->updatestats();
	/* fetch the time of the first "recent" request */
	itmp = now - c->oldest();
	itmp = getMAX(itmp, 1); /* divide-by-zero hack */
	bytes = readableNum(c->getbytecount(), &bytes_suffix);
	bps = readableNum(c->getbytecount()/itmp, &bps_suffix);
	per_req = readableNum(c->getbytecount()/getMAX(c->getreqcount(), 1),
	    &per_req_suffix);
	attron(A_BOLD);
	mvprintw(3, 0,
	  "R (%3ds): %7.0f reqs (%6.1f/sec)  %11.1f%c (%7.1f%c/sec)   %7.1f%c/req",
	  itmp, c->getreqcount(),
	  ((float)c->getreqcount()/itmp),
	  bytes, bytes_suffix,
	  bps, bps_suffix,
	  per_req, per_req_suffix
	);
	attroff(A_BOLD);

	ftmp = c->getsummary(2) + c->getsummary(3) +
	       c->getsummary(4) + c->getsummary(5);
	if (ftmp == 0) ftmp = 1; /* avoid NaN with no hits */
	mvprintw(4, 0,
	  "2xx: %7.0f (%4.*f%%) 3xx: %7.0f (%4.*f%%) "
	  "4xx: %5.0f (%4.*f%%) 5xx: %5.0f (%4.*f%%) ",
	 c->getsummary(2),
	 (c->getsummary(2)/ftmp) == 1 ? 0 : 1,
	 (c->getsummary(2)/ftmp)*100,

	 c->getsummary(3),
	 (c->getsummary(3)/ftmp) == 1 ? 0 : 1,
	 (c->getsummary(3)/ftmp)*100,

	 c->getsummary(4),
	 (c->getsummary(4)/ftmp) == 1 ? 0 : 1,
	 (c->getsummary(4)/ftmp)*100,

	 c->getsummary(5),
	 (c->getsummary(5)/ftmp) == 1 ? 0 : 1,
	 (c->getsummary(5)/ftmp)*100
	);

//	mvprintw(5, 0,
//	    "Unique Objects:         Size Footprint:");

	/* if any filters are active, and the user is not in a submenu,
	 * display a summary */
	if (cf.in_submenu == SUBMENU_NONE)
	{
		int y;
		char active_filters[20];
		bzero(active_filters, sizeof(active_filters));

		/* suss out which filters are active */
		if (cf.urlfilter->isactive())
			strcat(active_filters, "URLs ");
		if (cf.hostfilter->isactive())
			strcat(active_filters, "HOSTs ");
		if (cf.reffilter->isactive())
			strcat(active_filters, "REFs ");

		if (*active_filters)
		{
			y = COLS - (strlen(active_filters) + 11);
			mvprintw(SUBMENU_LINE_NUMBER,
			         y, "Filtering: %s", active_filters);
		}

	}

} /* }}} */

void display_list() /* {{{ */
{
	int x, t, xx;
	char *cptmp;
	struct config scf; /* for copying cf, see comment below at memcpy */

	OAHash item_hash;
	int item_used = 0, disp;
	double items_size;
	struct itemlist *item_ptr = NULL;

	/* for easy reference during walk() */
	map *map; unsigned int hash;
	int pos;

	/* walk() pointer */
	struct logbits *lb;

	/* make an array containing all we have, then sort it */
	items_size = c->getreqcount();
	if (items_size == 0)
	{
		/* nothing to do! */
		move(LINES_RESERVED-1, 0);
		clrtobot();
		refresh();
		return;
	}

	item_hash.create( (int)items_size * 5 );

	/* this is our array; the cast is because items_size is a double but
	 * I'm fairly sure, realistically, it'll never get high enough to be
	 * a problem, so uInt should be ok */
	if (items) free(items); /* get rid of the last one */
	items = (struct itemlist *)
	    calloc((unsigned int)items_size, sizeof(itemlist));

	/* another thread may change the contents of cf while we're running,
	 * and it would be undesirable to have most of this change on us, so
	 * we make safe copies to use */
	memcpy(&scf, &cf, sizeof(struct config));

	/* if we are in detailed display mode, then we have to overwrite
	 * scf.display_mode so all these switch()'es work */
	if (scf.display_mode == DISPLAY_DETAIL)
	    scf.display_mode = cf.display_mode_detail;

	/* pick a map to use */
	switch(scf.display_mode)
	{
		default:
		case DISPLAY_URLS: map = um; break;
		case DISPLAY_HOSTS: map = hm; break;
		case DISPLAY_REFS: map = rm; break;
		case DISPLAY_FILES: map = fm; break;
	}

	/* store that in a global that we can remember */
	last_display_map = map;

	/* walk the entire circle */
	while(c->walk(&lb) != -1)
	{
		/* skip unused */
		if (lb == NULL)
			continue;


		/* set up some pointers, depending on cf.display_mode, so
		 * that we can refer to the url_pos or ip_pos via just pos,
		 * and the urlmap or ipmap via just map
		*/
		switch(scf.display_mode)
		{
			default:
			case DISPLAY_URLS:
				hash = lb->url_hash; pos = lb->url_pos;
				break;

			case DISPLAY_HOSTS:
				hash = lb->host_hash; pos = lb->host_pos;
				break;

			case DISPLAY_REFS:
				hash = lb->ref_hash; pos = lb->ref_pos;
				break;
#if HAVE_FILE_MODE_DISPLAY
			case DISPLAY_FILES:
				hash = lb->file_hash ; pos = lb->file_pos;
				break;
#endif
		}

		/* FILTERS {{{ */
		/* skip this item if it doesn't match our filter */
		if (cf.urlfilter->isactive() &&
		    !cf.urlfilter->match(um->reverse(lb->url_pos)))
			continue;

		if (cf.hostfilter->isactive() &&
		    !cf.hostfilter->match(hm->reverse(lb->host_pos)))
			continue;

		if (cf.reffilter->isactive() &&
		    !cf.reffilter->match(rm->reverse(lb->ref_pos)))
			continue;
		/* }}} */

		/* look up whatever string this pos is for */
		cptmp = map->reverse(pos);

		/* then lookup that string in items */
		item_ptr = (struct itemlist *)item_hash.lookup(cptmp);


		/* do we have this string already? */
		if (item_ptr == NULL)
		{
			/* not seen it, make a new slot */
			item_ptr = &items[item_used];

			item_hash.insert(cptmp, item_ptr);
			item_ptr->item = pos;

			/* store the ip position in the itemlist too, just
			** in case we have to look it up in show_map_line
			*/
			item_ptr->ip_item = lb->ip_pos;

			item_ptr->hash = hash;

			++item_used;
			item_ptr->last = now;
			item_ptr->first = lb->time;
		}

		/* we have the string in items now, so update stats in array */
		item_ptr->reqcount++;
		item_ptr->bytecount += lb->bytes;

		/* we wish to count up how many times a given retcode
		 * has occurred for this item */
		t = (int)(lb->retcode/100);
		item_ptr->r_codes[t].reqcount++;
		item_ptr->r_codes[t].bytecount += lb->bytes;

		if (lb->time < item_ptr->first)
			item_ptr->first = lb->time;
	}

	/* no further use for this hash */
	item_hash.destroy();

	/* calculate some stats; timespan, kbps, rps */
	for(x = 0 ; x < item_used ; ++x)
	{
		item_ptr = &items[x];

		item_ptr->timespan = item_ptr->last - item_ptr->first;
		if (item_ptr->timespan == 0)
			item_ptr->timespan = 1; /* hack to avoid /0 */

		for(xx = 2 ; xx < 6 ; xx++)
		{
			item_ptr->r_codes[xx].rps = ((float)
			    item_ptr->r_codes[xx].reqcount/item_ptr->timespan);

			item_ptr->r_codes[xx].bps = ((float)
			    item_ptr->r_codes[xx].bytecount/item_ptr->timespan);
		}

		item_ptr->kbps = ((float)
		    (item_ptr->bytecount/1024)/item_ptr->timespan);
		item_ptr->rps = ((float)
		    item_ptr->reqcount/item_ptr->timespan);
	}

	move(SUBMENU_LINE_NUMBER+1, 0);
	clrtobot();

	/* a suitable header, at LINES_RESERVED-1
	 * (-1 because curses starts at zero, but I've started at 1) */
	switch(scf.numbers_mode)
	{
		case NUMBERS_HITS_BYTES:
		mvaddstr(LINES_RESERVED-1, 0, " REQS REQ/S    KB KB/S");
		break;

		case NUMBERS_RETCODES:
		mvaddstr(LINES_RESERVED-1, 0, "  2xx   3xx   4xx  5xx");
		break;
	}

	/* what are we showing in the table? we have a few options;
	 *
	 * top $X URLS or IPs/Hosts or Referrers
	 * detailed referrer stats for a given URL
	*/
	if (cf.display_mode == DISPLAY_DETAIL)
	{
		/* detailed referrer stats for a given URL */
	//	mvaddstr(LINES_RESERVED-1, 0, "REQS REQ/S    KB  KB/S");

		/* display only the item we're interested in */
		for (x = 0 ; x < item_used ; ++x)
		{
			item_ptr = &items[x];
			if (item_ptr->hash == cf.selected_item_hash)
			{
				/* show stats for this line only */
				show_map_line(item_ptr, 0, map,
				    NO_INDENT, scf.numbers_mode);
				break;
			}
		}
		/* and return, display_sub_list can do the rest */
		return;
	}

	/* top $X items; sort the array for display */
	if (item_used) shellsort_wrapper(items, item_used, scf);

	/* display something */

	switch(scf.display_mode)
	{
		case DISPLAY_URLS:
		mvaddstr(LINES_RESERVED-1, 23, "URL");
		break;

		case DISPLAY_HOSTS:
		mvaddstr(LINES_RESERVED-1, 23, "HOST");
		break;

		case DISPLAY_REFS:
		mvaddstr(LINES_RESERVED-1, 23, "REFERRER");
		break;

		case DISPLAY_FILES:
		mvaddstr(LINES_RESERVED-1, 23, "FILE");
		break;
	}

	disp = 0; /* count how many we've shown */
	for(x = 0, item_ptr = &items[0] ;
	    x < item_used &&  disp < LINES-LINES_RESERVED-1 ;
	    ++item_ptr, ++x)
	{
		/* skip empty tablespaces, even though none should exist */
		if (item_ptr->reqcount == 0)
			continue;

		/* render the line itself at position disp. map should be
		 * already set from earlier in this function */
		show_map_line(item_ptr, disp, map,
		    NO_INDENT, scf.numbers_mode);

		++disp;
	}

	/* translate the screen position (cf.selected_item_screen) into an
	 * url/ip/ref_pos (cf.selected_item_pos), depending on current
	 * display mode. We can then use selected_item_pos to get info for
	 * the selected item if the user hits return to get it */
	translate_screen_to_pos();

#if 0
	/* debug line */
	mvprintw(5,0, "%s", map->reverse(cf.selected_item_pos));
#endif

	cf.current_display_size = disp;

	/* if there were items, draw a marker */
	if (cf.current_display_size)
		drawMarker();
	else
	{
		/* come to rest just above the column headers */
		move(LINES_RESERVED-2,0);
		refresh();
	}
} /* }}} */

void display_sub_list(short display_mode_override, /* {{{ */
    unsigned short offset, unsigned short limit)
{
	/* similar to display_list(), but this one just uses a section of
	 * the screen for displaying information pertinent to a particular
	 * URL or IP or REFERRER.
	*/

	/* I'd like to lose this code and replace the breakdown screen
	 * with something more useful.. */

	int x, t, xx;
	char *cptmp;
	unsigned int h;
	struct config scf; /* for copying cf, see comment below at memcpy */

	OAHash item_hash;
	int item_used = 0, disp;
	double items_size;
	struct itemlist *subitems, *item_ptr = NULL;

	/* for easy reference during walk() */
	map *map; unsigned int pos;

	/* walk() pointer */
	struct logbits *lb;

	if (c->getreqcount() == 0)
	{
		/* nothing to do! */
		refresh();
		return;
	}

	/* make an array containing all we have, then sort it */
	items_size = c->getreqcount();
	item_hash.create( (int)items_size * 5 );

	/* this is our array; the cast is because items_size is a double but
	 * I'm fairly sure, realistically, it'll never get high enough to be
	 * a problem, so uInt should be ok */
	//if (items) free(items); /* get rid of the last one */
	subitems = (struct itemlist *)
	    calloc((unsigned int)items_size, sizeof(itemlist));

	/* another thread may change the contents of cf while we're running,
	 * and it would be undesirable to have most of this change on us, so
	 * we make safe copies to use */
	memcpy(&scf, &cf, sizeof(struct config));

	/* pick a map to use */
	switch(display_mode_override)
	{
		default:
		case DISPLAY_URLS: map = um; break;
		case DISPLAY_HOSTS: map = hm; break;
		case DISPLAY_REFS: map = rm; break;
		case DISPLAY_FILES: map = fm; break;
	}

	/* walk the entire circle */
	while(c->walk(&lb) != -1)
	{
		/* skip unused */
		if (lb == NULL)
			continue;

		/* FILTERS? */


		/* set up some pointers, depending on cf.display_mode, so
		 * that we can refer to the url_pos or ip_pos via just pos,
		 * and the urlmap or ipmap via just map
		*/
		switch(scf.display_mode_detail)
		{
			default:
			case DISPLAY_URLS: h = lb->url_hash; break;
			case DISPLAY_HOSTS: h = lb->host_hash; break;
			case DISPLAY_REFS: h = lb->ref_hash; break;
			case DISPLAY_FILES: h = lb->file_hash; break;
		}

		/* we're only interested in this circle item if it matches
		 * the url/ip/referrer we're interested in; remember this is
		 * a sub-list pertinent to one particular master item */
		if (h != cf.selected_item_hash)
			continue;

		switch(display_mode_override)
		{
			default:
			case DISPLAY_URLS: pos = lb->url_pos; break;
			case DISPLAY_HOSTS: pos = lb->host_pos; break;
			case DISPLAY_REFS: pos = lb->ref_pos; break;
			case DISPLAY_FILES: pos = lb->file_pos; break;
		}

		/* look up whatever string this pos is for */
		cptmp = map->reverse(pos);

		/* then lookup that string in items */
		item_ptr = (struct itemlist *)item_hash.lookup(cptmp);

		/* do we have this string already? */
		if (item_ptr == NULL)
		{
			/* not seen it, make a new slot */
			item_ptr = &subitems[item_used];

			item_hash.insert(cptmp, item_ptr);
			item_ptr->item = pos;

			/* store the ip position in the itemlist too, just
			** in case we have to look it up in show_map_line
			*/
			item_ptr->ip_item = lb->ip_pos;

			item_ptr->last = now;
			item_ptr->first = lb->time;

			++item_used;
		}

		/* we have the string in items now, so update stats in array */
		++(item_ptr->reqcount);
		item_ptr->bytecount += lb->bytes;

		/* we wish to count up how many times a given retcode
		 * has occurred for this item */
		t = (int)(lb->retcode/100);
		item_ptr->r_codes[t].reqcount++;
		item_ptr->r_codes[t].bytecount += lb->bytes;

		if (lb->time < item_ptr->first)
			item_ptr->first = lb->time;
	}

	/* no further use for this lookup hash */
	item_hash.destroy();

	/* calculate some stats; timespan, kbps, rps */
	for(x = 0 ; x < item_used ; ++x)
	{
		item_ptr = &subitems[x];

		item_ptr->timespan = item_ptr->last - item_ptr->first;
		if (item_ptr->timespan == 0)
			item_ptr->timespan = 1; /* hack to avoid /0 */

		for(xx = 2 ; xx < 6 ; xx++)
		{
			item_ptr->r_codes[xx].rps = ((float)
			    item_ptr->r_codes[xx].reqcount/item_ptr->timespan);

			item_ptr->r_codes[xx].bps = ((float)
			    item_ptr->r_codes[xx].bytecount/item_ptr->timespan);
		}

		item_ptr->kbps = ((float)
		    (item_ptr->bytecount/1024)/item_ptr->timespan);
		item_ptr->rps = ((float)
		    item_ptr->reqcount/item_ptr->timespan);
	}

	/* top $X items; sort the array for display */
	if (item_used) shellsort_wrapper(subitems, item_used, scf);


	//clrtobot();

	/* display something */

	/* a suitable header, at offset
	 * (-1 because curses starts at zero, but I've started at 1) */
	switch(display_mode_override)
	{
		case DISPLAY_URLS:
			mvaddstr(LINES_RESERVED+offset-1, 23, "URL");
			break;

		case DISPLAY_HOSTS:
			mvaddstr(LINES_RESERVED+offset-1, 23, "HOST");
			break;

		case DISPLAY_REFS:
			mvaddstr(LINES_RESERVED+offset-1, 23, "REFERRER");
			break;

		case DISPLAY_FILES:
			mvaddstr(LINES_RESERVED+offset-1, 23, "FILE");
			break;
	}

	disp = offset; /* count how many we've shown */
	for(x = 0, item_ptr = &subitems[0] ;
	    x < items_size  &&  disp < limit+offset ; ++item_ptr, ++x)
	{
		/* skip empty tablespaces, even though none should exist */
		if (item_ptr->reqcount == 0)
			continue;

		/* render the line itself at position disp. map should be
		 * already set from earlier in this function */
		show_map_line(item_ptr, disp, map, 2, scf.numbers_mode);

		++disp;
	}
	free(subitems);
} /* }}} */

void translate_screen_to_pos() /* {{{ */
{
	/* don't do anything if there's nothing on screen */
	if (items == NULL) return;

	/* convert cf.selected_item_screen to cf.selected_item_pos */
	cf.selected_item_pos = items[cf.selected_item_screen].item;

//	/* cf.selected_item_pos may well be zero */
//	if (cf.selected_item_pos)
//	{
		/* make a hash of it */
		cf.selected_item_hash =
	 	   TTHash(last_display_map->reverse(cf.selected_item_pos));

		cf.selected_item_mode = cf.display_mode;
//	}
} /* }}} */

void drawMarker(void) /* update position of asterisk next to URLs {{{ */
{
	if (cf.current_display_size == 0)
		return;

	/* ensure our marker isn't beyond the end of the list */
	if (cf.selected_item_screen > cf.current_display_size-1)
		cf.selected_item_screen = cf.current_display_size-1;

	/* or above the start of it */
	if (cf.selected_item_screen < 0) cf.selected_item_screen = 0;

	/* draw an asterisk next to the selected item and clear adjacent lines*/
	mvaddch(LINES_RESERVED+cf.selected_item_screen-1, COLS_RESERVED-3, ' ');
	mvaddch(LINES_RESERVED+cf.selected_item_screen,
	    COLS_RESERVED-3, '*' | A_BOLD);
	mvaddch(LINES_RESERVED+cf.selected_item_screen+1, COLS_RESERVED-3, ' ');

	/* come to rest under header */
	move(SUBMENU_LINE_NUMBER, 0);

	refresh();
} /* }}} */


/* render contents of item_ptr into a statistics line onscreen,
 * at offset vert_location; pull the text portion out of m */
void show_map_line(struct itemlist *item_ptr, int vert_location, /* {{{ */
    map *m, unsigned short indent, unsigned short number_mode)
{
	if (number_mode == NUMBERS_HITS_BYTES)
	{
		/* start drawing at LINES_RESERVED */
		mvprintw(LINES_RESERVED+vert_location, 0,
		    "%5.0f %5.*f %5.*f %4.*f",
		    item_ptr->reqcount,

		    /* set 1dp if rps > 99, or 2 if not */
		    ((item_ptr->rps > 99) ? 1 : 2),
		    item_ptr->rps,

		    /* scale KB display; if >1000K lose the decimal point */
		    ((item_ptr->bytecount > (999*1024)) ? 0 : 1),
		    ((float)item_ptr->bytecount/1024),

		    /* scale KB/s display; if >1000K/s lose the decimal point */
		    ((item_ptr->kbps > 99) ? 0 : 1),
		    item_ptr->kbps);
	}
	else /* number_mode == NUMBERS_RETCODES */
	{
		mvprintw(LINES_RESERVED+vert_location, 0,
		    "%5.0f %5.0f %5.0f %4.0f",
		    item_ptr->r_codes[2].reqcount,
		    item_ptr->r_codes[3].reqcount,
		    item_ptr->r_codes[4].reqcount,
		    item_ptr->r_codes[5].reqcount);
	}

	/* text after the number columns */

	/* if we are displaying host/ip rather than anything else, we
	** have slightly more work to do; we need to generate a line of the
	** format host* [ip]
	** * only needs to be present if it's still resolving.
	** ip only needs to be present if list_show_ip is true.
	*/

	/* 23+indent = start at 23, but move to the right by indent
	** spaces if we are doing, for example, a sublist
	*/
	if (m == hm)
	{
		/* we are displaying a host line */
		char *h = NULL, *i = NULL;
		int host_item, ip_item;

		/* 200 chars ought to be enough? */
		#define MAX_IP_STR_WIDTH 200

		char str[MAX_IP_STR_WIDTH];

		host_item = item_ptr->item;
		ip_item = item_ptr->ip_item;

		if (host_item >= 0) h = m->reverse(host_item);
		if (ip_item >= 0) i = im->reverse(ip_item);

		if (h && i)
			snprintf(str, MAX_IP_STR_WIDTH, "%s [%s]", h, i);
		else if (h)
			snprintf(str, MAX_IP_STR_WIDTH, "%s", h);
		else if (i)
			snprintf(str, MAX_IP_STR_WIDTH, "[%s]", i);
		else
			return; /* shouldn't get reached */

		mvprintw(LINES_RESERVED+vert_location, 23+indent, "%.*s",
		    (COLS-COLS_RESERVED /* width */), str);
	}
	else
	{
		char *str = m->reverse(item_ptr->item);
		mvprintw(LINES_RESERVED+vert_location, 23+indent, "%.*s",
		    (COLS-COLS_RESERVED /* width */), str);
	}


} /* }}} */

void shellsort_wrapper(struct itemlist *items, unsigned int size, /* {{{ */
    struct config pcf)
{
	short sort_method;

	if (size == 0) return;

	/* what are we displaying in the numbers column? */
	switch(pcf.numbers_mode)
	{
		/* sorting by hits or bytes */
		case NUMBERS_HITS_BYTES:
			sort_method = pcf.sort;
			break;

		/* sorting by a return code */
		case NUMBERS_RETCODES:
			sort_method = pcf.retcodes_sort;
			break;
	}

	shellsort(items, size, sort_method);
} /* }}} */
void shellsort(struct itemlist *items, unsigned int size, int sorttype) /* {{{ */
{
	int x;
	bool done;
	unsigned int i, j, jmp = size;
	double i_c, j_c;

	struct itemlist tmp;

	while (jmp > 1)
	{
		jmp >>= 1;
		do
		{
			done = true;
			for (j = 0; j < (size-jmp); j++)
			{
				i = j + jmp;
				if (cf.numbers_mode == NUMBERS_HITS_BYTES)
				{
					/* numbers_mode = NUMBERS_HITS_BYTES */
					switch(sorttype)
					{
						default:
						case SORT_REQCOUNT:
						i_c = items[i].reqcount;
						j_c = items[j].reqcount;
						break;

						case SORT_REQPERSEC:
						i_c = items[i].rps;
						j_c = items[j].rps;
						break;

						case SORT_BYTECOUNT:
						i_c = items[i].bytecount;
						j_c = items[j].bytecount;
						break;

						case SORT_BYTESPERSEC:
						i_c = items[i].kbps;
						j_c = items[j].kbps;
						break;
					}
				}
				else
				{
					/* numbers_mode = NUMBERS_RETCODES */
					/* this is easier ;)
					 * see apachetop.h for explanation
					 * of what SORTTYPE_OFFSET_HACK
					 * is */
					x = sorttype-SORTTYPE_OFFSET_HACK;
					i_c = items[i].r_codes[x].reqcount;
					j_c = items[j].r_codes[x].reqcount;
				}

				if (i_c > j_c)
				{
#define P_S_S sizeof(struct itemlist)
					memcpy(&tmp, &items[i], P_S_S);
					memcpy(&items[i], &items[j], P_S_S);
					memcpy(&items[j], &tmp, P_S_S);

					done = false;
				}
			}
		} while (!done);
	}
} /* }}} */

float readableNum(double num, char *suffix) /* {{{ */
{
	#define AP_TEN_KB ((double)(1024)*10)
	#define AP_TEN_MB ((double)(1024*1024)*10)
	#define AP_TEN_GB ((double)(1024*1024*1024)*10)

	if (num > AP_TEN_GB)
	{
		*suffix = 'G';
		return (float)num/((double)(1024*1024*1024));
	}
	if (num > AP_TEN_MB)
	{
		*suffix = 'M';
		return (float)num/((double)(1024*1024));
	}
	if (num > AP_TEN_KB)
	{
		*suffix = 'K';
		return (float)num/1024;
	}

	*suffix = 'B';
	return (float)num;
} /* }}} */

void display_submenu_banner(const char *title, int title_len, const char *banner)
{
	attron(A_REVERSE);
	mvaddstr(SUBMENU_LINE_NUMBER, 1, title);
	attroff(A_REVERSE);
	mvaddstr(SUBMENU_LINE_NUMBER, 1+title_len, banner);
	move(SUBMENU_LINE_NUMBER, 0);
	refresh();
}

void clear_submenu_banner(void)
{
	/* remove submenu banner */
	move(SUBMENU_LINE_NUMBER, 0);
	clrtoeol();
	refresh();
}

void display_help(void)
{
	clear();

	move(0, 0);
	printw("ApacheTop version %s", PACKAGE_VERSION);
	move(0, 27);
	addstr("Copyright (c) 2003-2011 Chris Elsworth");
	move(1, 27);
	addstr("Copyright (c) 2015-     Helmut K. C. Tessarek");

	move(3, 0);
	addstr("ONE-TOUCH COMMANDS\n");
	addstr("d          : switch item display between urls/referrers/hosts\n");
	addstr("n          : switch numbers display between hits & bytes or return codes\n");
	addstr("h or ?     : this help window\n");
	addstr("p          : (un)pause display (freeze updates)\n");
	addstr("q          : quit ApacheTop\n");
	addstr("up/down    : move marker asterisk up/down\n");
	addstr("right/left : enter/exit detailed subdisplay mode\n");
	addstr("\n");
	addstr("SUBMENUS:\n");
	addstr("s:  SORT BY: [the appropriate menu will appear for your display]\n");
	addstr("\tr) requests  R) reqs/sec  b) bytes  B) bytes/sec\n");
	addstr("\t2) 2xx   3) 3xx   4) 4xx   5) 5xx\n\n");
	addstr("t:  TOGGLE SUBDISPLAYS ON/OFF:\n");
	addstr("\tu) urls  r) referrers  h) hosts\n\n");
	addstr("f:  MANIPULATE FILTERS:\n");
	addstr("\ta) add/edit menu c) clear all  s) show active (not done yet)\n");
	addstr("\ta:  ADD FILTER SUBMENU\n");
	addstr("\t\tu) to urls  r) to referrers  h) to hosts\n");
	addstr("\n");
	attron(A_REVERSE);
	addstr("Hit any key to continue:");
	attroff(A_REVERSE);

	refresh();
	cf.display_paused = true;
}

void display_active_filters()
{
	clear();

	move(0, 0);

	addstr("ApacheTop: Currently Active Filters\n");


	addstr("\n");
	attron(A_REVERSE);
	addstr("Hit any key to continue:");
	attroff(A_REVERSE);

	refresh();
	cf.display_paused = true;

}

void display_histogram()
{
	int hist_height, hist_width;
	int i, j, age, oldest;
	//int y_scale;
	float y_scale, y_decr;
	float x_scale, max_bar = 0;
	char horiz_line[128];

	struct logbits *lb;

	/* histogram starts at LINES_RESERVED+10 */
	#define HISTOGRAM_START LINES_RESERVED+3
	hist_height = 10;
	hist_width = 60;

	/* for every width character, figure out how many
	 * characters high to draw the barchart
	*/
	float bar_height[hist_width];
	char line[hist_width + 1];
	for(i = 0 ; i < hist_width ; i++)
		bar_height[i] = 0;

	/* figure out scales; includes divide-by-zero avoidance hack */
	if (cf.circle_mode == TIMED_CIRCLE)
		/* timed_circle = we know exactly how old we're going to get */
		x_scale = ((float)hist_width/cf.circle_size);
	else
		x_scale = ((float)hist_width/getMAX(now - c->oldest(), 1));

	/* don't scale when we have less data than we have room for */
	if (x_scale > 1) x_scale = 1;

	while(c->walk(&lb) != -1)
	{
		if (!lb)
			continue;

		/* we have hist_width bars, and we need to put the entire
		 * circle into that many bars. Devise which bar we're using
		 * for this particular lb->time */
		age = int( x_scale * (now - lb->time) );

		/* add on x_scale; this is because if we are displaying 2
		 * seconds worth of data in one line, we only want to add on
		 * half. */
		bar_height[age] += x_scale;
	}

	/* find the maximum bar height we have. */
	for(i = 0 ; i < hist_width ; ++i)
		max_bar = getMAX(max_bar, bar_height[i]);

	y_scale = max_bar;
	y_decr = ((float)y_scale / hist_height);

	for(i = 0 ; i < hist_height ; ++i)
	{
		if (i % 2 == 0)
			mvprintw(HISTOGRAM_START + i, 0, "%3.0f", y_scale);

		mvaddch(HISTOGRAM_START + i, 3, '|');

		/* compose a row of hashes */
		memset(line, ' ', hist_width);
		line[hist_width] = '\0';
		for(j = 0 ; j < hist_width ; ++j)
		{
			if (bar_height[j] > y_scale)
				line[j] = '#';
		}
		mvprintw(HISTOGRAM_START + i, 4, "%s", line);

		y_scale -= y_decr;
	}

	memset(horiz_line, '-', hist_width);
	horiz_line[hist_width] = '\0';
	mvprintw(HISTOGRAM_START + hist_height, 2, "0+%*s",
	    hist_width, horiz_line);

	mvprintw(HISTOGRAM_START + hist_height+1, 4, "NOW");
	mvprintw(HISTOGRAM_START + hist_height+1, hist_width+3, "-%ds",
	    now - c->oldest());

	refresh();
}
