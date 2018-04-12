#ifndef _APACHETOP_H_
#define _APACHETOP_H_

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#if HAVE_STRINGS_H
# include <strings.h>
#endif

#include <fcntl.h>

#include <signal.h>

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#if HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#if HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#if HAVE_NETDB_H
# include <netdb.h>
#endif

#if HAVE_PCRE_H
# include <pcre.h>
# include "pcre_cpp_wrapper.h"
#endif

#include <curses.h>

#include <readline/readline.h>
#include <readline/history.h>

/* Use kqueue in preference to anything else.
 *  If we don't have that, try FAM
 *   If we don't have FAM, fall back to stat()
*/
#define USING_KQUEUE 1
#define USING_FAM    2
#define USING_STAT   3
#if HAVE_KQUEUE
# include <sys/event.h>
# define POLLING_METHOD USING_KQUEUE
#elif HAVE_FAM_H
# include <fam.h>
# define POLLING_METHOD USING_FAM
#endif

/* stat() fallback */
#ifndef POLLING_METHOD
# define POLLING_METHOD USING_STAT
#endif

#if HAVE_ADNS_H
# include <adns.h>
#endif


#define getMIN(a,b) (a < b ? a : b)
#define getMAX(a,b) (a > b ? a : b)

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

/* last resort */
#ifndef MAXPATHLEN
# define MAXPATHLEN 1024
#endif

/* upon startup, each input file is put into an element of this array,
 * starting at 0. The struct under this maps the current fd into this array
 * so we can find it without iterating.
*/
struct input {
	char *filename;
	int fd;
	ino_t inode;
	short type; /* types defined in log.h */
	time_t lastreq;

	/* if open == false, then ApacheTop will periodically try to re-open
	 * this input file. */
	bool open; /* is the file open or not? */

#if (POLLING_METHOD == USING_FAM)
	FAMRequest famreq;
#endif
};

#include "filters.h"

struct config {
	#define SORT_REQCOUNT 1
	#define SORT_REQPERSEC 2
	#define SORT_BYTECOUNT 3
	#define SORT_BYTESPERSEC 4
	#define SORT_2XX 12 /* see display.cc around line 911 (shellsort)  */
	#define SORT_3XX 13 /* for the consequences of changing these.     */
	#define SORT_4XX 14 /* Logic there relies on them being            */
	#define SORT_5XX 15 /* (retcode/100)+SORTTYPE_OFFSET_HACK          */
	#define SORTTYPE_OFFSET_HACK 10
	short sort, retcodes_sort;
	short refresh_delay;

	short do_resolving;

	short selected_item_screen; /* screen position our marker is at */
	unsigned int selected_item_pos; /* which url/ip/refpos it relates to */
	short selected_item_mode; /* is it url/ip/ref pos? */
	unsigned int selected_item_hash;

	short current_display_size; /* how many lines we're displaying */

	short input_count;

	#define TIMED_CIRCLE 'T'
	#define HITS_CIRCLE  'H'
	int circle_mode;
	int circle_size;

	/* these defines are used in cf.filter too */
	#define DISPLAY_URLS   1
	#define DISPLAY_HOSTS  2
	#define DISPLAY_REFS   3
	#define DISPLAY_FILES  4
	#define DISPLAY_DETAIL 9
	short display_mode;

	#define NUMBERS_HITS_BYTES  1
	#define NUMBERS_RETCODES    2
	short numbers_mode;

	/* when in DISPLAY_DETAIL mode, display_mode is copied in here */
	short display_mode_detail;
	/* what would we like to display in DISPLAY_DETAIL modes? */
	bool detail_display_hosts, detail_display_refs, detail_display_urls;

	bool display_paused;
	bool do_immed_display; /* signals a screen update is req'd */

	/* filters */
	Filter *urlfilter, *hostfilter, *reffilter;

	/* url munging */
	bool keep_querystring;
	bool lowercase_urls;
	unsigned short keep_segments;
	bool preserve_ref_protocol;

	bool debug;

	bool exit; /* true when we want to finish */

	/* keypress submenu */
	#define SUBMENU_NONE          0

	#define SUBMENU_SORT_HB       1
	#define SUBMENU_SORT_RC       2

	#define SUBMENU_DISP          4

	#define SUBMENU_FILT          5
	#define SUBMENU_FILT_ADD      6
	#define SUBMENU_FILT_SHOW     7

	#define SUBMENU_HELP          9
	unsigned short in_submenu;
	bool in_submenu_stay; /* stay in submenu till keypress? */
	time_t in_submenu_time;

};

struct hitinfo {
	double bytecount;
	double reqcount;
	time_t first, last;

	/* for stats worked out in display.cc */
	float rps, bps;
};

struct gstat {
	time_t start; /* when did we start */

	/* space for 1xx-5xx return codes */
	struct hitinfo r_codes[6];

	/* space for counting global hits and bytes per sec */
	struct hitinfo alltime;
};

//#include "opt.h"
#include "ohtbl.h"

#if HAVE_ADNS_H
# include "resolver.h"
#endif

#include "map.h"
#include "circle.h"
#include "hits_circle.h"
#include "timed_circle.h"
#include "display.h"
#include "log.h"
#include "queue.h"


#define JAN 281
#define FEB 269
#define MAR 288
#define APR 291
#define MAY 295
#define JUN 301
#define JUL 299
#define AUG 285
#define SEP 296
#define OCT 294
#define NOV 307
#define DEC 268

#define DEBUG_OUTPUT "/tmp/atop.debug"

/* this can be overridden from config.h via ./configure --with-logfile .. */
#ifndef DEFAULT_LOGFILE
# define DEFAULT_LOGFILE "/var/log/access_log"
#endif
#define DEFAULT_CIRCLE_SIZE 30
#define DEFAULT_CIRCLE_MODE TIMED_CIRCLE
#define DEFAULT_SORT SORT_REQCOUNT
#define DEFAULT_RETCODES_SORT SORT_2XX
#define DEFAULT_REFRESH_DELAY 5
#define DEFAULT_DISPLAY_MODE DISPLAY_URLS
#define DEFAULT_NUMBERS_MODE NUMBERS_HITS_BYTES

/* if the layout of the display changes, these need updating */
#define COLS_RESERVED 25
#define LINES_RESERVED 7

#define MAX_INPUT_FILES 50

int recordstats(struct logbits l);

int read_key(int ch);

#define SEEK_TO_END true
#define NO_SEEK_TO_END false
int new_file(const char *filename, bool do_seek_to_end);

void usage(void);
void version(void);
int dprintf(const char *fmt, ...);

static void catchsig(int s);
static void catchwinch(int s);

#endif
