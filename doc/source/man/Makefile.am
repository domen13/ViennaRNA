include $(top_srcdir)/silent_rules.mk

REFMAN_MANPAGES = \
    RNA2Dfold.rst \
    RNAaliduplex.rst \
    RNAalifold.rst \
    RNAcofold.rst \
    RNAdistance.rst \
    RNAdos.rst \
    RNAduplex.rst \
    RNAeval.rst \
    RNAfold.rst \
    RNAheat.rst \
    RNAinverse.rst \
    RNALalifold.rst \
    RNALfold.rst \
    RNAmultifold.rst \
    RNApaln.rst \
    RNAparconv.rst \
    RNApdist.rst \
    RNAPKplex.rst \
    RNAplex.rst \
    RNAplfold.rst \
    RNAplot.rst \
    RNApvmin.rst \
    RNAsnoop.rst \
    RNAsubopt.rst \
    RNAup.rst


SUFFIXES = .1 .rst

vpath %.1 = ../../../man

if WITH_REFERENCE_MANUAL

.1.rst:
	$(man2rst_verbose)$(PYTHON3) ../../man2rst.py -i $< -o $@

endif WITH_REFERENCE_MANUAL

BUILT_SOURCES = $(REFMAN_MANPAGES)

# remove all generated man pages
maintainer-clean-local:
	-rm -f $(REFMAN_MANPAGES)

