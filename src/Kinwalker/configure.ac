AC_INIT([kinwalker],[2.0])

AC_PREREQ([2.59])
AC_CONFIG_AUX_DIR(config)

AC_SUBST(VERSION)

AC_CONFIG_MACRO_DIR([m4])

AM_SILENT_RULES([yes])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

if test "$GCC" = yes; then
  AC_DEFINE(UNUSED, __attribute__ ((unused)), unsused argument avoids warning about unused variables)	
else
  AC_DEFINE(UNUSED,, only for gcc)
fi

#check for compilers c++ and c
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_CC
AC_PROG_CPP

AX_CXX_COMPILE_STDCXX(14, noext, mandatory)

#Output this variables to the makefiles

AC_SUBST(VERSION)
ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)

#Check headers
AC_CHECK_HEADERS(malloc.h strings.h unistd.h)
AC_C_INLINE   #check if C compiler uses INLINE

AC_CONFIG_HEADERS(config.h) 


#Serach for the RNAlib2 and set variables for Makefiles if found.  _CFLAGS and _LIBS can be overriden with the modue Name from PKG_CHECK_MODULES
PKG_CHECK_MODULES([VRNA], [RNAlib2],
                  [ AC_SUBST([VRNA_CFLAGS])
                    AC_SUBST([VRNA_LIBS])
                  ],
                  [])
AC_CONFIG_FILES([Makefile man/Makefile])
AC_OUTPUT
