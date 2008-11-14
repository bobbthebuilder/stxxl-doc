/***************************************************************************
 *  include/stxxl/bits/io/mem_file.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MEM_FILE_HEADER
#define STXXL_MEM_FILE_HEADER

#include <stxxl/bits/io/iobase.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implementation of file based on new[] and memcpy
class mem_file : public file
{
    char * ptr;
    unsigned_type sz;

public:
    //! \brief constructs file object
    //! \param disk disk(file) identifier
    mem_file(
        int disk = -1);
    request_ptr aread(
        void * buffer,
        stxxl::int64 pos,
        size_t bytes,
        completion_handler on_cmpl);
    request_ptr awrite(
        void * buffer,
        stxxl::int64 pos,
        size_t bytes,
        completion_handler on_cmpl);
    char * get_ptr() const;
    ~mem_file();
    stxxl::int64 size();
    void set_size(stxxl::int64 newsize);
    void lock();
    void delete_region(int64 offset, unsigned_type size);
};

//! \brief Implementation of request based on memcpy()
class mem_request : public request
{
    friend class mem_file;

protected:
    state<request_status> _state;
    mutex waiters_mutex;
    std::set<onoff_switch *> waiters;

    mem_request(
        mem_file * f,
        void * buf,
        stxxl::int64 off,
        size_t b,
        request_type t,
        completion_handler on_cmpl);
    bool add_waiter(onoff_switch * sw);
    void delete_waiter(onoff_switch * sw);
    int nwaiters();             // returns number of waiters
    void serve();

public:
    virtual ~mem_request();
    void wait();
    bool poll();
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_MEM_FILE_HEADER
