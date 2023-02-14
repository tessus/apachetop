#include "apachetop.h"

Filter::Filter(void)
{
#if HAVE_PCRE2_H
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

#if HAVE_PCRE2_H
	if (regexp) delete regexp;
	try
	{
		regexp = new RegEx(filter);
		regex_isvalid = true;
	}
	catch (int err)
	{
		regexp = NULL;
		regex_isvalid = false;
	}
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

#if HAVE_PCRE2_H
	if (regex_isvalid)
		return regexp->Search(string);
#endif
	return strstr(string, filter_text);
}


void Filter::empty(void)
{
#if HAVE_PCRE2_H
	if (regexp) delete regexp;
	regexp = NULL;
#endif
	if (filter_text) free(filter_text);
	filter_text = NULL;
}
