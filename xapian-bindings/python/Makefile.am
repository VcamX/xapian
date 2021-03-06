## Process this file with automake to produce Makefile.in

include ../generic/generic.mk

## Test programs to be run
TESTS = smoketest.py pythontest.py
AM_TESTS_ENVIRONMENT = \
	abs_builddir='$(abs_builddir)' ;\
	srcdir='$(srcdir)' ;\
	PYTHON='$(PYTHON2)' ;\
	export abs_builddir ;\
	export srcdir ;\
	export PYTHON ;
LOG_COMPILER = '$(srcdir)'/run-python-test

RST_DOCS = \
	docs/examples.rst \
	docs/index.rst \
	docs/introduction.rst \
	docs/xapian.rst

# FIXME: replicationtest.py attempts to test performing replication whilst
# modifications are in progress.  However, it sometimes fails due to a race
# condition in it.  It also contains hard-coded paths to the replication
# server and client, which assume the file layout in a repo checkout.
# Therefore, we don't run it by default currently.
# replicationtest.py

BUILT_SOURCES = modern/xapian_wrap.cc modern/xapian_wrap.h modern/xapian.py

EXTRA_DIST = python.i util.i extra.i extracomments.i except.i \
	generate-python-exceptions \
	module_top.py \
	run-python-test \
	testsuite.py \
	test_xapian_star.py \
	replicationtest.py \
	$(TESTS) $(BUILT_SOURCES) $(RST_DOCS)

pkgpylibdir = @PYTHON2_LIB@/xapian

# Install as _DATA rather than _SCRIPTS because we don't want to make these
# executable (they don't have a #! line).
pkgpylib_DATA = xapian/__init__.py \
		xapian/__init__.pyc \
		xapian/__init__.pyo

pkgpylib_LTLIBRARIES = _xapian.la

# Remove the .la file - _xapian.la is never linked against (it's a module)
# and Python doesn't use libltdl.  Note that the library gets installed by
# install-data, so that's where we need to hook.
install-data-hook:
	rm -f $(DESTDIR)$(pkgpylibdir)/_xapian.la

# Because we don't install the .la file, "make uninstall" doesn't work and
# we need to remove the file ourselves.
uninstall-local:
	rm -f $(DESTDIR)$(pkgpylibdir)/_xapian$(PYTHON2_SO)

AM_CPPFLAGS = -I$(PYTHON2_INC)
AM_CXXFLAGS = $(SWIG_CXXFLAGS) $(XAPIAN_CXXFLAGS)
_xapian_la_LDFLAGS = -avoid-version -module -shrext "$(PYTHON2_SO)" $(NO_UNDEFINED)
_xapian_la_SOURCES = modern/xapian_wrap.cc
_xapian_la_LIBADD = $(XAPIAN_LIBS) $(PYTHON2_LIBS)

# In a distribution, xapian.py is in $(srcdir)/modern, and python will always
# generate the .pyc and .pyo files in the same directory as the .py file.
# However, $(srcdir) might be shared between builds for different python
# versions so we can't let the .pyc and .pyo files get written there.  Hence we
# copy the .py file to the build directory from wherever it actually is and use
# the copy.  We also rename the copy to __init__.py so that it can be installed
# in a "xapian" directory and be treated as a package.  Finally, we take this
# opportunity to insert a module doc string at the top of the file.
xapian/__init__.py: modern/xapian.py
	$(MKDIR_P) xapian
	cat '$(srcdir)/module_top.py' `test -f modern/xapian.py || echo '$(srcdir)/'`modern/xapian.py > xapian/__init__.pyT
	mv xapian/__init__.pyT xapian/__init__.py

# We "import _xapian" first so that if we fail to import the glue library
# we don't generate a broken .pyc or .pyo.
xapian/__init__.pyc: xapian/__init__.py xapian/_xapian$(PYTHON2_SO)
	PYTHONPATH="xapian:$$PYTHONPATH" $(PYTHON2) -c "import _xapian"
	PYTHONPATH=".:$$PYTHONPATH" $(PYTHON2) -c "import xapian"

xapian/__init__.pyo: xapian/__init__.py xapian/_xapian$(PYTHON2_SO)
	PYTHONPATH="xapian:$$PYTHONPATH" $(PYTHON2) -O -c "import _xapian"
	PYTHONPATH=".:$$PYTHONPATH" $(PYTHON2) -O -c "import xapian"

xapian/_xapian$(PYTHON2_SO): _xapian.la
	$(MKDIR_P) xapian
	$(LIBTOOL) --config > libtoolconfig.tmp
## ksh requires a path on the sourced file.
	. ./libtoolconfig.tmp; cp $$objdir/_xapian$(PYTHON2_SO) xapian
	rm -f libtoolconfig.tmp

CLEANFILES = \
    xapian/_xapian$(PYTHON2_SO) \
    xapian/__init__.py xapian/__init__.pyc xapian/__init__.pyo \
    $(srcdir)/*.pyc

# Clean the xapian directory which we created, if it's empty, and any
# databases created by test cases.
clean-local:
	-rmdir xapian
	rm -rf db_test_* dbs_replication

if MAINTAINER_MODE
BUILT_SOURCES += except.i doccomments.i
except.i: $(srcdir)/generate-python-exceptions ../../xapian-core/exception_data.pm
	$(PERL) -w -I$(srcdir)/../../xapian-core $(srcdir)/generate-python-exceptions

if MAINTAINER_NO_DOCS
doccomments.i:
	: > doccomments.i
else
doccomments.i: doxy2swig.py ../../xapian-core/exception_data.pm ../doxygen-xml/index.xml
	$(PYTHON) $(srcdir)/doxy2swig.py ../doxygen-xml/index.xml doccomments.i
endif

# We need to explicitly set -outdir because on Windows, SWIG splits paths at
# "\" when extracting the output directory from the value passed to the -o
# option.

stamp = modern/xapian_wrap.stamp
RUN_SWIG = stamp='$(stamp)' $(PERL) '$(top_srcdir)'/swig-depcomp $(SWIG)
SWIG_FLAGS += -DSWIG_PYTHON_LEGACY_BOOL

modern/xapian_wrap.cc modern/xapian_wrap.h modern/xapian.py modern/xapian_wrap.d: $(stamp)
	$(make_many_locked)
$(stamp): except.i doccomments.i
	$(MKDIR_P) modern
	$(multitarget_begin)
	$(RUN_SWIG) $(SWIG_WERROR) -I. -I'$(srcdir)' $(SWIG_FLAGS) -c++ \
	    -python -threads -shadow -modern -O -outdir modern \
	    -o modern/xapian_wrap.cc '$(srcdir)/'python.i
	$(PERL) -pi -e 's/class Error:/class Error(Exception):/' modern/xapian.py
	$(multitarget_end)

-include modern/xapian_wrap.d

CLEANFILES += modern/xapian_wrap.d $(stamp)
endif
MAINTAINERCLEANFILES = $(BUILT_SOURCES)

exampledatadir = $(docdir)/python/examples
dist_exampledata_DATA = \
	docs/examples/simpleindex.py \
	docs/examples/simpleexpand.py \
	docs/examples/simplematchdecider.py \
	docs/examples/simplesearch.py

sphinxdocs = docs/html/index.html

dist-hook:
	$(MKDIR_P) '$(distdir)/docs'
	cp -R -p docs/html $(distdir)/docs

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/python
	cp -R -p docs/html $(DESTDIR)$(docdir)/python

if DOCUMENTATION_RULES
all-local: $(sphinxdocs)

$(sphinxdocs): xapian/__init__.py docs/conf.py $(RST_DOCS) $(dist_exampledata_DATA)
	PYTHONPATH=..:$$PYTHONPATH $(PYTHON2) $(SPHINX_BUILD) -b html -d doctrees -c docs $(srcdir)/docs docs/html
endif
