/***************************************************************************
 *  io/ufs_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002, 2005, 2008 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/basic_request_state.h>


__STXXL_BEGIN_NAMESPACE


basic_request_state::basic_request_state(
    completion_handler on_cmpl,
    file * f,
    void * buf,
    stxxl::int64 off,
    size_t b,
    request_type t) :
    request(on_cmpl, f, buf, off, b, t),
    _state(OP)
{ }

basic_request_state::~basic_request_state()
{
    STXXL_VERBOSE3("basic_request_state " << static_cast<void *>(this) << ": deletion, cnt: " << ref_cnt);

    assert(_state() == DONE || _state() == READY2DIE);

    // if(_state() != DONE && _state()!= READY2DIE )
    //	STXXL_ERRMSG("WARNING: serious stxxl error request being deleted while I/O did not finish "<<
    //		"! Please report it to the stxxl author(s) <dementiev@mpi-sb.mpg.de>");

    // _state.wait_for (READY2DIE); // does not make sense ?
}

void basic_request_state::wait()
{
    STXXL_VERBOSE3("ufs_request_base : " << static_cast<void *>(this) << " wait ");

    stats::scoped_wait_timer wait_timer;

    _state.wait_for(READY2DIE);

    check_errors();
}

bool basic_request_state::poll()
{
    const request_state s = _state();

    check_errors();

    return s == DONE || s == READY2DIE;
}

const char * basic_request_state::io_type() const
{
    return "basic_request_state";
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
