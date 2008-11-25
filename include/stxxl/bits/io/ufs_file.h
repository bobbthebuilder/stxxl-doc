/***************************************************************************
 *  include/stxxl/bits/io/ufs_file.h
 *
 *  UNIX file system file base
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_UFSFILEBASE_HEADER
#define STXXL_UFSFILEBASE_HEADER

#include <stxxl/bits/io/file_request_basic.h>
#include <stxxl/bits/io/request.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Base for UNIX file system implementations
class ufs_file_base : public file_request_basic
{
protected:
    mutex fd_mutex;        // sequentialize function calls involving file_des
    int file_des;          // file descriptor
    int mode_;             // open mode
    ufs_file_base(const std::string & filename, int mode, int disk);

public:
    ~ufs_file_base();
    stxxl::int64 size();
    void set_size(stxxl::int64 newsize);
    void lock();
    const char * io_type() const;
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_UFSFILEBASE_HEADER
