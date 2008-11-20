/***************************************************************************
 *  include/stxxl/bits/io/basic_waiters_request.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_BASIC_WAITERS_REQUEST_HEADER
#define STXXL_BASIC_WAITERS_REQUEST_HEADER

#include <set>

#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/common/switch.h>
#include <stxxl/bits/io/request.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implements basic waiters.
class basic_waiters_request : public request
{
    mutex waiters_mutex;
    std::set<onoff_switch *> waiters;

protected:
    basic_waiters_request(
        completion_handler on_cmpl,
        file * f,
        void * buf,
        stxxl::int64 off,
        size_t b,
        request_type t);
    virtual ~basic_waiters_request();

    bool add_waiter(onoff_switch * sw);
    void delete_waiter(onoff_switch * sw);
    void notify_waiters();
    /*
    int nwaiters();             // returns number of waiters
    */

public:
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_BASIC_WAITERS_REQUEST_HEADER
