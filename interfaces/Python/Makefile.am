include $(top_srcdir)/silent_rules.mk
include ../generic.mk

if WITH_REFERENCE_MANUAL_XML
doxy2swig_pydoc_file = documentation.i
doxy2swig_pydoc_define = -DDOXY2SWIG
else
doxy2swig_pydoc_file = 
doxy2swig_pydoc_define = 
endif


LANG_SPECIFIC_INTERFACE_FILES = \
  $(srcdir)/tmaps.i \
  $(srcdir)/file_py3.i \
  $(srcdir)/globals-md.i \
  $(srcdir)/callbacks-fc.i \
  $(srcdir)/callbacks-sc.i \
  $(srcdir)/callbacks-sc-direct.i \
  $(srcdir)/callbacks-ud.i \
  $(srcdir)/callbacks-subopt.i \
  $(srcdir)/callbacks-boltzmann-sampling.i \
  $(srcdir)/callbacks-melting.i \
  $(srcdir)/callbacks-landscape.i \
  $(srcdir)/callbacks-mfe-window.i \
  $(srcdir)/callbacks-pf-window.i \
  $(srcdir)/callbacks-log.i \
  $(builddir)/version.i \
  $(doxy2swig_pydoc_file)


INTERFACE_FILES = $(SWIG_src) \
                  $(LANG_SPECIFIC_INTERFACE_FILES)

pkgpyexecdir = $(py3execdir)/RNA
pkgpycachedir = $(pkgpyexecdir)/__pycache__
pkgpyexec_LTLIBRARIES = _RNA.la
pkgpyexec_DATA = \
    RNA/__init__.py \
    RNA/RNA.py
pkgpycache_DATA = \
    RNA/__pycache__/__init__.@PYTHON3_CACHE_TAG@.pyc \
    RNA/__pycache__/__init__.@PYTHON3_CACHE_OPT1_EXT@ \
    RNA/__pycache__/RNA.@PYTHON3_CACHE_TAG@.pyc \
    RNA/__pycache__/RNA.@PYTHON3_CACHE_OPT1_EXT@

pkgpyvrnaexecdir = $(py3execdir)/ViennaRNA
pkgpyvrnaexec_DATA = \
    ViennaRNA/__init__.py

_RNA_la_SOURCES = $(INTERFACE_FILES) \
                  $(SWIG_wrapper)

EXTRA_DIST = \
  $(SWIG_wrapper) \
  $(SWIG_module_name).py \
  RNA/RNA.py \
  ViennaRNA/__init__.py \
  $(LANG_SPECIFIC_INTERFACE_FILES)

_RNA_la_CFLAGS = $(RNA_CFLAGS) $(PTHREAD_CFLAGS)
_RNA_la_CXXFLAGS = $(RNA_CXXFLAGS) $(PTHREAD_CFLAGS) -Wno-unused-label -Wno-unused-variable
_RNA_la_CPPFLAGS = -I$(PYTHON3_INC) $(RNA_CPPFLAGS) -I$(top_srcdir)/src

_RNA_la_LIBADD = \
    $(LDADD) \
    $(top_builddir)/src/ViennaRNA/libRNA_conv.la

if VRNA_AM_SWITCH_SVM
_RNA_la_LIBADD += -lstdc++
_RNA_la_CPPFLAGS += -DVRNA_WITH_SVM
RNA_CPPFLAGS += -DVRNA_WITH_SVM
endif

if VRNA_AM_SWITCH_MPFR
_RNA_la_LIBADD += $(MPFR_LIBS)
endif

_RNA_la_LDFLAGS = -Xcompiler $(PTHREAD_CFLAGS) \
                  $(PTHREAD_LIBS) \
                  -avoid-version \
                  -module \
                  -shared \
                  -export-dynamic \
                  -shrext \
                  $(PYTHON3_SO) \
                  $(PYTHON3_LDFLAGS)

if HAS_SWIG

if WITH_REFERENCE_MANUAL_XML

documentation.i: $(srcdir)/../classify.csv $(srcdir)/../typedefs.csv $(srcdir)/../ignore_list.csv
	$(doxy2swig_verbose)test -d $(top_builddir)/doc/doxygen/xml || ( test -d $(top_srcdir)/doc/doxygen/xml && $(LN_S) $(top_srcdir)/doc/doxygen/xml $(top_builddir)/doc/doxygen/xml ) && \
	$(PYTHON3) $(top_srcdir)/doc/doxy2swig/doxy2swig.py \
      -b $(srcdir)/../classify.csv \
      -m $(srcdir)/../typedefs.csv \
      -i $(srcdir)/../ignore_list.csv \
      -p "vrna_" \
      -p "VRNA_" \
      --prefix-name="RNA" \
      -s "_t" \
      -s "_s" \
      -s "_e" \
      -s "_f" \
      --constructor-suffix="_t" \
      --constructor-suffix="_s" \
      -t \
      -a \
      -c \
      -w 100 \
      -q \
      -d $(PACKAGE_VERSION) \
      $(top_builddir)/doc/doxygen/xml/index.xml \
      documentation.i

endif WITH_REFERENCE_MANUAL_XML

$(SWIG_wrapper): $(SWIG_src) $(INTERFACE_FILES)
	$(swig_verbose)$(SWIG) \
                  -I$(top_srcdir)/src \
                  -I$(srcdir) \
                  -I$(builddir) \
                  $(doxy2swig_pydoc_define) \
                  $(RNA_CPPFLAGS) \
                  -python \
                  -c++ \
                  -keyword \
                  -w511,509 \
                  -o $(SWIG_wrapper) \
                  $(SWIG_main_src)


$(SWIG_module_name).py: $(SWIG_wrapper)

endif HAS_SWIG


# We copy RNA.py to RNA/RNA.py so that it becomes a module
# in an "RNA" directory that itself serves as package.
RNA/RNA.py: $(SWIG_module_name).py
	$(AM_V_GEN)cp `test -f RNA.py || echo '$(srcdir)/'`RNA.py RNA/RNA.py

# We "import _RNA" first so that if we fail to import the glue library
# we don't generate a broken .pyc or .pyo.
RNA/__pycache__/__init__.@PYTHON3_CACHE_TAG@.pyc: RNA/RNA.py RNA/_RNA$(PYTHON3_SO)
	$(AM_V_GEN)( PYTHONPATH="RNA:$$PYTHONPATH" $(PYTHON3) -c "import _RNA" ; \
  PYTHONPATH=".:$$PYTHONPATH" $(PYTHON3) -c "import RNA" )

RNA/__pycache__/__init__.@PYTHON3_CACHE_OPT1_EXT@: RNA/RNA.py RNA/_RNA$(PYTHON3_SO)
	$(AM_V_GEN)( PYTHONPATH="RNA:$$PYTHONPATH" $(PYTHON3) -O -c "import _RNA" ; \
  PYTHONPATH=".:$$PYTHONPATH" $(PYTHON3) -O -c "import RNA" )

RNA/__pycache__/RNA.@PYTHON3_CACHE_TAG@.pyc: RNA/RNA.py RNA/_RNA$(PYTHON3_SO)
	$(AM_V_GEN)( PYTHONPATH="RNA:$$PYTHONPATH" $(PYTHON3) -c "import _RNA" ; \
  PYTHONPATH=".:$$PYTHONPATH" $(PYTHON3) -c "from RNA import RNA" )

RNA/__pycache__/RNA.@PYTHON3_CACHE_OPT1_EXT@: RNA/RNA.py RNA/_RNA$(PYTHON3_SO)
	$(AM_V_GEN)( PYTHONPATH="RNA:$$PYTHONPATH" $(PYTHON3) -O -c "import _RNA" ; \
  PYTHONPATH=".:$$PYTHONPATH" $(PYTHON3) -O -c "from RNA import RNA" )

RNA/_RNA$(PYTHON3_SO): _RNA.la
	$(AM_V_GEN)( $(LIBTOOL) --config > libtoolconfig.tmp ) && \
  ( . ./libtoolconfig.tmp; cp $$objdir/_RNA$(PYTHON3_SO) RNA ) && \
  rm -f libtoolconfig.tmp


CLEANFILES = \
    RNA/_RNA$(PYTHON3_SO) \
    RNA/RNA.py \
    RNA/__pycache__/* \
    ViennaRNA/__pycache__/* \
    $(srcdir)/*.pyc

all-local:  RNA/_RNA$(PYTHON3_SO)

clean-local:
	-rm -rf $(CLEANFILES)

# Remove the .la file - RNA.la is never linked against (it's a module)
# and Python doesn't use libltdl.  Note that the library gets installed by
# install-data, so that's where we need to hook.
install-data-hook:
	rm -f $(DESTDIR)$(pkgpyexecdir)/_RNA.la

# Because we don't install the .la file, "make uninstall" doesn't work and
# we need to remove the file ourselves.
uninstall-local:
	eval `grep '^dlname=' $(pkgpyexec_LTLIBRARIES)` ; \
	rm -f $(DESTDIR)$(pkgpyexecdir)/"$$dlname"

## remove all generated content
maintainer-clean-local:
	-rm -f $(SWIG_wrapper) RNA.py documentation.i version.i

distclean-local:
	-rm -f $(SWIG_wrapper) RNA.py documentation.i version.i
