#ifndef _DISPLAY_H_
#define _DISPLAY_H_

/* macro to render "paused" in inverse at the coords given. This is
 * called from apachetop.cc/read_key() when pause is activated, and in each
 * display.cc/draw_header() when pause is turned on.
*/
#define DRAW_PAUSED(x,y) attron(A_REVERSE); \
                         mvaddstr(x, y, "paused"); \
                         attroff(A_REVERSE)

#define SUBMENU_LINE_NUMBER LINES_RESERVED-2

/* display() makes an array of these, sorts, and displays */
struct itemlist {
	unsigned int hash;
	int item, ip_item;
	double reqcount;
	double bytecount;
	time_t first, last, timespan;
	float rps, kbps;

	struct hitinfo r_codes[6];
};

bool display(time_t last_display);

void display_header();

void display_list();
void display_sub_list(short display_mode_override,
    unsigned short offset, unsigned short limit);

void translate_screen_to_pos();

void drawMarker(void);

#define NO_INDENT 0
void show_map_line(struct itemlist *item_ptr, int vert_location, map *m,
    unsigned short indent, unsigned short number_mode);

void shellsort_wrapper(struct itemlist *items, unsigned int size,
    struct config pcf);
void shellsort(struct itemlist *items, unsigned int size, int sorttype);

float readableNum(double num, char *suffix);

void display_submenu_banner(char *title, int title_len, char *banner);
void clear_submenu_banner(void);

void display_help(void);
void display_active_filters(void);

void display_histogram();

#endif
