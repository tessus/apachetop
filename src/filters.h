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
	char *filter_text;

#if HAVE_PCRE2_H
	bool regex_isvalid;
	RegEx *regexp;
#endif

};

#endif /* _FILTERS_H_ */
