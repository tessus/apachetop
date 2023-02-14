//
// Adapted for PCRE2 (c) 2023 Helmut K. C. Tessarek (tessarek@evermeet.cx)
// removed unnecessary methods (not needed by apachetop)
//
// regex.hpp 1.0 Copyright (c) 2003 Peter Petersen (pp@on-time.de)
// Simple C++ wrapper for PCRE
//
// This source file is freeware. You may use it for any purpose without
// restriction except that the copyright notice as the top of this file as
// well as this paragraph may not be removed or altered.
//
// This header file declares class RegEx, a simple and small API wrapper
// for PCRE.
//
// RegEx::RegEx(const char * regex, int options = 0)
//
//    The constructor's first parameter is the regular expression the
//    created object shall implement. Optional parameter options can be
//    any combination of PCRE options accepted by pcre_compile(). If
//    compiling the regular expression fails, an error message string is
//    thrown as an exception.
//
// RegEx::~RegEx()
//
//    The destructor frees all resources held by the RegEx object.
//
// int RegEx::SubStrings(void) const
//
//    Method SubStrings() returns the number of substrings defined by
//    the regular expression. The match of the entire expression is also
//    considered a substring, so the return value will always be >= 1.
//
// bool RegEx::Search(const char * subject, int len = -1, int options = 0)
//
//    Method Search() applies the regular expression to parameter subject.
//    Optional parameter len can be used to pass the subject's length to
//    Search(). If not specified (or less than 0), strlen() is used
//    internally to determine the length. Parameter options can contain
//    any combination of options PCRE_ANCHORED, PCRE_NOTBOL, PCRE_NOTEOL.
//    PCRE_NOTEMPTY. Search() returns true if a match is found.
//
// bool RegEx::SearchAgain(int options = 0)
//
//    SearchAgain() again applies the regular expression to parameter
//    subject last passed to a successful call of Search(). It returns
//    true if a further match is found. Subsequent calls to SearchAgain()
//    will find all matches in subject. Example:
//
//       if (Pattern.Search(astring)) {
//          do {
//             printf("%s\n", Pattern.Match());
//          } while (Pattern.SearchAgain());
//       }
//
//    Parameter options is interpreted as for method Search().
//
// const char * RegEx::Match(int i = 1)
//
//    Method Match() returns a pointer to the matched substring specified
//    with parameter i. Match() may only be called after a successful
//    call to Search() or SearchAgain() and applies to that last
//    Search()/SearchAgain() call. Parameter i must be less than
//    SubStrings(). Match(-1) returns the last searched subject.
//    Match(0) returns the match of the complete regular expression.
//    Match(1) returns $1, etc.
//
// The bottom of this file contains an example using class RegEx. It's
// the simplest version of grep I could come with. You can compile it by
// defining REGEX_DEMO on the compiler command line.
//

#ifndef _REGEX_H
#define _REGEX_H

#include <string.h>

#ifndef _PCRE2_H
#define PCRE2_CODE_UNIT_WIDTH 8
#include "pcre2.h"
#endif

class RegEx
{
	public:
		/////////////////////////////////
		RegEx(const char *regex, int options = 0)
		{
			int error;
			PCRE2_SIZE erroroffset;
			PCRE2_SPTR pattern;
			PCRE2_SIZE pattern_length;

			pattern = (PCRE2_SPTR)regex;
			pattern_length = (PCRE2_SIZE)strlen((char *)regex);

			re = pcre2_compile(pattern, pattern_length, options, &error, &erroroffset, NULL);
			if (re == NULL)
			{
#if PCRE2_WRAPPER_DEBUG
				PCRE2_UCHAR buffer[256];
				pcre2_get_error_message(error, buffer, sizeof(buffer));
				fprintf(stderr, "PCRE2 compilation failed at offset %d: %s\n", (int)erroroffset, buffer);
#endif
				throw error;
			}
			match_data = pcre2_match_data_create_from_pattern(re, NULL);
		};

		/////////////////////////////////
		~RegEx()
		{
			ClearMatchData();
			pcre2_code_free(re);
		}

		/////////////////////////////////
		bool Search(const char *text)
		{
			int rc = 0;
			PCRE2_SPTR subject;
			PCRE2_SIZE subject_length;

			subject = (PCRE2_SPTR)text;
			subject_length = (PCRE2_SIZE)strlen((char *)text);

			rc = pcre2_match(re, subject, subject_length, 0, 0, match_data, NULL);
#if PCRE2_WRAPPER_DEBUG
			fprintf(stderr, "PCRE2 pcre2_match rc: %d\n", rc);
#endif
			return rc > 0;
		}

	private:
		inline void ClearMatchData(void)
		{
			if (match_data)
			{
				pcre2_match_data_free(match_data);
				match_data = NULL;
			}
		}
		pcre2_code *re;
		pcre2_match_data *match_data;
};

// Below is a little demo/test program using class RegEx

#ifdef REGEX_DEMO

#include <stdio.h>
#include "regex.hpp"

///////////////////////////////////////
int main(int argc, char * argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: grep pattern\n\n"
							 "Reads stdin, searches 'pattern', writes to stdout\n");
		return 2;
	}
	try
	{
		RegEx Pattern(argv[1]);
		int count = 0;
		char buffer[1024];

		while (fgets(buffer, sizeof(buffer), stdin))
			if (Pattern.Search(buffer))
				fputs(buffer, stdout),
				count++;
		return count == 0;
	}
	catch (const char * ErrorMsg)
	{
		fprintf(stderr, "error in regex '%s': %s\n", argv[1], ErrorMsg);
		return 2;
	}
}

#endif // REGEX_DEMO

#endif // _REGEX_H
