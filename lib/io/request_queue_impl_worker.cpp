/***************************************************************************
 *  io/request_queue_impl_worker.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/request_queue_impl_worker.h>
#include <stxxl/bits/io/request.h>

#if STXXL_BOOST_THREADS
 #include <boost/bind.hpp>
#endif

#include <iostream>
__STXXL_BEGIN_NAMESPACE

void request_queue_impl_worker::start_thread(void * (*worker)(void *), void * arg, thread_type & t, state<thread_state> & s)
{
    assert(s() == NOT_RUNNING);
#if STXXL_STD_THREADS
    t = new std::thread(worker, arg);
#elif STXXL_BOOST_THREADS
    t = new boost::thread(boost::bind(worker, arg));
#else
    check_pthread_call(pthread_create(&t, NULL, worker, arg));
#endif
    s.set_to(RUNNING);
}

void request_queue_impl_worker::stop_thread(thread_type & t, state<thread_state> & s, semaphore & sem)
{
    assert(s() == RUNNING);
    s.set_to(TERMINATING);
    sem++;
#if STXXL_STD_THREADS
#if STXXL_MSVC >= 1700
    // skip join and delete of threads due to deadlock bug in CRT library,
    // which occurs due to main() exiting before the threads do.
#else
    t->join();
    delete t;
#endif
    t = NULL;
#elif STXXL_BOOST_THREADS
    t->join();
    delete t;
    t = NULL;
#else
    check_pthread_call(pthread_join(t, NULL));
#endif
    s.set_to(NOT_RUNNING);
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
