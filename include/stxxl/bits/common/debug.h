/***************************************************************************
 *  include/stxxl/bits/common/debug.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2004 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_DEBUG_HEADER
#define STXXL_DEBUG_HEADER

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/singleton.h>
#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/compat_hash_map.h>


__STXXL_BEGIN_NAMESPACE

class debugmon : public singleton<debugmon>
{
    friend class singleton<debugmon>;

#ifdef STXXL_DEBUGMON

    struct tag
    {
        bool ongoing;
        char * end;
        size_t size;
    };
    struct hash_fct
    {
        inline size_t operator () (char * arg) const
        {
            return long(arg);
        }
#ifdef BOOST_MSVC
        bool operator () (char * a, char * b) const
        {
            return (long(a) < long(b));
        }
        enum
        {                       // parameters for hash table
            bucket_size = 4,    // 0 < bucket_size
            min_buckets = 8     // min_buckets = 2 ^^ N, 0 < N
        };
#endif
    };

    compat_hash_map<char *, tag, hash_fct>::result tags;

#ifdef STXXL_BOOST_THREADS
    boost::mutex mutex1;
#else
    mutex mutex1;
#endif

#endif // #ifdef STXXL_DEBUGMON

public:
    void block_allocated(char * ptr, char * end, size_t size);
    void block_deallocated(char * ptr);
    void io_started(char * ptr);
    void io_finished(char * ptr);
};

#ifndef STXXL_DEBUGMON
inline void debugmon::block_allocated(char * /*ptr*/, char * /*end*/, size_t /*size*/)
{ }
inline void debugmon::block_deallocated(char * /*ptr*/)
{ }
inline void debugmon::io_started(char * /*ptr*/)
{ }
inline void debugmon::io_finished(char * /*ptr*/)
{ }
#endif

__STXXL_END_NAMESPACE

#endif // !STXXL_DEBUG_HEADER
