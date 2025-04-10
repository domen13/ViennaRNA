dnl Process this file with autoconf to produce a configure script.
m4_define([vrna_version_major], [2])
m4_define([vrna_version_minor], [7])
m4_define([vrna_version_patch], [0])

dnl Every other copy of the package version number gets its value from here
AC_INIT([ViennaRNA],[vrna_version_major.vrna_version_minor.vrna_version_patch],[rna@tbi.univie.ac.at],[ViennaRNA],[https://www.tbi.univie.ac.at/RNA])
AC_PREREQ([2.69])
AC_CONFIG_AUX_DIR(config)

AC_SUBST(VERSION)

AC_CONFIG_MACRO_DIR([m4])

AC_USE_SYSTEM_EXTENSIONS

dnl change the default installation prefix
dnl AC_PREFIX_DEFAULT(/usr/local/RNA)

AM_SILENT_RULES([yes])

AM_INIT_AUTOMAKE([-Wall -Wno-override foreign 1.11 tar-ustar])

dnl create a config.h file (Automake will add -DHAVE_CONFIG_H)
AC_CONFIG_HEADERS([config.h])

dnl configure options
AM_WITH_DMALLOC

ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)

AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_CC
AC_PROG_CPP
AC_PROG_F77

if test "$GCC" = yes; then
   AC_DEFINE(UNUSED, __attribute__ ((unused)), avoid warning about unused variables)
else
   AC_DEFINE(UNUSED,, only for gcc)
fi

AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_PROG_SED
AC_PROG_AWK
AC_PROG_GREP
AC_PROG_EGREP
AC_PROG_FGREP

AC_PROG_MKDIR_P
AC_PROG_LN_S

AM_PROG_AR

LT_INIT([pic-only, static])
AC_SUBST(LIBTOOL_DEPS)


dnl Checks for libraries.
AC_CHECK_LIB([m], [exp])


dnl Checks for header files.
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([malloc.h float.h limits.h stdlib.h string.h strings.h unistd.h math.h stdarg.h])

dnl Checks for funtions
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor strdup strstr strchr strrchr strstr strtol strtoul pow rint sqrt erand48 memset memmove erand48 asprintf vasprintf])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_C_INLINE
AC_C__GENERIC


AC_RNA_INIT

AC_OUTPUT

AC_RNA_NOTICE
