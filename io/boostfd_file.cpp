/***************************************************************************
 *  io/boostfd_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifdef STXXL_BOOST_CONFIG

#include <stxxl/bits/io/boostfd_file.h>
#include <stxxl/bits/io/request_impl_basic.h>
#include <stxxl/bits/common/debug.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>


__STXXL_BEGIN_NAMESPACE


void boostfd_file::serve(const request * req) throw(io_error)
{
    assert(req->get_file() == this);
    stxxl::int64 offset = req->get_offset();
    void * buffer = req->get_buffer();
    size_t bytes = req->get_size();
    request::request_type type = req->get_type();

    try
    {
        file_des.seek(offset, BOOST_IOS::beg);
    }
    catch (const std::exception & ex)
    {
            STXXL_THROW2(io_error,
                         "Error doing seek() in boostfd_request::serve()" <<
                         " offset=" << offset <<
                         " this=" << this <<
                         " buffer=" << buffer <<
                         " bytes=" << bytes <<
                         " type=" << ((type == request::READ) ? "READ" : "WRITE") <<
                         " : " << ex.what());
    }

        stats::scoped_read_write_timer read_write_timer(bytes, type == request::WRITE);

        if (type == request::READ)
        {
            STXXL_DEBUGMON_DO(io_started(buffer));

            try
            {
                file_des.read((char *)buffer, bytes);
            }
            catch (const std::exception & ex)
            {
                STXXL_THROW2(io_error,
                             "Error doing read() in boostfd_request::serve()" <<
                             " offset=" << offset <<
                             " this=" << this <<
                             " buffer=" << buffer <<
                             " bytes=" << bytes <<
                             " type=" << ((type == request::READ) ? "READ" : "WRITE") <<
                             " : " << ex.what());
            }

            STXXL_DEBUGMON_DO(io_finished(buffer));
        }
        else
        {
            STXXL_DEBUGMON_DO(io_started(buffer));

            try
            {
                file_des.write((char *)buffer, bytes);
            }
            catch (const std::exception & ex)
            {
                STXXL_THROW2(io_error,
                             "Error doing write() in boostfd_request::serve()" <<
                             " offset=" << offset <<
                             " this=" << this <<
                             " buffer=" << buffer <<
                             " bytes=" << bytes <<
                             " type=" << ((type == request::READ) ? "READ" : "WRITE") <<
                             " : " << ex.what());
            }

            STXXL_DEBUGMON_DO(io_finished(buffer));
        }
}

const char * boostfd_file::io_type() const
{
    return "boostfd";
}

boostfd_file::boostfd_file(
    const std::string & filename,
    int mode,
    int disk) : file(disk), mode_(mode)
{
    BOOST_IOS::openmode boostfd_mode;

 #ifndef STXXL_DIRECT_IO_OFF
    if (mode & DIRECT)
    {
        // direct mode not supported in Boost
    }
 #endif

    if (mode & RDONLY)
    {
        boostfd_mode = BOOST_IOS::in;
    }

    if (mode & WRONLY)
    {
        boostfd_mode = BOOST_IOS::out;
    }

    if (mode & RDWR)
    {
        boostfd_mode = BOOST_IOS::out | BOOST_IOS::in;
    }

    const boost::filesystem::path fspath(filename,
                                         boost::filesystem::native);

    if (mode & TRUNC)
    {
        if (boost::filesystem::exists(fspath))
        {
            boost::filesystem::remove(fspath);
            boost::filesystem::ofstream f(fspath);
            f.close();
            assert(boost::filesystem::exists(fspath));
        }
    }

    if (mode & CREAT)
    {
        // need to be emulated:
        if (!boost::filesystem::exists(fspath))
        {
            boost::filesystem::ofstream f(fspath);
            f.close();
            assert(boost::filesystem::exists(fspath));
        }
    }

    file_des.open(filename, boostfd_mode, boostfd_mode);
}

boostfd_file::~boostfd_file()
{
    file_des.close();
}

stxxl::int64 boostfd_file::size()
{
    stxxl::int64 size_ = file_des.seek(0, BOOST_IOS::end);
    return size_;
}

void boostfd_file::set_size(stxxl::int64 newsize)
{
    stxxl::int64 oldsize = size();
    file_des.seek(newsize, BOOST_IOS::beg);
    file_des.seek(0, BOOST_IOS::beg); // not important ?
    assert(size() >= oldsize);
}

request_ptr boostfd_file::aread(
    void * buffer,
    stxxl::int64 pos,
    size_t bytes,
    completion_handler on_cmpl)
{
    request_ptr req = new request_impl_basic(on_cmpl, this,
                                          buffer, pos, bytes,
                                          request::READ);

    if (!req.get())
        stxxl_function_error(io_error);

    disk_queues::get_instance()->add_readreq(req, get_id());

    return req;
}

request_ptr boostfd_file::awrite(
    void * buffer,
    stxxl::int64 pos,
    size_t bytes,
    completion_handler on_cmpl)
{
    request_ptr req = new request_impl_basic(on_cmpl, this, buffer, pos, bytes,
                                          request::WRITE);

    if (!req.get())
        stxxl_function_error(io_error);

    disk_queues::get_instance()->add_writereq(req, get_id());

    return req;
}

void boostfd_file::lock()
{
    // FIXME: is there no locking possible/needed/... for boostfd?
}

__STXXL_END_NAMESPACE

#endif // #ifdef STXXL_BOOST_CONFIG
// vim: et:ts=4:sw=4
