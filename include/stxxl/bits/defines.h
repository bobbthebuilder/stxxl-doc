/***************************************************************************
 *  include/stxxl/bits/defines.h
 *
 *  Document all defines that may change the behavior of stxxl.
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_DEFINES_HEADER
#define STXXL_DEFINES_HEADER

//#define STXXL_CHECK_BLOCK_ALIGNING
// default: not defined
// used in: io/*_file.cpp
// effect:  call request::check_alignment() from request::request(...)

//#define STXXL_DO_NOT_COUNT_WAIT_TIME
// default: not defined
// used in: io/iostats.{h,cpp}
// effect:  makes calls to wait time counting functions no-ops

//#define STXXL_WAIT_LOG_ENABLED
// default: not defined
// used in: common/log.cpp, io/iostats.cpp
// effect:  writes wait timing information to the file given via environment
//          variable STXXLWAITLOGFILE, does nothing if this is not defined

//#define STXXL_SORT_OPTIMAL_PREFETCHING 0/1
// default: 1
// used in: algo/*sort.h, stream/sort_stream.h
// effect if defined to 0: unknown

//#define STXXL_CHECK_ORDER_IN_SORTS 0/1
// default: 0
// used in: algo/*sort.h, stream/sort_stream.h, containers/priority_queue.h
// effect if set to 1: perform additional checking of sorted results

#endif // !STXXL_DEFINES_HEADER
