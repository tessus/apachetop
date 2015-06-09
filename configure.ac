# Process this file with autoconf to produce a configure script.

AC_INIT([apachetop],[0.12.7],[https://github.com/tessus/apachetop/issues])
AC_CONFIG_SRCDIR(src/apachetop.cc)

AC_CONFIG_AUX_DIR(config)
#AC_CONFIG_MACRO_DIR(m4) # 2.58

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE

# Add non-standard directories to the include path
AC_ARG_WITH(libraries,
	[
  --with-libraries=<path> additional place to look for libraries],
	[LDFLAGS="$LDFLAGS -L $withval"],
	,
)
# Add non-standard includes to the include path
AC_ARG_WITH(includes,
	[
  --with-includes=<path> additional place to look for header files],
	[CPPFLAGS="$CPPFLAGS -I $withval"],
	,
)


# Checks for programs.
#AC_PROG_CC
#AC_PROG_CPP
AC_PROG_CXX
AC_LANG([C++])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h \
		string.h strings.h sys/socket.h time.h sys/time.h \
		limits.h sys/param.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_HEADER_STDBOOL # not in 2.53?
AC_C_CONST
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_SYS_LARGEFILE

# Checks for library functions.
AC_FUNC_MALLOC
#AC_FUNC_REALLOC # not in 2.53?
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([inet_aton memset strchr strdup kqueue strerror strstr])

# pcre {{{
AC_ARG_WITH(pcre,
	[  --with-pcre=<path>      prefix of pcre installation (eg /usr/local)],
	[
		CPPFLAGS="$CPPFLAGS -I $withval/include"
		LDFLAGS="$LDFLAGS -L $withval/lib"
	]
)

AC_CHECK_HEADERS(pcre.h,
	AC_SEARCH_LIBS([pcre_compile], [pcre]) ,
	AC_MSG_WARN([*** pcre.h not found -- consider using --with-pcre])
)
# }}}

# fam {{{
AC_ARG_WITH(fam,
	[  --with-fam=<path>      prefix of fam installation (eg /usr/local)],
	[
		CPPFLAGS="$CPPFLAGS -I $withval/include"
		LDFLAGS="$LDFLAGS -L $withval/lib"
	]
)

AC_CHECK_HEADERS(fam.h,
	AC_SEARCH_LIBS([FAMOpen], [fam]) ,
	AC_MSG_WARN([*** fam.h not found -- consider using --with-fam])
)
# }}}

# adns {{{
AC_ARG_WITH(adns,
	[  --with-adns=<path>     prefix of adns installation (eg /usr/local)],
	[
		CPPFLAGS="$CPPFLAGS -I $withval/include"
		LDFLAGS="$LDFLAGS -L $withval/lib"
	]
)

AC_CHECK_HEADERS(adns.h,
	AC_SEARCH_LIBS([adns_submit], [adns]) ,
	AC_MSG_WARN([*** adns.h not found -- consider using --with-adns])
)
# }}}

# --with-logfile {{{
AC_ARG_WITH(logfile,
	[
  --with-logfile=<log>    location of default logfile],
	[AC_DEFINE_UNQUOTED(DEFAULT_LOGFILE, "$withval", [Optionally override the DEFAULT_LOGFILE in apachetop.h])],
	,
)
# }}}

AC_SEARCH_LIBS([socket], [socket])
AC_SEARCH_LIBS([inet_addr], [nsl])
AC_SEARCH_LIBS([attron], [ncurses])
AC_SEARCH_LIBS([tgetstr], [termcap])
AC_SEARCH_LIBS([mvprintw], [curses ncurses] ,
	[] ,
	[
		AC_MSG_ERROR([No useful curses library found!])
	]
)

AC_SEARCH_LIBS([readline], [readline],
	[
		AC_DEFINE(HAVE_READLINE,1,[Define if you have readline library])
		AC_SUBST(HAVE_READLINE)
	] ,
	[
		AC_MSG_ERROR(readline library not found)
	]
)


# everything is in CPPFLAGS up to this point, now we move to CXXFLAGS
# this is mostly done for pcre test, AC_CHECK_HEADERS wants to use CPP
#CXXFLAGS="$CXXFLAGS $CPPFLAGS"
AC_SUBST(CXXFLAGS)

AC_CONFIG_FILES([Makefile man/Makefile src/Makefile config/Makefile])
AC_OUTPUT
