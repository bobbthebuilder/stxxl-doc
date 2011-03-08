/***************************************************************************
 *  include/stxxl/bits/io/request.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_REQUEST_HEADER
#define STXXL_REQUEST_HEADER

#include <iostream>
#include <memory>
#include <cassert>

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/common/types.h>
#include <stxxl/bits/io/request_interface.h>
#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/common/switch.h>
#include <stxxl/bits/common/exceptions.h>
#include <stxxl/bits/io/completion_handler.h>
#include <stxxl/bits/compat_unique_ptr.h>
#include <stxxl/bits/common/error_handling.h>
#include <stxxl/bits/verbose.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

#define BLOCK_ALIGN 4096

class file;
class request_ptr;

//! \brief Basic properties of a request.
class request : virtual public request_base
{
    friend int wait_any(request_ptr req_array[], int count);
    template <class request_iterator_>
    friend
    request_iterator_ wait_any(request_iterator_ reqs_begin, request_iterator_ reqs_end);
    friend class request_queue_impl_qwqr;
    friend class request_ptr;

protected:
    completion_handler on_complete;
    int ref_cnt;
    compat_unique_ptr<stxxl::io_error>::result error;

    mutex ref_cnt_mutex;

protected:
    file * file_;
    void * buffer;
    offset_type offset;
    size_type bytes;
    request_type type;

    void completed();

    // returns number of references
    int nref()
    {
        scoped_mutex_lock Lock(ref_cnt_mutex);
        return ref_cnt;
    }

public:
    request(const completion_handler & on_compl,
            file * file__,
            void * buffer_,
            offset_type offset_,
            size_type bytes_,
            request_type type_);

    virtual ~request();

    file * get_file() const { return file_; }
    void * get_buffer() const { return buffer; }
    offset_type get_offset() const { return offset; }
    size_type get_size() const { return bytes; }
    request_type get_type() const { return type; }

    void check_alignment() const;

    virtual std::ostream & print(std::ostream & out) const
    {
        out << "File object address: " << (void *)get_file();
        out << " Buffer address: " << (void *)get_buffer();
        out << " File offset: " << get_offset();
        out << " Transfer size: " << get_size() << " bytes";
        out << " Type of transfer: " << ((get_type() == READ) ? "READ" : "WRITE");
        return out;
    }

    //! \brief Inform the request object that an error occurred
    //! during the I/O execution
    void error_occured(const char * msg)
    {
        error.reset(new stxxl::io_error(msg));
    }

    //! \brief Inform the request object that an error occurred
    //! during the I/O execution
    void error_occured(const std::string & msg)
    {
        error.reset(new stxxl::io_error(msg));
    }

    //! \brief Rises an exception if there were error with the I/O
    void check_errors() throw (stxxl::io_error)
    {
        if (error.get())
            throw * (error.get());
    }

private:
    void add_ref()
    {
        scoped_mutex_lock Lock(ref_cnt_mutex);
        ref_cnt++;
        STXXL_VERBOSE3("[" << static_cast<void *>(this) << "] request::add_ref(): added reference, ref_cnt=" << ref_cnt);
    }

    bool sub_ref()
    {
        int val;
        {
            scoped_mutex_lock Lock(ref_cnt_mutex);
            val = --ref_cnt;
            STXXL_VERBOSE3("[" << static_cast<void *>(this) << "] request::sub_ref(): subtracted reference, ref_cnt=" << ref_cnt);
        }
        assert(val >= 0);
        return (val == 0);
    }

protected:
    void check_nref(bool after = false)
    {
        if (nref() < 2)
            check_nref_failed(after);
    }

private:
    void check_nref_failed(bool after);
};

inline std::ostream & operator << (std::ostream & out, const request & req)
{
    return req.print(out);
}

//! \}

__STXXL_END_NAMESPACE
#include <stxxl/bits/io/request_ptr.h>
#include <stxxl/bits/io/request_operations.h>

#endif // !STXXL_REQUEST_HEADER
// vim: et:ts=4:sw=4
