AUTOMAKE_OPTIONS = subdir-objects

include $(top_srcdir)/silent_rules.mk

AM_CPPFLAGS = $(RNA_CPPFLAGS) -I$(top_srcdir)/src -I$(top_builddir)/src/
AM_CFLAGS = $(RNA_CFLAGS) $(PTHREAD_CFLAGS) @CHECK_CFLAGS@ 
AM_CXXFLAGS = $(RNA_CXXFLAGS) $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(RNA_LDFLAGS) $(PTHREAD_LIBS)

LDADD = $(top_builddir)/src/ViennaRNA/libRNA_conv.la @CHECK_LIBS@

if VRNA_AM_SWITCH_MPFR
LDADD += $(MPFR_LIBS)
endif

# Link against stdc++ if we use SVM
if VRNA_AM_SWITCH_SVM
LDADD += $(SVM_LIBS)
endif

SUFFIXES = .c .ts
TEST_EXTENSIONS = .pl .t .py .py2
LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
                  $(top_srcdir)/config/tap-driver.sh


if WITH_UNIT_TESTS

AM_TESTS_ENVIRONMENT =  \
                        CK_DEFAULT_TIMEOUT=20 \
## Some environment initializations are kept in a separate shell
## file 'tests-env.sh', which can make it easier to also run tests
## from the command line.
                        . $(builddir)/test-env.sh; \
## On Solaris, prefer more POSIX-compliant versions of the standard
## tools by default.
                        if test -d /usr/xpg4/bin; then \
                          PATH=/usr/xpg4/bin:$$PATH; export PATH; \
                        fi;

## With this, the test scripts will be able to print diagnostic
## messages to the original standard error stream, even if the test
## driver redirects the stderr of the test scripts to a log file
## before executing them.
AM_TESTS_FD_REDIRECT = 9>&2

######################################
## unit tests for Perl5 interface   ##
######################################
if WITH_PERL_TESTS

BUILT_SOURCES = perl5/test-RNA-Design.t

PL_LOG_COMPILER = $(PERL)
T_LOG_COMPILER  = $(PERL)
PL_LOG_DRIVER   = env AM_TAP_AWK='$(AWK)' $(SHELL) \
                  $(top_srcdir)/config/tap-driver.sh
T_LOG_DRIVER    = env AM_TAP_AWK='$(AWK)' $(SHELL) \
                  $(top_srcdir)/config/tap-driver.sh

AM_PL_LOG_FLAGS = -Mstrict -w \
                  -I$(top_builddir)/interfaces/Perl \
                  -I$(top_srcdir)/interfaces/Perl \
                  -I$(top_builddir)/tests
AM_T_LOG_FLAGS = -Mstrict -w \
                  -I$(top_builddir)/interfaces/Perl \
                  -I$(top_srcdir)/interfaces/Perl \
                  -I$(top_builddir)/tests

# add test scripts here
PERL_TESTS =  perl5/test-RNA.pl \
              perl5/test-RNA-callbacks.pl \
              perl5/test-RNA-combinatorics.pl \
              perl5/test-RNA-Design.t \
              perl5/test-RNA-ensemble_defect.pl \
              perl5/test-RNA-file-formats.pl \
              perl5/test-RNA-mfe_eval.pl \
              perl5/test-RNA-mfe_window.pl \
              perl5/test-RNA-pf_window.pl \
              perl5/test-RNA-boltzmann-sampling.pl \
              perl5/test-RNA-constraints.pl \
              perl5/test-RNA-constraints-soft.pl \
              perl5/test-RNA-constraints-SHAPE.pl \
              perl5/test-RNA-constraints-special.pl


# add test script output files here
PERL_TEST_OUTPUT =  test.coord \
                    test_dp.ps \
                    test_ss.ps \
                    test_ss_a.ps \
                    test.par \
                    test-RNA-mfe_eval.pl.out \
                    outputFile_test.txt

perl5/test-RNA-Design.t: $(top_srcdir)/interfaces/Perl/RNA/t/Design.t
	cp $(top_srcdir)/interfaces/Perl/RNA/t/Design.t $(builddir)/perl5/test-RNA-Design.t

endif

######################################  
## unit tests for Python2 interface ##
######################################
if WITH_PYTHON2_TESTS

PY2_LOG_COMPILER = $(PYTHON2)
AM_PY2_LOG_FLAGS =
PY2_LOG_DRIVER   = env AM_TAP_AWK='$(AWK)' $(SHELL) \
                  $(top_srcdir)/config/tap-driver.sh

# add test scripts here
PYTHON2_TESTS = python2/test-RNA-combinatorics.py2 \
                python2/test-RNA-constraints.py2 \
                python2/test-RNA-constraints-soft.py2 \
                python2/test-RNA-constraints-SHAPE.py2 \
                python2/test-RNA-constraints-special.py2 \
                python2/test-RNA-ensemble_defect.py2 \
                python2/test-RNA-file-formats.py2 \
                python2/test-RNA-mfe_eval.py2 \
                python2/test-RNA-mfe_window.py2 \
                python2/test-RNA-pf_window.py2 \
                python2/test-RNA-sc-callbacks.py2 \
                python2/test-RNA-subopt.py2 \
                python2/test-RNA-boltzmann-sampling.py2 \
                python2/test-RNA-utils.py2 \
                python2/test-RNA.py2

# add test script output files here
PYTHON2_TEST_OUTPUT = test-RNA-mfe_eval.py2.out

endif

######################################
## unit tests for Python3 interface ##
######################################
if WITH_PYTHON_TESTS

PY_LOG_COMPILER  = $(PYTHON3)
AM_PY_LOG_FLAGS  =
PY_LOG_DRIVER    = env AM_TAP_AWK='$(AWK)' $(SHELL) \
                   $(top_srcdir)/config/tap-driver.sh

# add test scripts here
PYTHON3_TESTS = python/test_RNA-combinatorics.py \
                python/test_RNA-constraints.py \
                python/test_RNA-constraints-soft.py \
                python/test_RNA-constraints-SHAPE.py \
                python/test_RNA-constraints-special.py \
                python/test_RNA-ensemble_defect.py \
                python/test_RNA-file-formats.py \
                python/test_RNA-mfe_eval.py \
                python/test_RNA-mfe_window.py \
                python/test_RNA-pf_window.py \
                python/test_RNA-sc-callbacks.py \
                python/test_RNA-subopt.py \
                python/test_RNA-boltzmann-sampling.py \
                python/test_RNA-utils.py \
                python/test_RNA-modified-bases.py \
                python/test_RNA-circ.py \
                python/test_RNA.py

# add test script output files here
PYTHON3_TEST_OUTPUT = test_RNA-mfe_eval.py.out

endif

######################################
## unit tests for C-library         ##
######################################
if WITH_CHECK

CHECKMK_FILES = \
              energy_evaluation.ts \
              constraints.ts \
              constraints_soft.ts \
              ensemble_defect.ts \
              fold.ts \
              utils.ts \
              eval_structure.ts \
              walk.ts \
              neighbor.ts \
              hash_table.ts

CHECK_CFILES = \
              energy_evaluation.c \
              constraints.c \
              constraints_soft.c \
              ensemble_defect.c \
              fold.c \
              utils.c \
              eval_structure.c \
              walk.c \
              neighbor.c \
              hash_table.c

LIBRARY_TESTS = energy_evaluation \
                constraints \
                constraints_soft \
                ensemble_defect \
                fold \
                utils \
                eval_structure \
                walk \
                neighbor \
                hash_table

check_PROGRAMS = ${LIBRARY_TESTS}

endif

########################################
## unit tests for executable programs ##
########################################
if WITH_EXECUTABLE_TESTS

EXECUTABLE_TESTS  = \
                  RNAfold/general \
                  RNAfold/partfunc \
                  RNAfold/special \
                  RNAfold/long \
                  RNAcofold/general \
                  RNAcofold/partfunc \
                  RNAalifold/general \
                  RNAalifold/partfunc \
                  RNAalifold/special

endif

TESTS = \
      ${EXECUTABLE_TESTS} \
      ${LIBRARY_TESTS} \
      ${PERL_TESTS} \
      ${PYTHON2_TESTS} \
      ${PYTHON3_TESTS}

## Below are the rules how to create the test suite executables
## See checkmk man page for details about *.ts files
.ts.c:
	$(checkmk_verbose)checkmk $< > $@

endif

EXTRA_DIST =  data \
              py_include/__init__.py \
              py_include/taprunner/__init__.py \
              RNAfold/results \
              RNAcofold/results \
              RNAalifold/results \
              ${EXECUTABLE_TESTS} \
              $(CHECKMK_FILES) \
              $(CHECK_CFILES) \
              ${PERL_TESTS} \
              ${PYTHON2_TESTS} \
              ${PYTHON3_TESTS}

clean-local:
	-rm -rf \
    ${PERL_TEST_OUTPUT} \
    $(PYTHON2_TEST_OUTPUT) \
    $(PYTHON3_TEST_OUTPUT) \
    *.pyc \
    __pycache__ \
    py_include/*.pyc \
    py_include/__pycache__ \
    py_include/taprunner/*.pyc \
    py_include/taprunner/__pycache__
