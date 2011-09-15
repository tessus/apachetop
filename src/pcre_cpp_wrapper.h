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

#ifndef _PCRE_H
#include "pcre.h"
#endif

class RegEx
{
   public:
      /////////////////////////////////
      RegEx(const char * regex, int options = 0)
      {
         const char * error;
         int          erroffset;

         re = pcre_compile(regex, options, &error, &erroffset, NULL);
         if (re == NULL)
            throw error;
         pe = pcre_study(re, 0, &error);
         pcre_fullinfo(re, pe, PCRE_INFO_CAPTURECOUNT, &substrcount);
         substrcount++;
         ovector = new int[3*substrcount];
         matchlist = NULL;
      };

      /////////////////////////////////
      ~RegEx()
      {
         ClearMatchList();
         delete ovector;
         if (pe)
            pcre_free(pe);
         pcre_free(re);
      }

      /////////////////////////////////
      inline int SubStrings(void) const
      {
         return substrcount;
      }

      /////////////////////////////////
      bool Search(const char * subject, int len = -1, int options = 0)
      {
         ClearMatchList();
         return pcre_exec(re, pe, lastsubject = subject, slen = (len >= 0) ? len : strlen(subject), 0, options, ovector, 3*substrcount) > 0;
      }

      /////////////////////////////////
      bool SearchAgain(int options = 0)
      {
         ClearMatchList();
         return pcre_exec(re, pe, lastsubject, slen, ovector[1], options, ovector, 3*substrcount) > 0;
      }

      /////////////////////////////////
      const char * Match(int i = 1)
      {
         if (i < 0)
            return lastsubject;
         if (matchlist == NULL)
            pcre_get_substring_list(lastsubject, ovector, substrcount, &matchlist);
         return matchlist[i];
      }

   private:
      inline void ClearMatchList(void)
      {
         if (matchlist)
            pcre_free_substring_list(matchlist),
            matchlist = NULL;
      }
      pcre * re;
      pcre_extra * pe;
      int substrcount;
      int * ovector;
      const char * lastsubject;
      int slen;
      const char * * matchlist;
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
