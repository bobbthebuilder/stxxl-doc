# This -*- Makefile -*- is intended for processing with GNU make.
THISMAKEFILE	:= $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)) 

main: library

include make.settings

SUBDIRS	= algo common containers io lib mng stream utils

SUBDIRS-lib: $(SUBDIRS:%=lib-in-%)
SUBDIRS-tests: $(SUBDIRS:%=tests-in-%)
SUBDIRS-clean: $(SUBDIRS:%=clean-in-%)

# compute STXXL_CPPFLAGS/STXXL_LDLIBS for stxxl.mk
# don't include optimization, warning and debug flags
stxxl_mk_cppflags	 = $$(STXXL_CPPFLAGS_STXXL)
stxxl_mk_ldlibs		 = $$(STXXL_LDLIBS_STXXL)
ifeq ($(strip $(USE_MCSTL)),yes)
stxxl_mk_cppflags	+= $$(STXXL_CPPFLAGS_MCSTL)
stxxl_mk_ldlibs		+= $$(STXXL_LDLIBS_MCSTL)
endif
ifeq ($(strip $(USE_BOOST)),yes)
stxxl_mk_cppflags	+= $$(STXXL_CPPFLAGS_BOOST)
stxxl_mk_ldlibs		+= $$(STXXL_LDLIBS_BOOST)
endif

lib-in-lib:
	@# nothing to compile
lib-in-%:
	$(MAKE) -C $* lib

build-lib: SUBDIRS-lib
	$(MAKE) -C lib
	$(MAKE) -C utils create

$(LIBNAME).mk: build-lib
	echo 'STXXL_CXX	 = $(COMPILER)'	> $@
	echo 'STXXL_CPPFLAGS	 = $(stxxl_mk_cppflags)'	>> $@
	echo 'STXXL_LDLIBS	 = $(stxxl_mk_ldlibs)'	>> $@
	echo 'STXXL_CPPFLAGS_STXXL	 = $(STXXL_SPECIFIC)'	>> $@
	echo 'STXXL_LDLIBS_STXXL	 = $(STXXL_LDLIBS)'	>> $@
	echo 'STXXL_LIBDEPS		 = $(STXXL_LIBDEPS)'	>> $@
	echo 'STXXL_CPPFLAGS_MCSTL	 = $(MCSTL_CPPFLAGS)'	>> $@
	echo 'STXXL_LDLIBS_MCSTL	 = $(MCSTL_LDFLAGS)'	>> $@
	echo 'STXXL_CPPFLAGS_BOOST	 = $(BOOST_COMPILER_OPTIONS)'	>> $@
	echo 'STXXL_LDLIBS_BOOST	 = $(BOOST_LINKER_OPTIONS)'	>> $@

library: $(LIBNAME).mk
	@echo ""
	@echo "Building library is completed."
	@echo "Use the following compiler options with programs using Stxxl: $(STXXL_COMPILER_OPTIONS)"
	@echo "Use the following linker options with programs using Stxxl: $(STXXL_LINKER_OPTIONS)"

lib/lib$(LIBNAME).$(LIBEXT): make.settings
	$(MAKE) -f $(THISMAKEFILE) library

ifneq (,$(wildcard .svn))
lib-in-common: common/version_svn.defs

ifeq (,$(strip $(shell svnversion . | tr -d 0-9)))
# clean checkout - use svn info
VERSION_DATE	:= $(shell LC_ALL=POSIX svn info . | sed -ne '/Last Changed Date/{s/.*: //;s/ .*//;s/-//gp}')
VERSION_SVN_REV	:= $(shell LC_ALL=POSIX svn info . | sed -ne '/Last Changed Rev/s/.*: //p')
else
# modified, mixed, ... checkout - use svnversion and today
VERSION_DATE	:= $(shell date "+%Y%m%d")
VERSION_SVN_REV	:= $(shell svnversion .)
endif

.PHONY: common/version_svn.defs
common/version_svn.defs:
	echo '#define STXXL_VERSION_STRING_DATE "$(VERSION_DATE)"' > $@.tmp
	echo '#define STXXL_VERSION_STRING_SVN_REVISION "$(VERSION_SVN_REV)"' >> $@.tmp
	cmp -s $@ $@.tmp || mv $@.tmp $@
	$(RM) $@.tmp
endif

tests-in-%: lib/lib$(LIBNAME).$(LIBEXT)
	$(MAKE) -C $* tests

tests: SUBDIRS-tests
	@echo ""
	@echo "Building tests is completed."
	@echo "Use the following compiler options with programs using Stxxl: $(STXXL_COMPILER_OPTIONS)"
	@echo "Use the following linker options with programs using Stxxl: $(STXXL_LINKER_OPTIONS)"

clean-in-%:
	$(MAKE) -C $* clean

clean: SUBDIRS-clean
	$(RM) common/version_svn.defs
	$(RM) $(LIBNAME).mk
	$(RM) compiler.options linker.options
	@echo "Cleaning completed"

.PHONY: main library tests clean
