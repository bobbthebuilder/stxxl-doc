###############################################################################
# CMakeLists.txt
#
# Cmake file for stxxl/tests/io/
#
# Part of the STXXL. See http://stxxl.sourceforge.net
#
# Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
###############################################################################

stxxl_build_test(test_cancel)
stxxl_build_test(test_io)
stxxl_build_test(test_io_sizes)

stxxl_test(test_io "${STXXL_TMPDIR}")

stxxl_test(test_cancel syscall "${STXXL_TMPDIR}/testdisk1")
# FIXME: clean up after fileperblock_syscall
stxxl_test(test_cancel fileperblock_syscall "${STXXL_TMPDIR}/testdisk1")
stxxl_test(test_cancel mmap "${STXXL_TMPDIR}/testdisk1")
stxxl_test(test_cancel fileperblock_mmap "${STXXL_TMPDIR}/testdisk1")
stxxl_test(test_cancel simdisk "${STXXL_TMPDIR}/testdisk1")
if(USE_BOOST)
  stxxl_test(test_cancel boostfd "${STXXL_TMPDIR}/testdisk1")
  stxxl_test(test_cancel fileperblock_boostfd "${STXXL_TMPDIR}/testdisk1")
endif(USE_BOOST)
stxxl_test(test_cancel memory "${STXXL_TMPDIR}/testdisk1")
# FIXME: asserts!
#stxxl_test(test_cancel wbtl "${STXXL_TMPDIR}/testdisk1")

stxxl_test(test_io_sizes syscall "${STXXL_TMPDIR}/testdisk1" 2147483648)
stxxl_test(test_io_sizes mmap "${STXXL_TMPDIR}/testdisk1" 2147483648)
if(USE_BOOST)
  stxxl_test(test_io_sizes boostfd "${STXXL_TMPDIR}/testdisk1" 2147483648)
endif(USE_BOOST)

# unit tests
if(CPPUNIT_FOUND AND BUILD_TESTS)

  add_executable(unittest_io unittest.cpp)
  target_link_libraries(unittest_io ${STXXL_LIBS} ${CPPUNIT_LIBRARIES})
  stxxl_test(unittest_io)

endif(CPPUNIT_FOUND AND BUILD_TESTS)
