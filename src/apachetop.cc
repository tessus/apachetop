/* APACHETOP
**
**
*/
#include "apachetop.h"

/* die and report why */
#define DIE(msg) fprintf(stderr, "%s: %s\n", msg, strerror(errno)); catchsig(1);
/* die with no strerror */
#define DIE_N(msg) fprintf(stderr, "%s\n", msg); catchsig(1);

#if HAVE_ADNS_H
/* resolver master state */
adns_state adns;
#endif

/* global stats */
struct gstat gstats;

time_t now;

struct itemlist *items = NULL;
map *last_display_map;

Circle *c;
Timed_Circle *tc;
Hits_Circle *hc;

map *um, /* urlmap */
    *im, /* ipmap */
    *hm, /* hostmap */
    *rm, /* referrermap */
    *fm; /* filemap */

struct config cf;

/* inputs */
struct input *in;

WINDOW *win;

#if (POLLING_METHOD == USING_KQUEUE)
int kq;
#elif (POLLING_METHOD == USING_FAM)
/* master fd for talking to FAM */
FAMConnection fc;
#endif

int main(int argc, char *argv[])
{
	int fd, buflen, ch, x;
	char buf[32768], *bufcp, *nextline;
	time_t last_display = 0;
	struct logbits lb;
	struct input *curfile;
	bool seen_h = false, seen_t = false;
	extern char *optarg;
#if (POLLING_METHOD == USING_KQUEUE)
	/* we have and are using kqueue */
	struct timespec stv;
	struct kevent *ev, *evptr;
#elif (POLLING_METHOD == USING_FAM)
	/* we have and are using FAM */
	int fam_fd;
	FAMEvent fe;
	struct timeval tv;
	tv.tv_sec = 1; tv.tv_usec = 0;
	fd_set readfds;

#else /* stat() then */
	struct stat sb;
	struct timeval tv;
	tv.tv_sec = 1; tv.tv_usec = 0;
#endif

	LogParser *p;
	CommonLogParser *cmmp; //AtopLogParser *atpp;


	/* some init stuff will need to know the time */
	now = time(NULL);

	/* set up initial configuration {{{ */
	memset(&cf, 0, sizeof(cf));
	cf.debug = true;
	cf.current_display_size = 0;
	cf.input_count = 0;
	cf.circle_size = DEFAULT_CIRCLE_SIZE;
	cf.circle_mode = DEFAULT_CIRCLE_MODE;
	cf.sort = DEFAULT_SORT;
	cf.retcodes_sort = DEFAULT_RETCODES_SORT;
	cf.refresh_delay = DEFAULT_REFRESH_DELAY;
	cf.display_mode = DEFAULT_DISPLAY_MODE;
	cf.numbers_mode = DEFAULT_NUMBERS_MODE;
	cf.detail_display_urls = true;
	cf.detail_display_hosts = true;
	cf.detail_display_refs = true;
	cf.display_paused = false;
	cf.keep_querystring = false;
	cf.lowercase_urls = false;
	cf.keep_segments = 0;
	cf.preserve_ref_protocol = 0;
	cf.do_immed_display = false;
	cf.do_resolving = false;
	cf.urlfilter = new Filter();
	cf.reffilter = new Filter();
	cf.hostfilter = new Filter();
	/* }}} */

	/* support MAX_INPUT_FILES input files */
	in = (struct input *)calloc(MAX_INPUT_FILES, sizeof(struct input));

#if (POLLING_METHOD == USING_KQUEUE) /* then create kqueue {{{ */
	if ((kq = kqueue()) < 0)
	{
		DIE("cannot create kqueue");
		exit(1);
	}

	/* space in ev for all inputs */
	ev = (struct kevent *)calloc(MAX_INPUT_FILES, sizeof(struct kevent));
/* }}} */
#elif (POLLING_METHOD == USING_FAM) /* open FAM connection */
	if (FAMOpen(&fc))
	{
		DIE("cannot connect to fam");
		exit(1);
	}

	fam_fd = FAMCONNECTION_GETFD(&fc);
#endif

#if HAVE_ADNS_H
	/* open adns connection */
	adns_init(&adns, adns_if_noerrprint, 0);
#endif

	/* process commandline {{{ */
	while ((ch = getopt(argc, argv, "f:H:T:hvqlrs:pd:")) != -1)
	{
		switch(ch)
		{
			case 'f':
				if (new_file(optarg, SEEK_TO_END) == -1)
				{
					fprintf(stderr, "opening %s: %s\n",
					        optarg, strerror(errno));
					sleep(2);
				}
				else
					cf.input_count++;
				break;
			case 'T':
				x = atoi(optarg);
				seen_t = true;
				if (x > 0)
				{
					cf.circle_mode = TIMED_CIRCLE;
					cf.circle_size = x;
				}
				break;
			case 'H':
				x = atoi(optarg);
				seen_h = true;
				if (x > 0)
				{
					cf.circle_mode = HITS_CIRCLE;
					cf.circle_size = x;
				}
				break;

			case 'q':
				cf.keep_querystring = true;
				break;

			case 'l':
				cf.lowercase_urls = true;
				break;

			case 'r':
				cf.do_resolving = true;
				break;

			case 's':
				x = atoi(optarg);
				if (x > 0)
					cf.keep_segments = x;
				break;

			case 'p':
				cf.preserve_ref_protocol = 1;
				break;

			case 'd':
				x = atoi(optarg);
				if (x > 0)
					cf.refresh_delay = x;
				break;

			case 'v':
				version();
				exit(0);
				break;

			case 'h':
			case '?':
				usage();
				exit(1);
				break;
		}
	} /* }}} */

	/* if no files have been specified, we'll use DEFAULT_LOGFILE */
	if (cf.input_count == 0)
	{
		if (new_file(DEFAULT_LOGFILE, SEEK_TO_END) != -1)
			cf.input_count++;

		/* if it's still zero, fail */
		if (cf.input_count == 0)
		{
			fprintf(stderr, "opening %s: %s\n",
			        DEFAULT_LOGFILE, strerror(errno));
			DIE_N("No input files could be opened");
			exit(1);
		}
	}

	if (seen_t && seen_h)
	{
		DIE_N("-T and -H are mutually exclusive. Specify only one.");
		exit(1);
	}


	/* set up circle */
	switch(cf.circle_mode)
	{
		default:
		case HITS_CIRCLE:
			hc = new Hits_Circle;
			hc->create(cf.circle_size);
			c = hc;
			break;

		case TIMED_CIRCLE:
			tc = new Timed_Circle;
			tc->create(cf.circle_size);
			c = tc;
			break;
	}

	/* set up one of each parser */
	/* CommonLogParser handles combined too */
	cmmp = new CommonLogParser;
#if 0
/* this isn't written yet */
	atpp = new AtopLogParser;
#endif

	/* maps auto-resize, just use a sane starting point {{{ */
	/* url string -> url hash map */
	um = new map; /* hashing happens internally */
	um->create(cf.circle_size);

	/* referrer string -> hash map */
	rm = new map;
	rm->create(cf.circle_size);

	/* ip string -> ip hash map */
	im = new map;
	im->create(cf.circle_size);

	/* host string -> host hash map */
	hm = new map;
	hm->create(cf.circle_size);
	/* }}} */

	memset(&gstats, 0, sizeof(gstats));
	gstats.start = time(NULL);

	signal(SIGINT, &catchsig);
	signal(SIGKILL, &catchsig);
	signal(SIGWINCH, &catchwinch);

	win = initscr();
	nonl(); /* no NL->CR/NL on output */
	cbreak(); /* enable reading chars one at a time */
	noecho();
	keypad(win, true);

	nodelay(win, true);

	for( ;; )
	{
		if (cf.exit) break;

		now = time(NULL);

		/* periodically check that we're not in a submenu, and we
		 * haven't been for more than 5 seconds without any more
		 * keypresses; but if cf.in_submenu_stay is true, we stay in
		 * the submenu until we receive a keypress (in read_key) */
		if (cf.in_submenu &&
		    !cf.in_submenu_stay &&
		    now - cf.in_submenu_time > 5)
		{
			cf.in_submenu = SUBMENU_NONE;
			clear_submenu_banner();
		}

		/* if we have any input files waiting to be opened, try to */
		for (x = 0 ; x < MAX_INPUT_FILES ; ++x)
		{
			/* for each entry in input .. */
			curfile = &in[x];
			if (curfile->inode && curfile->open == false)
			{
				dprintf("%s needs reopening\n",
				   curfile->filename);

				new_file(curfile->filename, NO_SEEK_TO_END);

				/* what happens if a new fd is used? the old
				 * struct will still have inode > 0 and
				 * open == false so we'll keep coming back
				 * here */
			}
		}


#if HAVE_ADNS_H
		/* see if adns has got anything for us */
		collect_dns_responses();
#endif

		/* see if it's time to update.. */
		if (display(last_display))
		{
			/* returns true when we updated, so.. */
			last_display = now; // remember our last update
		}

		/* start of for() loop within these folds.. */
#if (POLLING_METHOD == USING_KQUEUE) /* {{{ */
		/* every 1/10th of a second */
		stv.tv_sec = 0; stv.tv_nsec = 10000000;
		x = kevent(kq, NULL, 0, ev, MAX_INPUT_FILES, &stv);
//		if (x == 0) continue; /* timeout, nothing happened */
		if (x < 0) break; /* error */

		for(evptr = ev ; evptr->filter ; evptr++)
		{
			/* udata contains a pointer to the struct input
			** array element for this fd
			*/
			curfile = (struct input *)evptr->udata;
			fd = curfile->fd; /* or evptr->ident will work */

			/* see if we can deduce what happened */
			if (((evptr->fflags & NOTE_DELETE) == NOTE_DELETE) ||
			    ((evptr->fflags & NOTE_RENAME) == NOTE_RENAME))
			{
				/* file deleted or renamed */
				close(fd);
				curfile->open = false;

				/* skip rest of loop, we'll try
				 * to reopen at top of for(;;) */
				continue;
			}
			else if ((evptr->fflags & NOTE_WRITE) == NOTE_WRITE)
			{
				//read_amt = ev.data; /* we don't use this */
			}
/* }}} */
#elif (POLLING_METHOD == USING_FAM) /* {{{ */
		FD_ZERO(&readfds);
		FD_SET(fam_fd, &readfds);

		/* 1/10th of a second */
		tv.tv_sec = 0;
		tv.tv_usec = 100000;

		/* perform a select() on the fam filedescriptor */
		select(fam_fd + 1, &readfds, NULL, NULL, &tv);

		/* if FAM has nothing to report, loop around */
		if (!(FD_ISSET(fam_fd, &readfds)))
			continue;

		while (FAMPending(&fc) == 1 && FAMNextEvent(&fc, &fe))
		{
			/* fe.userdata is struct input * element for this
			** file; see new_file() for how it's set
			*/
			curfile = (struct input *)fe.userdata;
			fd = curfile->fd;

/* }}} */
#else /* fallback to stat() {{{ */

		/* 1/10th of a second */
		tv.tv_sec = 0; tv.tv_usec = 100000;
		select(0, NULL, NULL, NULL, &tv);

		/* check every file */
		for (x = 0 ; x < MAX_INPUT_FILES ; ++x)
		{
			curfile = &in[x];
			fd = curfile->fd;

			if (curfile->open == false || fd == 0)
				continue;

			if (stat(curfile->filename, &sb) == -1)
			{
				/* file removed */
				close(fd);
				curfile->fd = 0;
				curfile->open = false;

				/* skip rest of loop, we'll try
				 * to reopen at top of for(;;) */
				continue;
			}
			else
			{
				/* file still there, but lets check the
				 * inode hasn't changed (ie, it's been
				 * recreated under our feet */
				if (sb.st_ino != curfile->inode)
				{
					close(fd);
					curfile->open = false;

					/* skip rest of loop, we'll try
					 * to reopen at top of for(;;) */
					continue;
				}
			}
#endif /* }}} */

			/* read the data */
			buflen = read(fd, buf, sizeof(buf));

			if (buflen == 0) /* no data */
			{
				/* this should only happen if we've had to
				 * fall back to stat() */
				continue;
			}

			if (buflen < 0) /* error */
			{
				DIE("read");
			}

			curfile->lastreq = now;

			/* pass each line to logsplit; FIXME tidy this up */
			nextline = bufcp = buf;
			while ((nextline - buf) < buflen && nextline)
			{
				bufcp = nextline;
				/* find the end of this line */
				if (!(nextline = strchr(bufcp, '\n')))
				{
					/* we can't, ignore it */
					continue;
				}

				*nextline = '\0';
				++nextline;

				/* which parser? */
				#if CHRIS_HAS_WRITTEN_MORE_PARSERS
				switch(in->type)
				{
					/* only one atm */
					default:
						p = cmmp;
						break;
				}
				#else
				p = cmmp;
				#endif

				if (p->parse(bufcp, &lb) == 0)
				{
					/* record which file the log is from */
					lb.fileid = fd;

					/* insert into circle */
					c->insert(lb);

					/* record stats */
					recordstats(lb);
				}
				// next line..

			} /* while ((nl - buf) < buflen && nl) */
		} /* for(evptr = ev ; evptr->filter ; evptr++) */
		  /* while (FAMPending(&fc) == 1 && FAMNextEvent(&fc, &fe)) */
		  /* for (i = 0 ; i < MAX_INPUT_FILES ; ++i) */

		/* check for keypresses */
		if ((ch = wgetch(win)) != ERR)
			read_key(ch);

	} /* for( ;; ) */

	delete cmmp;
	endwin();

	return 0;
}

int recordstats(struct logbits l) /* {{{ */
{
	int t;

	/* global stats */
	if (gstats.alltime.first == 0) gstats.alltime.first = now;
	gstats.alltime.last = now;
	gstats.alltime.reqcount++;
	gstats.alltime.bytecount += l.bytes;

	/* for this return code, increment */
	t = (int)(l.retcode/100);
	gstats.r_codes[t].reqcount++;
	gstats.r_codes[t].bytecount += l.bytes;

	return 0;
} /* }}} */

int read_key(int ch) /* {{{ */
{
#define SUBMENU_SORT_HB_TITLE "sort by.."
#define SUBMENU_SORT_HB_BANNER "r) REQUESTS  R) REQS/SEC  b) BYTES  B) BYTES/SEC"
#define SUBMENU_SORT_RC_TITLE "sort by.."
#define SUBMENU_SORT_RC_BANNER "2) 2xx   3) 3xx   4) 4xx   5) 5xx"

#define SUBMENU_DISP_TITLE "toggle subdisplay.."
#define SUBMENU_DISP_BANNER "u) URLS  r) REFERRERS  h) HOSTS"

#define SUBMENU_FILT_TITLE "filters.."
#define SUBMENU_FILT_BANNER "a) add/edit menu  c) clear all  s) show active"

#define SUBMENU_FILT_ADD_TITLE "filters: add.."
#define SUBMENU_FILT_ADD_BANNER "u) to URLS  r) to REFERRERS  h) to HOSTS"

	/* got a keypress, best process it */

	/* check submenu state; in no submenu we do the master list.. */
	if (cf.in_submenu == SUBMENU_NONE) /* {{{ */
	    switch(ch)
	{
		case 's': /* sort .. submenus */
			cf.in_submenu_time = now;
			cf.in_submenu_stay = false;

			switch(cf.numbers_mode)
			{
				default:
				case NUMBERS_HITS_BYTES:
					cf.in_submenu = SUBMENU_SORT_HB;
					/* display banner to aid further
					 * presses */
					display_submenu_banner(
					    SUBMENU_SORT_HB_TITLE,
					    sizeof(SUBMENU_SORT_HB_TITLE),
					    SUBMENU_SORT_HB_BANNER);
					break;

				case NUMBERS_RETCODES:
					cf.in_submenu = SUBMENU_SORT_RC;
					/* display banner to aid further
					 * presses */
					display_submenu_banner(
					    SUBMENU_SORT_RC_TITLE,
					    sizeof(SUBMENU_SORT_RC_TITLE),
					    SUBMENU_SORT_RC_BANNER);
					break;
			}

			break;

		case 't': /* toggle displays */
			cf.in_submenu = SUBMENU_DISP;
			cf.in_submenu_time = now;
			cf.in_submenu_stay = false;

			/* display banner to aid further presses */
			display_submenu_banner(SUBMENU_DISP_TITLE,
			    sizeof(SUBMENU_DISP_TITLE), SUBMENU_DISP_BANNER);
			break;

		case 'f': /* filter submenu */
			cf.in_submenu = SUBMENU_FILT;
			cf.in_submenu_time = now;
			cf.in_submenu_stay = false;

			/* display banner to aid further presses */
			display_submenu_banner(SUBMENU_FILT_TITLE,
			    sizeof(SUBMENU_FILT_TITLE), SUBMENU_FILT_BANNER);

			break;

		case 'h': /* enter help; this is a special submenu */
		case '?':
			cf.in_submenu = SUBMENU_HELP;
			cf.in_submenu_time = now;

			/* stay in here till next keypress */
			cf.in_submenu_stay = true;

			/* don't blat it */
			cf.display_paused = true;

			display_help();

			break;

		case 'p': /* (un)pause display */
			cf.display_paused = !cf.display_paused;
			if (cf.display_paused)
			{
				/* tell the user we're paused */
				DRAW_PAUSED(0,60); /* macro in display.h */
				refresh();
			}
			else
			{
				/* turning off pause forces an update */
				cf.do_immed_display = true;
			}
			break;

		case ' ':
			cf.do_immed_display = true;
			break;

#if 0
		case 's': /* new refresh delay */
#ifndef SOLARIS /* linking against readline is failing on Solaris atm */
			endwin();
			char *t;
			int nd;
			t = readline("Seconds to Delay: ");
			if ((nd = atoi(t)) != 0)
				cf.refresh_delay = nd;

			refresh();
#endif
			break;
#endif
		case 'd': /* display mode; urls or hosts */
			switch(cf.display_mode)
			{
				case DISPLAY_URLS:
					cf.display_mode = DISPLAY_HOSTS;
					break;
				case DISPLAY_HOSTS:
					cf.display_mode = DISPLAY_REFS;
					break;
				case DISPLAY_REFS:
					cf.display_mode = DISPLAY_URLS;
					break;
			}
			cf.do_immed_display = true;
			break;

		case 'n': /* numbers mode; hits/bytes or returncodes */
			switch(cf.numbers_mode)
			{
				case NUMBERS_HITS_BYTES:
					cf.numbers_mode = NUMBERS_RETCODES;
					break;
				case NUMBERS_RETCODES:
					cf.numbers_mode = NUMBERS_HITS_BYTES;
					break;
			}
			cf.do_immed_display = true;
			break;

		case KEY_UP:
			if (cf.display_mode != DISPLAY_DETAIL)
			{
				/* sanity checking for this is done
				 * in drawMarker() */
				cf.selected_item_screen--;
				drawMarker();
				translate_screen_to_pos();
			}
			break;
#if 0
		case KEY_END:
		case KEY_NPAGE:
			if (cf.display_mode != DISPLAY_DETAIL)
			{
				cf.selected_item_screen = 9999;
				drawMarker();
				translate_screen_to_pos();
			}
			break;
#endif
		case KEY_DOWN:
			if (cf.display_mode != DISPLAY_DETAIL)
			{
				/* sanity checking for this is done
				 * in drawMarker() */
				cf.selected_item_screen++;
				drawMarker();
				translate_screen_to_pos();
			}
			break;

		case KEY_RIGHT:
			if (cf.display_mode != DISPLAY_DETAIL)
			{
				/* enter detailed display mode */
				cf.display_mode_detail = cf.display_mode;
				cf.display_mode = DISPLAY_DETAIL;
				cf.do_immed_display = true;
			}
			break;

		case KEY_LEFT:
			if (cf.display_mode == DISPLAY_DETAIL)
			{
				/* leave detailed display mode */
				cf.display_mode = cf.display_mode_detail;
				cf.do_immed_display = true;
			}
			break;

		case KEY_REFRESH:
			refresh();
			break;

		case KEY_BREAK:
		case 'q':
			cf.exit = true;
			break;
	} /* }}} */

	else if (cf.in_submenu == SUBMENU_SORT_HB) /* {{{ */
	{
		switch(ch)
		{
			case 'r':
				cf.sort = SORT_REQCOUNT;
				break;

			case 'R':
				cf.sort = SORT_REQPERSEC;
				break;

			case 'b':
				cf.sort = SORT_BYTECOUNT;
				break;

			case 'B':
				cf.sort = SORT_BYTESPERSEC;
				break;

		}
		cf.in_submenu = SUBMENU_NONE;
		clear_submenu_banner();
		cf.do_immed_display = true;
	} /* }}} */
	else if (cf.in_submenu == SUBMENU_SORT_RC) /* {{{ */
	{
		switch(ch)
		{
			case '2':
				cf.retcodes_sort = SORT_2XX;
				break;
			case '3':
				cf.retcodes_sort = SORT_3XX;
				break;
			case '4':
				cf.retcodes_sort = SORT_4XX;
				break;
			case '5':
				cf.retcodes_sort = SORT_5XX;
				break;
		}
		cf.in_submenu = SUBMENU_NONE;
		clear_submenu_banner();
		cf.do_immed_display = true;
	} /* }}} */

	else if (cf.in_submenu == SUBMENU_DISP) /* {{{ */
	{
		switch(ch)
		{
			case 'u':
				cf.detail_display_urls = !cf.detail_display_urls;
				break;

			case 'r':
				cf.detail_display_refs = !cf.detail_display_refs;
				break;

			case 'h':
			case 'i':
				cf.detail_display_hosts = !cf.detail_display_hosts;
				break;

		}
		cf.in_submenu = SUBMENU_NONE;
		clear_submenu_banner();
		cf.do_immed_display = true;
	} /* }}} */

	else if (cf.in_submenu == SUBMENU_FILT) /* {{{ */
	    switch(ch)
	{
		default: /* default action is to back out */
			cf.in_submenu = SUBMENU_NONE;
			clear_submenu_banner();
			cf.do_immed_display = true;
			break;

		case 'a': /* add filter.. */
			/* clean away existing menu */
			clear_submenu_banner();

			cf.in_submenu = SUBMENU_FILT_ADD;
			cf.in_submenu_time = now;
			cf.in_submenu_stay = false;

			/* display banner to aid further presses */
			display_submenu_banner(SUBMENU_FILT_ADD_TITLE,
			    sizeof(SUBMENU_FILT_ADD_TITLE),
			    SUBMENU_FILT_ADD_BANNER);

			break;

		case 'c': /* clear all filters */
			cf.urlfilter->empty();
			cf.hostfilter->empty();
			cf.reffilter->empty();

			cf.in_submenu = SUBMENU_NONE;
			clear_submenu_banner();
			cf.do_immed_display = true;
			break;

		case 's': /* show filter page */

			cf.in_submenu = SUBMENU_FILT_SHOW;
			cf.in_submenu_time = now;

			/* stay in here till next keypress */
			cf.in_submenu_stay = true;

			/* don't blat it */
			cf.display_paused = true;

			/* and render the page; similar to display_help();
			 * this calls functions in the Filter class to do
			 * its work */
			display_active_filters();


			break;

	} /* }}} */
	else if (cf.in_submenu == SUBMENU_FILT_ADD) /* {{{ */
	{
		char *input = NULL;

		/* check the keypress was valid .. */
		switch(ch)
		{
			case 'u': case 'r': case 'h':
				/* it was */
				break;

			default:
				cf.in_submenu = SUBMENU_NONE;
				cf.in_submenu_stay = false;
				clear_submenu_banner();
				cf.do_immed_display = true;
				return 0;
				break;
		}

		/* get an expression */

		/* do not leave until readline is done */
		cf.in_submenu_stay = true;
		/* do not update the display, because we're endwin()'ed */
		cf.display_paused = true;

		endwin();

		input = readline("Filter: ");

		/* back into curses mode */
		refresh();
		nonl(); /* no NL->CR/NL on output */
		cbreak(); /* enable reading chars one at a time */
		noecho();

		/* update again */
		cf.in_submenu = SUBMENU_NONE;
		cf.in_submenu_stay = false;
		cf.display_paused = false;
		cf.do_immed_display = true;

		if (!(input && *input))
		{
			return 0;
		}

		add_history(input);

		/* apply to the appropriate filter */
		switch(ch)
		{
			case 'u':
				cf.urlfilter->store(input);
				break;

			case 'h':
				cf.hostfilter->store(input);
				break;

			case 'r':
				cf.reffilter->store(input);
				break;
		}
		free(input);

	} /* }}} */

	/* special submenus which take over the entire screen */
	else if (cf.in_submenu == SUBMENU_HELP ||
	         cf.in_submenu == SUBMENU_FILT_SHOW)
	/* {{{ */
	{
		/* any key in these special "submenus" exits them */
		cf.in_submenu = SUBMENU_NONE;
		cf.in_submenu_stay = false;

		cf.display_paused = false;
		cf.do_immed_display = true;
	}
	/* }}} */

	return 0;
} /* }}} */


int new_file(const char *filename, bool do_seek_to_end) /* {{{ */
/* opens the filename supplied,
 * and fills in the struct input passed by reference
*/
{
	int i, fd, input_element = -1;
	struct stat sb;
	struct input *this_file;
	char realfile[MAXPATHLEN];

	/* if realpath cannot resolve the file, give up */
	if ((realpath(filename, realfile) == NULL))
		return -1;

#if (POLLING_METHOD == USING_KQUEUE)
	struct kevent kev;
#endif

	/* stat the item; ensure it's a file and get an inode */
	if (stat(realfile, &sb) == -1)
		return -1;

	/* we can't tail anything except a file */
//	if (sb.st_mode != S_IFREG)
//		return -1;

	/* choose where to put this input in the struct */
	/* if it has an existing slot, re-use */
	for(i = 0 ; i < cf.input_count ; ++i)
	{
		if ((strcmp(in[i].filename, filename)) == 0)
		{
			input_element = i;
			break;
		}
	}

	if (input_element == -1)
	{
		/* new open, make sure we have room in our arrays */
		if (cf.input_count == MAX_INPUT_FILES)
		{
			DIE_N("Only 50 files are supported at the moment");
		}

		/* add it on at the end */
		input_element = cf.input_count;
	}

	if ((fd = open(realfile, O_RDONLY)) == -1)
		return -1;

	this_file = &in[input_element];
	this_file->fd = fd;

	if (do_seek_to_end) lseek(fd, 0, SEEK_END);

	if (this_file->filename) free(this_file->filename);

	this_file->inode = sb.st_ino;
	this_file->filename = strdup(realfile);
	this_file->lastreq = now;
	this_file->type = LOG_COMMON; /* assumption */
	this_file->open = true;

#if (POLLING_METHOD == USING_KQUEUE)
	/* add into kqueue */
#ifdef __NetBSD__
	EV_SET(&kev, fd, EVFILT_VNODE,
	    EV_ADD | EV_ENABLE | EV_CLEAR,
	    NOTE_WRITE | NOTE_DELETE | NOTE_RENAME, 0, (intptr_t)this_file);
#else
	EV_SET(&kev, fd, EVFILT_VNODE,
	    EV_ADD | EV_ENABLE | EV_CLEAR,
	    NOTE_WRITE | NOTE_DELETE | NOTE_RENAME, 0, this_file);
#endif
	if (kevent(kq, &kev, 1, NULL, 0, NULL) < 0)
	{
		DIE("cannot create kevent");
	}
#elif (POLLING_METHOD == USING_FAM)
	/* add into FAM */
	FAMMonitorFile(&fc, realfile, &this_file->famreq, this_file);
#endif

	return 0;
} /* }}} */

void version(void) /* {{{ */
{
	fprintf(stdout, "ApacheTop %s\n", PACKAGE_VERSION);
} /* }}} */

void usage(void) /* {{{ */
{
	fprintf(stderr,
	    "ApacheTop v%s - Usage:\n"
	    "File options:\n"
	    "  -f logfile  open logfile (assumed common/combined) [%s]\n"
	    "              (repeat option for more than one source)\n"
	    "\n"
	    "URL/host/referrer munging options:\n"
	    "  -q          keep query strings [%s]\n"
	    "  -l          lowercase all URLs [%s]\n"
	    "  -s num      keep num path segments of URL [all]\n"
	    "  -p          preserve protocol at front of referrers [%s]\n"
	    "  -r          resolve hostnames/IPs into each other [%s]\n"
	    "\n"
	    "Stats options:\n"
	    "  Supply up to one of the following two. default: [-%c %d]\n"
	    "  -H hits     remember stats for this many hits\n"
	    "  -T secs     remember stats for this many seconds\n"
	    "\n"
	    "  -d secs     refresh delay in seconds [%d]\n"
	    "\n"
	    "  -v          show version\n"
	    "  -h          this help\n"
	    "\n"
	    "Compile Options: %cHAVE_KQUEUE %cHAVE_FAM %cENABLE_PCRE\n"
	    "Polling Method: %s\n"
	    ,
	    PACKAGE_VERSION,
	    DEFAULT_LOGFILE,
	    /* cf is set by the time we get to usage, so we can use contents */
	    cf.keep_querystring ? "yes" : "no",
	    cf.lowercase_urls ? "yes" : "no",
	    cf.preserve_ref_protocol ? "yes" : "no",
	    cf.do_resolving ? "yes" : "no",
	    cf.circle_mode, cf.circle_size,
	    cf.refresh_delay,
#if HAVE_KQUEUE /* {{{ */
	    '+',
#else
	    '-',
#endif /* }}} */
#if HAVE_FAM_H /* {{{ */
	    '+',
#else
	    '-',
#endif /* }}} */
#if HAVE_PCRE_H /* {{{ */
	    '+',
#else
	    '-',
#endif /* }}} */
	    (POLLING_METHOD == USING_KQUEUE ? "kqueue" :
	        (POLLING_METHOD == USING_FAM ? "fam" :
	            "stat"
		)
	    )

	    );

	return;
} /* }}} */

int dprintf(const char *fmt, ...) /* {{{ */
{
	FILE *d;
        va_list args;
        static char fileName[1024] = {'\0'};

       	if ( !strlen( fileName ) )
      	{
        	strcpy( fileName, "/tmp/atop.XXXXXX" );
        	mkdtemp( fileName );
        	strncat( fileName, "/debug", sizeof(fileName) - strlen(fileName) - 1 );
       	}

        if (cf.debug && (d = fopen(fileName, "a")))
        {
                va_start(args, fmt);
                vfprintf(d, fmt, args);
		fclose(d);
        	va_end(args);
	}

        return 0;
} /* }}} */

static void catchsig(int s) /* {{{ */
{
	cf.exit = s;
} /* }}} */

/* handle a window resize by simply reopening and redrawing our window */
static void catchwinch(int s)
{
	endwin();
	refresh();
	cf.do_immed_display = true;
}
