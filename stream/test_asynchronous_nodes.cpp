/***************************************************************************
 *  stream/test_asynchronous_nodes.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example stream/test_asynchronous_nodes.cpp
//! This is an example of how to use the asynchronous nodes needed for
//! parallel pipelining.

#define STXXL_START_PIPELINE_DEFERRED 1
#define STXXL_STREAM_SORT_ASYNCHRONOUS_PULL 1

#define PIPELINED 1
#define BATCHED 1
#define SYMMETRIC 1

#define OUTPUT_STATS 1

//#define STXXL_VERBOSE_LEVEL 0

#include "test_asynchronous_pipelining_common.h"
#include <vector>
#include <stxxl/stream>
#include <stxxl/bits/stream/stream.h>
#include <stxxl/vector>

stxxl::unsigned_type memory_to_use = 512 * megabyte;
stxxl::unsigned_type run_size = memory_to_use / 4;
stxxl::unsigned_type buffer_size = 16 * megabyte;

void double_diamond(vector_type & input, bool asynchronous_pull, stxxl::stream::StartMode start_mode, bool wait_for_stop)
{
    using stxxl::stream::generator2stream;
    using stxxl::stream::round_robin;
    using stxxl::stream::streamify;
    using stxxl::stream::async::pull;
    using stxxl::stream::async::pull_batch;
    using stxxl::stream::async::dummy_pull;
    using stxxl::stream::async::push;
    using stxxl::stream::async::push_batch;
    using stxxl::stream::async::dummy_push;
    using stxxl::stream::async::push_pull;
    using stxxl::stream::transform;
    using stxxl::stream::sort;
    using stxxl::stream::runs_creator;
    using stxxl::stream::runs_creator_batch;
    using stxxl::stream::startable_runs_merger;
    using stxxl::stream::make_tuple;
    using stxxl::stream::use_push;

    stxxl::stats_data stats_begin(*stxxl::stats::get_instance());

    vector_tuple_type tuple_output(input.size());
    accumulate<my_type> acc, acc_left, acc_right;
    accumulate_tuple<my_type> acc_tuple;

    vector_tuple_type::iterator o;
    {
        STXXL_MSG("Double Diamond...");

//input

#ifdef BOOST_MSVC
        typedef stxxl::stream::streamify_traits<vector_type::iterator>::stream_type input_stream_type;
#else
        typedef __typeof__(streamify(input.begin(), input.end())) input_stream_type;
#endif //BOOST_MSVC
        input_stream_type input_stream = streamify(input.begin(), input.end()); //0

        typedef cmp_less_load cmp_4_type;
        typedef cmp_greater_load cmp_10_type;
        typedef cmp_less_key cmp_7_type;
        typedef cmp_greater_key cmp_13_type;

        cmp_4_type cmp_4;
        cmp_10_type cmp_10;
        cmp_7_type cmp_7;
        cmp_13_type cmp_13;

//accumulate1

        typedef transform<accumulate<my_type>, input_stream_type> accumulate_stream_type;
        accumulate_stream_type accumulate_stream(acc, input_stream);    //1

//right flow

#if SYMMETRIC
        typedef PUSH_PULL<my_type> runs_creator_stream_node1_type;
        runs_creator_stream_node1_type runs_creator_stream_node1(buffer_size, start_mode);        //9
        stxxl::STXXL_UNUSED(wait_for_stop);
#else
        typedef runs_creator<use_push<my_type>, cmp_10_type, block_size, STXXL_DEFAULT_ALLOC_STRATEGY> runs_creator_stream1_type;
        runs_creator_stream1_type runs_creator_stream1(cmp_10, run_size, asynchronous_pull, wait_for_stop);       //10a

#if PIPELINED
        //runs_creator<use_push> will not pull asynchronously
        typedef PUSH_BATCH<runs_creator_stream1_type> runs_creator_stream_node1_type;
        runs_creator_stream_node1_type runs_creator_stream_node1(run_size, runs_creator_stream1, asynchronous_pull, start_mode);     //9
#else
        typedef runs_creator_stream1_type runs_creator_stream_node1_type;
        runs_creator_stream_node1_type & runs_creator_stream_node1 = runs_creator_stream1;            //renaming
#endif

#endif

//split

        typedef transform<split2<my_type, runs_creator_stream_node1_type>, accumulate_stream_type> split2_stream_type;
        split2<my_type, runs_creator_stream_node1_type> s2(runs_creator_stream_node1);
        split2_stream_type split2_stream(s2, accumulate_stream);                //2

#if PIPELINED
        typedef PULL<split2_stream_type> split2_stream_node_type;
        split2_stream_node_type split2_stream_node(run_size, split2_stream, start_mode);  //3
#else
        typedef split2_stream_type split2_stream_node_type;
        split2_stream_node_type & split2_stream_node = split2_stream;         //renaming
#endif

//left flow

#if PIPELINED && BATCHED
        typedef sort<split2_stream_node_type, cmp_4_type, block_size, STXXL_DEFAULT_ALLOC_STRATEGY, runs_creator_batch<split2_stream_node_type, cmp_4_type, block_size, STXXL_DEFAULT_ALLOC_STRATEGY> > sort_left_stream1_type;
#else
        typedef sort<split2_stream_node_type, cmp_4_type, block_size> sort_left_stream1_type;
#endif

        sort_left_stream1_type sort_left_stream1(split2_stream_node, cmp_4, run_size, asynchronous_pull, start_mode); //4


        typedef transform<accumulate<my_type>, sort_left_stream1_type> left_modifier_stream_type;
        left_modifier_stream_type left_modifier_stream(acc_left, sort_left_stream1);    //5


#if PIPELINED
        typedef PULL_BATCH<left_modifier_stream_type> left_modifier_stream_node_type;
        left_modifier_stream_node_type left_modifier_stream_node(buffer_size, left_modifier_stream, start_mode);  //6
#else
        typedef left_modifier_stream_type left_modifier_stream_node_type;
        left_modifier_stream_node_type & left_modifier_stream_node = left_modifier_stream;            //renaming
#endif


        STXXL_MSG("1/3 break in the DAG reached");

//right flow

#if SYMMETRIC

#if PIPELINED && BATCHED
        typedef sort<runs_creator_stream_node1_type, cmp_10_type, block_size, STXXL_DEFAULT_ALLOC_STRATEGY, runs_creator_batch<runs_creator_stream_node1_type, cmp_10_type, block_size, STXXL_DEFAULT_ALLOC_STRATEGY> > sort_right_stream1_type;
#else
        typedef sort<runs_creator_stream_node1_type, cmp_10_type, block_size> sort_right_stream1_type;
#endif

        sort_right_stream1_type sort_right_stream1(runs_creator_stream_node1, cmp_10, run_size, asynchronous_pull, start_mode);       //10

#else

        typedef startable_runs_merger<runs_creator_stream1_type, cmp_10_type> sort_right_stream1_type;
        sort_right_stream1_type sort_right_stream1(runs_creator_stream1, cmp_10, run_size, start_mode); //10a

#endif

        typedef transform<accumulate<my_type>, sort_right_stream1_type> right_modifier_stream_type;
        right_modifier_stream_type right_modifier_stream(acc_right, sort_right_stream1);                        //11

#if PIPELINED
        typedef PULL_BATCH<right_modifier_stream_type> right_modifier_stream_node_type;
        right_modifier_stream_node_type right_modifier_stream_node(buffer_size, right_modifier_stream, start_mode);       //12
#else
        typedef right_modifier_stream_type right_modifier_stream_node_type;
        right_modifier_stream_node_type & right_modifier_stream_node = right_modifier_stream;                 //renaming
#endif


//left flow
#if PIPELINED && BATCHED
        typedef sort<left_modifier_stream_node_type, cmp_7_type, block_size, STXXL_DEFAULT_ALLOC_STRATEGY, runs_creator_batch<left_modifier_stream_node_type, cmp_7_type, block_size, STXXL_DEFAULT_ALLOC_STRATEGY> > sort_left_stream2_type;
#else
        typedef sort<left_modifier_stream_node_type, cmp_7_type, block_size> sort_left_stream2_type;
#endif

        sort_left_stream2_type sort_left_stream2(left_modifier_stream_node, cmp_7, run_size, asynchronous_pull, start_mode);  //7

#if PIPELINED
        typedef PULL_BATCH<sort_left_stream2_type> sort_left_stream_node2_type;
        sort_left_stream_node2_type sort_left_stream_node2(buffer_size, sort_left_stream2, start_mode);   //8
#else
        typedef sort_left_stream2_type sort_left_stream_node2_type;
        sort_left_stream_node2_type & sort_left_stream_node2 = sort_left_stream2;             //renaming
#endif

        STXXL_MSG("1/3 break in the DAG reached");

//right flow

#if SYMMETRIC
#else
        typedef runs_creator<right_modifier_stream_node_type, cmp_13_type, block_size, STXXL_DEFAULT_ALLOC_STRATEGY> runs_creator_stream2_type;
        runs_creator_stream2_type runs_creator_stream2(right_modifier_stream_node, cmp_13, run_size, asynchronous_pull, start_mode);  //13a
#endif


//right flow

#if SYMMETRIC

#if PIPELINED && BATCHED
        typedef sort<right_modifier_stream_node_type, cmp_13_type, block_size, STXXL_DEFAULT_ALLOC_STRATEGY,
                     runs_creator_batch<right_modifier_stream_node_type, cmp_13_type, block_size, STXXL_DEFAULT_ALLOC_STRATEGY>
                     > sort_right_stream2_type;
#else
        typedef sort<right_modifier_stream_node_type, cmp_13_type, block_size> sort_right_stream2_type;
#endif

        sort_right_stream2_type sort_right_stream2(right_modifier_stream_node, cmp_13, run_size, asynchronous_pull, start_mode);      //13


#else
        typedef startable_runs_merger<runs_creator_stream2_type, cmp_13_type> sort_right_stream2_type;
        sort_right_stream2_type sort_right_stream2(runs_creator_stream2, cmp_13, run_size, asynchronous_pull, start_mode);     //13b

#endif

#if PIPELINED
        typedef PULL_BATCH<sort_right_stream2_type> sort_right_stream_node2_type;
        sort_right_stream_node2_type sort_right_stream_node2(buffer_size, sort_right_stream2, start_mode);        //14
#else
        typedef sort_right_stream2_type sort_right_stream_node2_type;
        sort_right_stream_node2_type & sort_right_stream_node2 = sort_right_stream2;                  //renaming
#endif


//join

        typedef make_tuple<sort_left_stream_node2_type, sort_right_stream_node2_type> make_tuple_stream_type;
        make_tuple_stream_type make_tuple_stream(sort_left_stream_node2, sort_right_stream_node2);    //15


        typedef transform<accumulate_tuple<my_type>, make_tuple_stream_type> accumulate_tuple_stream_type;
        accumulate_tuple_stream_type accumulate_tuple_stream(acc_tuple, make_tuple_stream);             //16


#if BATCHED
        o = materialize_batch(accumulate_tuple_stream, tuple_output.begin(), tuple_output.end(), 0, start_mode);       //17
#else
        o = materialize(accumulate_tuple_stream, tuple_output.begin(), tuple_output.end(), 0, start_mode);             //17
#endif
    }
    assert(o == tuple_output.end());
#define STREAM_OUT tuple_output

#if OUTPUT_STATS
    std::cout << stxxl::stats_data(*stxxl::stats::get_instance()) - stats_begin;
#endif

    if (acc.result() != acc_tuple.result().first || acc.result() != acc_tuple.result().second)
    {
        STXXL_MSG("WRONG DATA");
        std::cout << acc.result() << std::endl;
        std::cout << acc_tuple.result().first << " - " << acc_tuple.result().second << " = " << ((unsigned long long)acc_tuple.result().first - (unsigned long long)acc_tuple.result().second) << std::endl;
    }
    else
        std::cout << acc.result() << std::endl;

    bool is_sorted;

    stats_begin = *stxxl::stats::get_instance();
    {

#define STREAMED_CHECKING 0
#if STREAMED_CHECKING
        typedef __typeof__(streamify(tuple_output.begin(), tuple_output.end())) output_stream_type;

        output_stream_type * output_stream;

        output_stream = new output_stream_type(tuple_output.begin(), tuple_output.end());

        typedef transform<check_order<my_tuple, cmp_less_tuple>, output_stream_type> check_order_stream_type;
        cmp_less_tuple clt;
        check_order<my_tuple, cmp_less_tuple> co(clt);
        check_order_stream_type check_order_stream(co, *output_stream);

#if BATCHED
        pull_batch(check_order_stream, tuple_output.size());
#else
        pull(check_order_stream, tuple_output.size());
#endif
        delete output_stream;

        is_sorted = co.result();
#else
        is_sorted = stxxl::is_sorted(STREAM_OUT.begin(), STREAM_OUT.end(), cmp_less_tuple());
#endif
    }

#if OUTPUT_STATS
    std::cout << stxxl::stats_data(*stxxl::stats::get_instance()) - stats_begin;
#endif
    if (!is_sorted)
    {
        vector_tuple_type::const_iterator i = STREAM_OUT.begin(), last = STREAM_OUT.begin();
        for ( ; i != STREAM_OUT.begin() + 1000; i++)
            std::cout << *i << "   ";
        std::cout << std::endl;

        for (i = STREAM_OUT.begin(); i != STREAM_OUT.end(); i++)
        {
            if (cmp_less_tuple() (*i, *last))
                std::cout << std::endl << "Wrong @" << (i - STREAM_OUT.begin()) << ":\t" << *last << " > " << *i << std::endl;
            last = i;
        }
    }

    STXXL_MSG((is_sorted ? "OK" : "NOT SORTED"));

    //STXXL_MSG("Done, tuple_output size="<<input.size())
}

int main()
{
    const int megabytes_to_process = 1024;
    const stxxl::int64 n_records =
        stxxl::int64(megabytes_to_process) * stxxl::int64(megabyte) / sizeof(my_type);
    vector_type input(n_records);

#if PIPELINED
    std::cout << "PIPELINED" << std::endl;
#endif
#if BATCHED
    std::cout << "BATCHED" << std::endl;
#endif
#if SYMMETRIC
    std::cout << "SYMMETRIC" << std::endl;
#endif

    int seed = 1000;

    STXXL_MSG("Filling vector..., input size =" << input.size());

    random_my_type rnd(seed);

    stxxl::stats_data stats_begin(*stxxl::stats::get_instance());

    stxxl::generate(input.begin(), input.end(), rnd, memory_to_use / STXXL_DEFAULT_BLOCK_SIZE(my_type));

#if OUTPUT_STATS
    std::cout << stxxl::stats_data(*stxxl::stats::get_instance()) - stats_begin;
#endif

    double_diamond(input, true, stxxl::stream::start_deferred, true);

    return 0;
}
