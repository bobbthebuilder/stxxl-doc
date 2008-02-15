# This -*- Makefile -*- is intended for processing with GNU make.

TOPDIR	?= $(error TOPDIR not defined) # DO NOT CHANGE! This is set elsewhere.

# Change this file according to your paths.

# Instead of modifying this file, you could also set your modified variables
# in make.settings.local (needs to be created first).
-include $(TOPDIR)/make.settings.local


USE_BOOST	?= no	# set 'yes' to use Boost libraries or 'no' to not use Boost libraries
USE_LINUX	?= yes	# set 'yes' if you run Linux, 'no' otherwise
USE_MCSTL	?= no	# will be overriden from main Makefile
USE_ICPC	?= no	# will be overriden from main Makefile

STXXL_ROOT	?= $(HOME)/work/stxxl

ifeq ($(strip $(USE_ICPC)),yes)
COMPILER	?= icpc
ICPC_MCSTL_CPPFLAGS	?= -gcc-version=420 -cxxlib#=$(FAKEGCC)
WARNINGS	?= -Wall -w1 -openmp-report0 -vec-report0
endif

ifeq ($(strip $(USE_MCSTL)),yes)
COMPILER	?= g++-4.2
LIBNAME		?= mcstxxl
# the root directory of your MCSTL installation
MCSTL_ROOT	?= $(HOME)/work/mcstl
endif

#BOOST_ROOT	?= /usr/local/boost-1.34.1

COMPILER	?= g++
LINKER		?= $(COMPILER)
OPT		?= -O3 # compiler optimization level
WARNINGS	?= -W -Wall
DEBUG		?= # put here -g option to include the debug information into the binaries

LIBNAME		?= stxxl


#### TROUBLESHOOTING
#
# For automatical checking of order of the output elements in
# the sorters: stxxl::stream::sort, stxxl::stream::merge_runs,
# stxxl::sort, and stxxl::ksort use
#
#STXXL_SPECIFIC	+= -DSTXXL_CHECK_ORDER_IN_SORTS
#
# If your program aborts with message "read/write: wrong parameter"
# or "Invalid argument"
# this could be that your kernel does not support direct I/O
# then try to set it off recompiling the libs and your code with option
#
#STXXL_SPECIFIC	+= -DSTXXL_DIRECT_IO_OFF
#
# But for the best performance it is strongly recommended
# to reconfigure the kernel for the support of the direct I/O.
#
# FIXME: documentation needed
#
#STXXL_SPECIFIC += -DSTXXL_DEBUGMON


#### You usually shouldn't need to change the sections below #####


#### STXXL OPTIONS ###############################################

# check, whether stxxl has been configured
ifeq (,$(strip $(wildcard $(STXXL_ROOT)/include/stxxl.h)))
$(warning *** WARNING: STXXL hasn't been configured correctly) #')
$(error ERROR: could not find a STXXL installation in STXXL_ROOT=$(STXXL_ROOT))
endif

# in the top dir, check whether STXXL_ROOT points to ourselves
ifneq (,$(wildcard make.settings))
stat1	 = $(shell stat -L -c '%d:%i' ./)
stat2	 = $(shell stat -L -c '%d:%i' $(STXXL_ROOT)/)

ifneq ($(stat1),$(stat2))
$(error ERROR: STXXL_ROOT=$(STXXL_ROOT) points to a different STXXL installation)
endif
endif

PTHREAD_FLAG	?= -pthread

STXXL_SPECIFIC	+= \
	$(PTHREAD_FLAG) \
	$(CPPFLAGS_ARCH) \
	-DSORT_OPTIMAL_PREFETCHING \
	-DUSE_MALLOC_LOCK \
	-DCOUNT_WAIT_TIME \
	-I$(strip $(STXXL_ROOT))/include \
	-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE \
	$(POSIX_MEMALIGN) $(XOPEN_SOURCE)

STXXL_LDFLAGS	+= $(PTHREAD_FLAG)
STXXL_LDLIBS	+= -L$(strip $(STXXL_ROOT))/lib -l$(LIBNAME)

STXXL_LIBDEPS	+= $(strip $(STXXL_ROOT))/lib/lib$(LIBNAME).$(LIBEXT)

UNAME_M		:= $(shell uname -m)
CPPFLAGS_ARCH	+= $(CPPFLAGS_$(UNAME_M))
CPPFLAGS_i686	?= -march=i686

##################################################################


#### ICPC OPTIONS ################################################

ifeq ($(strip $(USE_ICPC)),yes)

OPENMPFLAG	?= -openmp

STXXL_SPECIFIC	+= -include stxxl/bits/common/intel_compatibility.h

endif

##################################################################


#### MCSTL OPTIONS ###############################################

ifeq ($(strip $(USE_MCSTL)),yes)

OPENMPFLAG	?= -fopenmp

ifeq (,$(strip $(wildcard $(strip $(MCSTL_ROOT))/c++/mcstl.h)))
$(error ERROR: could not find a MCSTL installation in MCSTL_ROOT=$(MCSTL_ROOT))
endif

ifeq ($(strip $(USE_ICPC)),yes)
MCSTL_CPPFLAGS		+= $(ICPC_MCSTL_CPPFLAGS)
endif

MCSTL_CPPFLAGS		+= $(OPENMPFLAG) -D__MCSTL__ $(MCSTL_INCLUDES_PREPEND) -I$(MCSTL_ROOT)/c++
MCSTL_LDFLAGS		+= $(OPENMPFLAG)

MCSTL_INCLUDES_PREPEND	+= $(if $(findstring 4.3,$(COMPILER)),-I$(MCSTL_ROOT)/c++/mod_stl/gcc-4.3)

endif

##################################################################


#### BOOST OPTIONS ###############################################

BOOST_COMPILER_OPTIONS	 = \
	-DSTXXL_BOOST_TIMESTAMP \
	-DSTXXL_BOOST_CONFIG \
	-DSTXXL_BOOST_FILESYSTEM \
	-DSTXXL_BOOST_THREADS \
	-DSTXXL_BOOST_RANDOM \
	$(if $(strip $(BOOST_ROOT)),-I$(strip $(BOOST_ROOT)))

BOOST_LIB_COMPILER_SUFFIX	?= 
BOOST_LIB_MT_SUFFIX		?= -mt
BOOST_LINKER_OPTIONS		 = \
	-lboost_thread$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX) \
	-lboost_date_time$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX) \
	-lboost_iostreams$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX) \
	-lboost_filesystem$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX)

##################################################################


#### CPPUNIT OPTIONS ############################################

CPPUNIT_COMPILER_OPTIONS	+=

CPPUNIT_LINKER_OPTIONS		+= -lcppunit -ldl

##################################################################


#### DEPENDENCIES ################################################

HEADER_FILES_BITS	+= namespace.h noncopyable.h version.h

HEADER_FILES_COMMON	+= aligned_alloc.h mutex.h rand.h semaphore.h state.h
HEADER_FILES_COMMON	+= timer.h utils.h gprof.h rwlock.h simple_vector.h
HEADER_FILES_COMMON	+= switch.h tmeta.h log.h exceptions.h debug.h tuple.h
HEADER_FILES_COMMON	+= types.h utils_ledasm.h settings.h seed.h

HEADER_FILES_IO		+= completion_handler.h io.h iobase.h iostats.h
HEADER_FILES_IO		+= mmap_file.h simdisk_file.h syscall_file.h
HEADER_FILES_IO		+= ufs_file.h wincall_file.h wfs_file.h boostfd_file.h

HEADER_FILES_MNG	+= adaptor.h async_schedule.h block_prefetcher.h
HEADER_FILES_MNG	+= buf_istream.h buf_ostream.h buf_writer.h mng.h
HEADER_FILES_MNG	+= write_pool.h prefetch_pool.h

HEADER_FILES_CONTAINERS	+= pager.h stack.h vector.h priority_queue.h queue.h
HEADER_FILES_CONTAINERS	+= map.h deque.h

HEADER_FILES_CONTAINERS_BTREE	+= btree.h iterator_map.h leaf.h node_cache.h
HEADER_FILES_CONTAINERS_BTREE	+= root_node.h node.h btree_pager.h iterator.h

HEADER_FILES_ALGO	+= adaptor.h inmemsort.h intksort.h run_cursor.h sort.h
HEADER_FILES_ALGO	+= async_schedule.h interleaved_alloc.h ksort.h
HEADER_FILES_ALGO	+= losertree.h scan.h stable_ksort.h random_shuffle.h

HEADER_FILES_STREAM	+= stream.h sort_stream.h

HEADER_FILES_UTILS	+= malloc.h

###################################################################


#### MISC #########################################################

DEPEXT	 = $(LIBNAME).d # extension of dependency files
OBJEXT	 = $(LIBNAME).o	# extension of object files
LIBEXT	 = a		# static library file extension
EXEEXT	 = $(LIBNAME).bin # executable file extension
RM	 = rm -f	# remove file command
LIBGEN	 = ar cr	# library generation
OUT	 = -o		# output file option for the compiler and linker

d	?= $(strip $(DEPEXT))
o	?= $(strip $(OBJEXT))
bin	?= $(strip $(EXEEXT))

###################################################################


#### COMPILE/LINK RULES ###########################################

DEPS_MAKEFILES	:= $(wildcard ../Makefile.subdir.gnu ../make.settings ../make.settings.local GNUmakefile Makefile.local)
%.$o: %.cpp $(DEPS_MAKEFILES)
	@$(RM) $@ $*.$d
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -MD -MF $*.$dT -c $(OUTPUT_OPTION) $< && mv $*.$dT $*.$d

LINK_STXXL	 = $(LINKER) $1 $(STXXL_LINKER_OPTIONS) -o $@

%.$(bin): %.$o $(STXXL_LIBDEPS)
	$(call LINK_STXXL, $<)

###################################################################


STXXL_COMPILER_OPTIONS	+= $(STXXL_SPECIFIC)
STXXL_COMPILER_OPTIONS	+= $(OPT) $(DEBUG) $(WARNINGS)
STXXL_LINKER_OPTIONS	+= $(DEBUG)
STXXL_LINKER_OPTIONS	+= $(STXXL_LDFLAGS)
STXXL_LINKER_OPTIONS	+= $(STXXL_LDLIBS)

ifeq ($(strip $(USE_MCSTL)),yes)
STXXL_COMPILER_OPTIONS	+= $(MCSTL_CPPFLAGS)
STXXL_LINKER_OPTIONS	+= $(MCSTL_LDFLAGS)
endif

ifeq ($(strip $(USE_BOOST)),yes)
STXXL_COMPILER_OPTIONS	+= $(BOOST_COMPILER_OPTIONS)
STXXL_LINKER_OPTIONS	+= $(BOOST_LINKER_OPTIONS)
endif

STXXL_COMPILER_OPTIONS	+= $(CPPFLAGS)

# vim: syn=make
