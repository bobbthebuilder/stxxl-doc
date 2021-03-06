/***************************************************************************
 *  include/stxxl/bits/libstxxl.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_LIBSTXXL_H
#define STXXL_LIBSTXXL_H

#include <stxxl/bits/config.h>

#ifdef STXXL_MSVC
 #ifndef STXXL_LIBNAME
  #define STXXL_LIBNAME "stxxl"
 #endif
//-tb #pragma comment (lib, "lib" STXXL_LIBNAME ".lib")
#endif

#endif // !STXXL_IO_HEADER
