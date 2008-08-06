/***************************************************************************
 *  io/ufs_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002, 2005, 2008 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Ilja Andronov <sni4ok@yandex.ru>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/ufs_file.h>

#ifndef BOOST_MSVC
 #include <unistd.h>
 #include <fcntl.h>
#endif


__STXXL_BEGIN_NAMESPACE


ufs_request_base::ufs_request_base(
    ufs_file_base * f,
    void * buf,
    stxxl::int64 off,
    size_t b,
    request_type t,
    completion_handler on_cmpl) :
    request(on_cmpl, f, buf, off, b, t),
    _state(OP)
{
#ifdef STXXL_CHECK_BLOCK_ALIGNING
    // Direct I/O requires file system block size alignment for file offsets,
    // memory buffer addresses, and transfer(buffer) size must be multiple
    // of the file system block size
    check_aligning();
#endif
}

ufs_request_base::~ufs_request_base()
{
    STXXL_VERBOSE3("ufs_request_base " << static_cast<void *>(this) << ": deletion, cnt: " << ref_cnt);

    assert(_state() == DONE || _state() == READY2DIE);

    // if(_state() != DONE && _state()!= READY2DIE )
    //	STXXL_ERRMSG("WARNING: serious stxxl error request being deleted while I/O did not finish "<<
    //		"! Please report it to the stxxl author(s) <dementiev@mpi-sb.mpg.de>");

    // _state.wait_for (READY2DIE); // does not make sense ?
}

bool ufs_request_base::add_waiter(onoff_switch * sw)
{
    scoped_mutex_lock Lock(waiters_mutex);

    if (poll())                     // request already finished
    {
        return true;
    }

    waiters.insert(sw);

    return false;
}

void ufs_request_base::delete_waiter(onoff_switch * sw)
{
    scoped_mutex_lock Lock(waiters_mutex);
    waiters.erase(sw);
}

int ufs_request_base::nwaiters()                 // returns number of waiters
{
    scoped_mutex_lock Lock(waiters_mutex);
    return waiters.size();
}

void ufs_request_base::check_aligning()
{
    if (offset % BLOCK_ALIGN != 0)
        STXXL_ERRMSG("Offset is not aligned: modulo "
                                              << BLOCK_ALIGN << " = " <<
                     offset % BLOCK_ALIGN);

    if (bytes % BLOCK_ALIGN != 0)
        STXXL_ERRMSG("Size is not a multiple of " <<
                     BLOCK_ALIGN << ", = " << bytes % BLOCK_ALIGN);

    if (long(buffer) % BLOCK_ALIGN != 0)
        STXXL_ERRMSG("Buffer is not aligned: modulo "
                                              << BLOCK_ALIGN << " = " <<
                     long(buffer) % BLOCK_ALIGN << " (" <<
                     std::hex << buffer << std::dec << ")");
}

void ufs_request_base::wait()
{
    STXXL_VERBOSE3("ufs_request_base : " << static_cast<void *>(this) << " wait ");

    START_COUNT_WAIT_TIME

#ifdef NO_OVERLAPPING
    enqueue();
#endif

    _state.wait_for(READY2DIE);

    END_COUNT_WAIT_TIME

    check_errors();
}

bool ufs_request_base::poll()
{
#ifdef NO_OVERLAPPING
    /*if(_state () < DONE)*/ wait();
#endif

    bool s = _state() >= DONE;

    check_errors();

    return s;
}

const char * ufs_request_base::io_type()
{
    return "ufs_base";
}

////////////////////////////////////////////////////////////////////////////

ufs_file_base::ufs_file_base(
    const std::string & filename,
    int mode,
    int disk) : file(disk), file_des(-1), mode_(mode)
{
    int fmode = 0;
#ifndef STXXL_DIRECT_IO_OFF
 #ifndef BOOST_MSVC
    if (mode & DIRECT)
        fmode |= O_SYNC | O_RSYNC | O_DSYNC | O_DIRECT;

 #endif
#endif
    if (mode & RDONLY)
        fmode |= O_RDONLY;

    if (mode & WRONLY)
        fmode |= O_WRONLY;

    if (mode & RDWR)
        fmode |= O_RDWR;

    if (mode & CREAT)
        fmode |= O_CREAT;

    if (mode & TRUNC)
        fmode |= O_TRUNC;


#ifdef BOOST_MSVC
    fmode |= O_BINARY;                     // the default in MS is TEXT mode
#endif

#ifdef BOOST_MSVC
    const int flags = S_IREAD | S_IWRITE;
#else
    const int flags = S_IREAD | S_IWRITE | S_IRGRP | S_IWGRP;
#endif

    if ((file_des = ::open(filename.c_str(), fmode, flags)) < 0)
        STXXL_THROW2(io_error, "Filedescriptor=" << file_des << " filename=" << filename << " fmode=" << fmode);
}

ufs_file_base::~ufs_file_base()
{
    int res = ::close(file_des);

    // if successful, reset file descriptor
    if (res >= 0)
        file_des = -1;

    else
        stxxl_function_error(io_error);
}

int ufs_file_base::get_file_des() const
{
    return file_des;
}

void ufs_file_base::lock()
{
#ifdef BOOST_MSVC
    // not yet implemented
#else
    struct flock lock_struct;
    lock_struct.l_type = F_RDLCK | F_WRLCK;
    lock_struct.l_whence = SEEK_SET;
    lock_struct.l_start = 0;
    lock_struct.l_len = 0; // lock all bytes
    if ((::fcntl(file_des, F_SETLK, &lock_struct)) < 0)
        STXXL_THROW2(io_error, "Filedescriptor=" << file_des);
#endif
}

stxxl::int64 ufs_file_base::size()
{
    struct stat st;
    stxxl_check_ge_0(fstat(file_des, &st), io_error);
    return st.st_size;
}

void ufs_file_base::set_size(stxxl::int64 newsize)
{
    stxxl::int64 cur_size = size();

#ifdef BOOST_MSVC
    if (!(mode_ & RDONLY))
    {
        HANDLE hfile;
        stxxl_check_ge_0(hfile = (HANDLE) ::_get_osfhandle(file_des), io_error);

        LARGE_INTEGER desired_pos;
        desired_pos.QuadPart = newsize;

        if (!SetFilePointerEx(hfile, desired_pos, NULL, FILE_BEGIN))
            stxxl_win_lasterror_exit("SetFilePointerEx in ufs_file_base::set_size(..) oldsize=" << cur_size <<
                                     " newsize=" << newsize << " ", io_error)

            if (!SetEndOfFile(hfile))
                stxxl_win_lasterror_exit("SetEndOfFile oldsize=" << cur_size <<
                                         " newsize=" << newsize << " ", io_error);
    }
#else
    if (!(mode_ & RDONLY))
        stxxl_check_ge_0(::ftruncate(file_des, newsize), io_error);

    if (newsize > cur_size)
        stxxl_check_ge_0(::lseek(file_des, newsize - 1, SEEK_SET), io_error);
#endif
}

__STXXL_END_NAMESPACE
