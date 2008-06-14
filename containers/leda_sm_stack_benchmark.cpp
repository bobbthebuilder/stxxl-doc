/***************************************************************************
 *            leda_sm_stack_benchmark.cpp
 *
 *  Wed Jul 12 18:28:43 2006
 *  Copyright  2006  User Roman Dementiev
 *  Email
 ****************************************************************************/


//! \example containers/leda_sm_stack_benchmark.cpp
//! This is a benchmark mentioned in the paper
//! R. Dementiev, L. Kettner, P. Sanders "STXXL: standard template library for XXL data sets"
//! Software: Practice and Experience
//! Volume 38, Issue 6, Pages 589-637, May 2008
//! DOI: 10.1002/spe.844


#include <LEDA-SM/ext_stack.h>
#include <LEDA-SM/ext_memory_manager.h>
#include <LEDA-SM/debug.h>
#include <cassert>
#include <LEDA/random_source.h>
#include <LEDA/stack.h>
#include <LEDA-SM/block.h>
#include <LEDA-SM/name_server.h>
#define DEBUG 0
#define DD 500

#include "stxxl/bits/common/utils_ledasm.h"
#include "stxxl/timer"



#define MEM_2_RESERVE    (768 * 1024 * 1024)


#define    BLOCK_SIZE1 (EXT_BLK_SZ * 4)
#define    BLOCK_SIZE2 (DISK_BLOCK_SIZE * 4)


#ifndef DISKS
 #define DISKS 1
#endif

template <unsigned RECORD_SIZE>
struct my_record_
{
    char data[RECORD_SIZE];
    my_record_() { }
};


template <class my_record>
void run_stack(stxxl::int64 volume)
{
    typedef ext_stack<my_record> stack_type;

    STXXL_MSG("Record size: " << sizeof(my_record) << " bytes");

    stack_type Stack;

    stxxl::int64 ops = volume / sizeof(my_record);

    stxxl::int64 i;

    my_record cur;

    stxxl::timer Timer;
    Timer.start();

    for (i = 0; i < ops; ++i)
    {
        Stack.push(cur);
    }

    Timer.stop();

    STXXL_MSG("Records in Stack: " << Stack.size());
    if (i != Stack.size())
    {
        STXXL_MSG("Size does not match");
        abort();
    }

    STXXL_MSG("Insertions elapsed time: " << (Timer.mseconds() / 1000.) <<
              " seconds : " << (double(volume) / (1024. * 1024. * Timer.mseconds() / 1000.)) <<
              " MB/s");

    ext_mem_mgr.print_statistics();
    ext_mem_mgr.reset_statistics();


    ////////////////////////////////////////////////
    Timer.reset();
    Timer.start();

    for (i = 0; i < ops; ++i)
    {
        Stack.pop();
    }

    Timer.stop();

    STXXL_MSG("Records in Stack: " << Stack.size());
    if (!Stack.empty())
    {
        STXXL_MSG("Stack must be empty");
        abort();
    }

    STXXL_MSG("Deletions elapsed time: " << (Timer.mseconds() / 1000.) <<
              " seconds : " << (double(volume) / (1024. * 1024. * Timer.mseconds() / 1000.)) <<
              " MB/s");

    ext_mem_mgr.print_statistics();
}



int main(int argc, char * argv[])
{
    STXXL_MSG("block size 1: " << BLOCK_SIZE1 << " bytes");
    STXXL_MSG("block size 2: " << BLOCK_SIZE2 << " bytes");


    if (argc < 3)
    {
        STXXL_MSG("Usage: " << argv[0] << " version #volume");
        STXXL_MSG("\t version = 1: LEDA-SM stack with 4 byte records");
        STXXL_MSG("\t version = 2: LEDA-SM stack with 32 byte records");
        return 0;
    }

    int version = atoi(argv[1]);
    stxxl::int64 volume = atoll(argv[2]);

    STXXL_MSG("Allocating array with size " << MEM_2_RESERVE
                                            << " bytes to prevent file buffering.");
    int * array = new int[MEM_2_RESERVE / sizeof(int)];
    std::fill(array, array + (MEM_2_RESERVE / sizeof(int)), 0);

    STXXL_MSG("Running version: " << version);
    STXXL_MSG("Data volume    : " << volume << " bytes");

    switch (version)
    {
    case 1:
        run_stack<my_record_<4> >(volume);
        break;
    case 2:
        run_stack<my_record_<32> >(volume);
        break;
    default:
        STXXL_MSG("Unsupported version " << version);
    }

    delete[] array;
}
