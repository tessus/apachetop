#include "apachetop.h"

Filter::Filter(void)
{
#if HAVE_PCRE_H_ 
	regexp = NULL;
#endif
	filter_text = NULL;
}


Filter::~Filter(void)
{
	this->empty();
}


void Filter::store(const char *filter)
{
	if (filter_text) free(filter_text);
	filter_text = strdup(filter);

#if HAVE_PCRE_H_
	if (regexp) delete regexp;
	regexp = new RegEx(filter);
#endif

	return;
}


bool Filter::isactive()
{
	return filter_text ? true : false;
}


bool Filter::match(const char *string)
{
	if (!filter_text)
		return false;

#if HAVE_PCRE_H_
	return regexp->Search(string);
#else
	return strstr(string, filter_text);
#endif
}


void Filter::empty(void)
{
#if HAVE_PCRE_H_ 
	if (regexp) delete regexp;
	regexp = NULL;
#endif
	if (filter_text) free(filter_text);
	filter_text = NULL;
}
