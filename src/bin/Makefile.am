AUTOMAKE_OPTIONS = subdir-objects

include $(top_srcdir)/silent_rules.mk

AM_CFLAGS = $(RNA_CFLAGS) $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(RNA_CXXFLAGS) $(PTHREAD_CFLAGS)
AM_CPPFLAGS = $(RNA_CPPFLAGS) -I$(top_srcdir)/src
AM_LDFLAGS = $(RNA_LDFLAGS) $(PTHREAD_LIBS)

if VRNA_AM_SWITCH_STATIC_EXECUTABLES
AM_LDFLAGS += -all-static
endif

noinst_LTLIBRARIES =  libhelpers.la

libhelpers_la_SOURCES = gengetopt_helpers.c \
                        input_id_helpers.c \
                        modified_bases_helpers.c \
                        parallel_helpers.c

libhelpers_la_LDFLAGS = \
        -avoid-version \
        -static \
        $(LTO_LDFLAGS)

# Link with cthreadpool if parallel input processing is enabled
if VRNA_AM_SWITCH_PTHREADS

AM_CPPFLAGS += -I$(top_srcdir)/src/cthreadpool
libhelpers_la_SOURCES += ../cthreadpool/thpool.c

endif

bin_PROGRAMS = \
        RNAfold RNAeval RNAheat RNApdist RNAdistance RNAinverse \
        RNAplot RNAsubopt RNALfold RNAcofold RNApaln RNAduplex \
        RNAalifold RNAplfold RNAup RNAaliduplex RNALalifold RNA2Dfold \
        RNAparconv RNAPKplex RNAplex RNAsnoop RNApvmin RNAdos \
        RNAmultifold

LDADD = \
        libhelpers.la \
        $(top_builddir)/src/ViennaRNA/libRNA_conv.la

if VRNA_AM_SWITCH_MPFR
LDADD += $(MPFR_LIBS)
endif

noinst_HEADERS = \
        gengetopt_helpers.h \
        input_id_helpers.h \
        modified_bases_helpers.h \
        parallel_helpers.h \
        $(top_srcdir)/src/cthreadpool/thpool.h

SUFFIXES = _cmdl.c _cmdl.h .ggo

# Link against stdc++ if we use SVM
if VRNA_AM_SWITCH_SVM
LDADD += $(SVM_LIBS)
endif

RNAmultifold_LDADD = $(LDADD)
RNAmultifold_LDADD += -lstdc++

RNAfold_LDADD = \
    $(top_builddir)/src/ViennaRNA/libRNA_special_const.la \
    $(LDADD)

GENGETOPT_CMDL = \
        RNAfold_cmdl.c RNAfold_cmdl.h \
        RNAalifold_cmdl.c RNAalifold_cmdl.h \
        RNAcofold_cmdl.c  RNAcofold_cmdl.h \
        RNAup_cmdl.c  RNAup_cmdl.h \
        RNALfold_cmdl.c RNALfold_cmdl.h \
        RNALalifold_cmdl.c RNALalifold_cmdl.h \
        RNAplfold_cmdl.c RNAplfold_cmdl.h \
        RNAduplex_cmdl.c RNAduplex_cmdl.h \
        RNAaliduplex_cmdl.c RNAaliduplex_cmdl.h \
        RNAeval_cmdl.c RNAeval_cmdl.h \
        RNAheat_cmdl.c RNAheat_cmdl.h \
        RNAsubopt_cmdl.c RNAsubopt_cmdl.h \
        RNA2Dfold_cmdl.c RNA2Dfold_cmdl.h \
        RNAparconv_cmdl.c RNAparconv_cmdl.h \
        RNAinverse_cmdl.c RNAinverse_cmdl.h \
        RNApaln_cmdl.c RNApaln_cmdl.h \
        RNAplot_cmdl.c RNAplot_cmdl.h \
        RNAPKplex_cmdl.c RNAPKplex_cmdl.h \
        RNAplex_cmdl.c RNAplex_cmdl.h \
        RNAsnoop_cmdl.c RNAsnoop_cmdl.h \
        RNAdistance_cmdl.c RNAdistance_cmdl.h \
        RNApdist_cmdl.c RNApdist_cmdl.h \
        RNApvmin_cmdl.c RNApvmin_cmdl.h \
        RNAdos_cmdl.c RNAdos_cmdl.h \
        RNAmultifold_cmdl.c RNAmultifold_cmdl.h

GENGETOPT_FILES = \
        RNAfold.ggo RNAalifold.ggo RNAcofold.ggo \
        RNAup.ggo RNALfold.ggo RNALalifold.ggo \
        RNAplfold.ggo RNAduplex.ggo RNAaliduplex.ggo \
        RNAeval.ggo RNAheat.ggo RNAsubopt.ggo RNA2Dfold.ggo \
        RNAparconv.ggo RNAinverse.ggo RNApaln.ggo RNAplot.ggo \
        RNAPKplex.ggo RNAplex.ggo RNAsnoop.ggo RNAdistance.ggo \
        RNApdist.ggo RNApvmin.ggo RNAdos.ggo RNAmultifold.ggo

EXTRA_DIST = \
        ${GENGETOPT_FILES} \
        ${GENGETOPT_CMDL} \
        ${HELPER_FILES}


if VRNA_AM_SWITCH_HAS_GENGETOPT

## Below are the rules how to create the test suite executables
# how we make the commandline interface parser
## See checkmk man page for details about *.ts files
.ggo_cmdl.c:
	$(ggo_verbose)$(GENGETOPT) -i $<

_cmdl.c_cmdl.h :
## Recover from the removal of $@
	@if test -f $@; then : touch $@ ; else \
          rm -f $< ; \
          $(MAKE) $(AM_MAKEFLAGS) $< ; \
        fi

endif


# remove all generated man pages
maintainer-clean-local:
	rm -rf $(GENGETOPT_CMDL)

RNAfold_SOURCES = \
    RNAfold_cmdl.c \
    RNAfold_cmdl.h \
    RNAfold.c

RNAsubopt_SOURCES = \
    RNAsubopt_cmdl.c \
    RNAsubopt_cmdl.h \
    RNAsubopt.c

RNAalifold_SOURCES = \
    RNAalifold_cmdl.c \
    RNAalifold_cmdl.h \
    RNAalifold.c

RNAcofold_SOURCES = \
    RNAcofold_cmdl.c \
    RNAcofold_cmdl.h \
    RNAcofold.c

RNAup_SOURCES = \
    RNAup_cmdl.c \
    RNAup_cmdl.h \
    RNAup.c

RNALfold_SOURCES = \
    RNALfold_cmdl.c \
    RNALfold_cmdl.h \
    RNALfold.c

RNALalifold_SOURCES = \
    RNALalifold_cmdl.c \
    RNALalifold_cmdl.h \
    RNALalifold.c

RNAplfold_SOURCES = \
    RNAplfold_cmdl.c \
    RNAplfold_cmdl.h \
    RNAplfold.c

RNAduplex_SOURCES = \
    RNAduplex_cmdl.c \
    RNAduplex_cmdl.h \
    RNAduplex.c

RNAaliduplex_SOURCES = \
    RNAaliduplex_cmdl.c \
    RNAaliduplex_cmdl.h \
    RNAaliduplex.c

RNAeval_SOURCES = \
    RNAeval_cmdl.c \
    RNAeval_cmdl.h \
    RNAeval.c

RNAheat_SOURCES = \
    RNAheat_cmdl.c \
    RNAheat_cmdl.h \
    RNAheat.c

RNA2Dfold_SOURCES = \
    RNA2Dfold_cmdl.c \
    RNA2Dfold_cmdl.h \
    RNA2Dfold.c

RNAparconv_SOURCES = \
    RNAparconv_cmdl.c \
    RNAparconv_cmdl.h \
    RNAparconv.c

RNAinverse_SOURCES = \
    RNAinverse_cmdl.c \
    RNAinverse_cmdl.h \
    RNAinverse.c

RNApaln_SOURCES = \
    RNApaln_cmdl.c \
    RNApaln_cmdl.h \
    RNApaln.c

RNAplot_SOURCES = \
    RNAplot_cmdl.c \
    RNAplot_cmdl.h \
    RNAplot.c

RNAPKplex_SOURCES = \
    RNAPKplex_cmdl.c \
    RNAPKplex_cmdl.h \
    RNAPKplex.c

RNAplex_SOURCES = \
    RNAplex_cmdl.c \
    RNAplex_cmdl.h \
    RNAplex.c

RNAsnoop_SOURCES = \
    RNAsnoop_cmdl.c \
    RNAsnoop_cmdl.h \
    RNAsnoop.c

RNAdistance_SOURCES = \
    RNAdistance_cmdl.c \
    RNAdistance_cmdl.h \
    RNAdistance.c

RNApdist_SOURCES = \
    RNApdist_cmdl.c \
    RNApdist_cmdl.h \
    RNApdist.c

RNApvmin_SOURCES = \
    RNApvmin_cmdl.c \
    RNApvmin_cmdl.h \
    RNApvmin.c

RNAdos_SOURCES = \
    RNAdos_cmdl.c \
    RNAdos_cmdl.h \
    RNAdos.c

RNAmultifold_SOURCES = \
    RNAmultifold_cmdl.c \
    RNAmultifold_cmdl.h \
    RNAmultifold.c
