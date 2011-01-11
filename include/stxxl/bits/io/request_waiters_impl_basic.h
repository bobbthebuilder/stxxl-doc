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

#ifndef STXXL_REQUEST_WAITERS_IMPL_BASIC_HEADER
#define STXXL_REQUEST_WAITERS_IMPL_BASIC_HEADER

#include <set>

#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/common/switch.h>
#include <stxxl/bits/io/request_state_impl_basic.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implements basic waiters.
class request_waiters_impl_basic : public request_state_impl_basic
{
    mutex waiters_mutex;
    std::set<onoff_switch *> waiters;

protected:
    bool add_waiter(onoff_switch * sw);
    void delete_waiter(onoff_switch * sw);
    void notify_waiters();
    /*
    int nwaiters();             // returns number of waiters
    */

public:
    request_waiters_impl_basic(
        const completion_handler & on_cmpl,
        file * f,
        void * buf,
        offset_type off,
        size_type b,
        request_type t) :
        request_state_impl_basic(on_cmpl, f, buf, off, b, t)
    { }

};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_REQUEST_WAITERS_IMPL_BASIC_HEADER
