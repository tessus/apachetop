#ifndef _FILTERS_H_
#define _FILTERS_H_

/* Filter class
**
** Each instance has one filter, which is either plaintext or regular
** expression (if HAVE_PCRE2_H is defined): Quick example:
**
** f = new Filter();
** f->store("(movies|music)");
** if (f->isactive() && f->match("some string with movies in it"))
** 	// you got a match
*/

class Filter
{
public:
	Filter(void);
	~Filter(void);

	void store(const char *filter);
	bool isactive(void);
	bool match(const char *string);

	void empty(void);

private:
	bool regex_isvalid;
	char *filter_text;

#if HAVE_PCRE2_H
	RegEx *regexp;
#endif

};

#endif /* _FILTERS_H_ */
