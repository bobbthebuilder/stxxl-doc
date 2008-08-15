/***************************************************************************
 *  include/stxxl/bits/io/boostfd_file.h
 *
 *  File implementation based on boost::iostreams::file_decriptor
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_BOOSTFD_FILE_H_
#define STXXL_BOOSTFD_FILE_H_

#ifdef STXXL_BOOST_CONFIG // if boost is available

 #include <stxxl/bits/io/iobase.h>

 #include <boost/iostreams/device/file_descriptor.hpp>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

class boostfd_request;

//! \brief Implementation based on boost::iostreams::file_decriptor
class boostfd_file : public file
{
public:
    typedef boost::iostreams::file_descriptor fd_type;

protected:
    fd_type file_des;
    int mode_;

public:
    boostfd_file(const std::string & filename, int mode, int disk = -1);
    fd_type get_file_des() const;
    ~boostfd_file();
    stxxl::int64 size();
    void set_size(stxxl::int64 newsize);
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
};

//! \brief Implementation based on boost::iostreams::file_decriptor
class boostfd_request : public request
{
    friend class boostfd_file;

protected:
    // states of request
    enum { OP = 0, DONE = 1, READY2DIE = 2 }; // OP - operating, DONE - request served,
                                              // READY2DIE - can be destroyed

    state _state;
    mutex waiters_mutex;
    std::set<onoff_switch *> waiters;

    boostfd_request(
        boostfd_file * f,
        void * buf,
        stxxl::int64 off,
        size_t b,
        request_type t,
        completion_handler on_cmpl);

    bool add_waiter(onoff_switch * sw);
    void delete_waiter(onoff_switch * sw);
    int nwaiters(); // returns the number of waiters
    void check_aligning();
    void serve();

public:
    virtual ~boostfd_request();
    void wait();
    bool poll();
    const char * io_type();
};

//! \}

__STXXL_END_NAMESPACE

#endif // #ifdef STXXL_BOOST_CONFIG

#endif // !STXXL_BOOSTFD_FILE_H_
