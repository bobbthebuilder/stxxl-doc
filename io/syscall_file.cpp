/***************************************************************************
 *  io/syscall_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/syscall_file.h>
#include <stxxl/bits/io/request_impl_basic.h>


__STXXL_BEGIN_NAMESPACE

#ifdef BOOST_MSVC
#define lseek _lseeki64
#endif

void syscall_file::serve(const request * req) throw (io_error)
{
    scoped_mutex_lock fd_lock(fd_mutex);
    assert(req->get_file() == this);
    offset_type offset = req->get_offset();
    char * buffer = static_cast<char *>(req->get_buffer());
    size_type bytes = req->get_size();
    request::request_type type = req->get_type();

    stats::scoped_read_write_timer read_write_timer(bytes, type == request::WRITE);

    while (bytes > 0)
    {
        int_type rc;
        if ((rc = ::lseek(file_des, offset, SEEK_SET) < 0))
        {
            STXXL_THROW2(io_error,
                         " this=" << this <<
                         " call=::lseek(fd,offset,SEEK_SET)" <<
                         " fd=" << file_des <<
                         " offset=" << offset <<
                         " buffer=" << (void*)buffer <<
                         " bytes=" << bytes <<
                         " type=" << ((type == request::READ) ? "READ" : "WRITE") <<
                         " rc= " << rc);
        }

        if (type == request::READ)
        {
            if ((rc = ::read(file_des, buffer, bytes)) <= 0)
            {
                STXXL_THROW2(io_error,
                             " this=" << this <<
                             " call=::read(fd,buffer,bytes)" <<
                             " fd=" << file_des <<
                             " offset=" << offset <<
                             " buffer=" << (void*)buffer <<
                             " bytes=" << bytes <<
                             " type=" << "READ" <<
                             " rc= " << rc);
            }
            bytes -= rc;
            offset += rc;
            buffer += rc;
        }
        else
        {
            if ((rc = ::write(file_des, buffer, bytes)) <= 0)
            {
                STXXL_THROW2(io_error,
                             " this=" << this <<
                             " call=::write(fd,buffer,bytes)" <<
                             " fd=" << file_des <<
                             " offset=" << offset <<
                             " buffer=" << (void*)buffer <<
                             " bytes=" << bytes <<
                             " type=" << "WRITE" <<
                             " rc= " << rc);
            }
            bytes -= rc;
            offset += rc;
            buffer += rc;
        }
    }
}

const char * syscall_file::io_type() const
{
    return "syscall";
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
