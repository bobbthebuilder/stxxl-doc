/***************************************************************************
 *  io/unittest.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TestRunner.h>
#include <stxxl.h>

using namespace stxxl;

struct my_handler
{
    void operator () (request * ptr)
    {
        STXXL_MSG("Request completed: " << ptr);
    }
};


class IOLayerTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(IOLayerTest);
    CPPUNIT_TEST(testIO);
    CPPUNIT_TEST_EXCEPTION(testIOException, stxxl::io_error);
    CPPUNIT_TEST_SUITE_END();

public:
    IOLayerTest(std::string name) : CppUnit::TestCase(name) { }
    IOLayerTest() { }

    void testIO()
    {
        const int size = 1024 * 384;
        char * buffer = static_cast<char *>(stxxl::aligned_alloc<BLOCK_ALIGN>(size));
        memset(buffer, 0, size);
#ifdef BOOST_MSVC
        const char * paths[2] = { "data1", "data2" };
#else
        const char * paths[2] = { "/var/tmp/data1", "/var/tmp/data2" };
        mmap_file file1(paths[0], file::CREAT | file::RDWR, 0);
        file1.set_size(size * 1024);
#endif

        syscall_file file2(paths[1], file::CREAT | file::RDWR, 1);

        request_ptr req[16];
        unsigned i = 0;
        for ( ; i < 16; i++)
            req[i] = file2.awrite(buffer, i * size, size, my_handler());


        wait_all(req, 16);

        stxxl::aligned_dealloc<BLOCK_ALIGN>(buffer);
    }

    void testIOException()
    {
        unlink("TestFile");
#ifdef BOOST_MSVC
        mmap_file file1("TestFile", file::RDWR, 0);
#else
        mmap_file file1("TestFile", file::RDWR, 0);
#endif
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(IOLayerTest);


int main()
{
    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry & registry = CppUnit::TestFactoryRegistry::getRegistry();
    runner.addTest(registry.makeTest());
    bool wasSuccessful = runner.run("", false);
    return wasSuccessful;
}
