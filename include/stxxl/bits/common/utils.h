/***************************************************************************
 *  include/stxxl/bits/common/utils.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2006 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007, 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_UTILS_HEADER
#define STXXL_UTILS_HEADER

#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <string>
#include <limits>

#include <cassert>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#ifdef STXXL_BOOST_FILESYSTEM
 #include <boost/filesystem/operations.hpp>
#endif

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/common/log.h>
#include <stxxl/bits/common/types.h>
#include <stxxl/bits/common/timer.h>
#include <stxxl/bits/common/is_sorted.h>


__STXXL_BEGIN_NAMESPACE

#ifdef UNUSED
#error Somebody has defined a macro UNUSED which clashes with a helper function of STXXL
// possible solutions:
//
// 1) #undef UNUSED
//
// 2) #undef UNUSED
//    #define UNUSED(unused) (stxxl::UNUSED)(unused)
//
// 3) don't declare stxxl::UNUSED
#endif
template <typename U>
inline void UNUSED(const U &)
{ }

#ifdef BOOST_MSVC
  #define __STXXL_DEPRECATED(x) __declspec(deprecated) x
#elif defined(__GNUG__) && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) < 30400)
  // no __attribute__ ((__deprecated__)) in GCC 3.3
  #define __STXXL_DEPRECATED(x) x
#else
  #define __STXXL_DEPRECATED(x) x __attribute__ ((__deprecated__))
#endif

////////////////////////////////////////////////////////////////////////////

#define _STXXL_PRINT(label, outstream, log_stream, message) \
    { std::ostringstream str_; \
      str_ << "[" label "] " << message << std::endl; \
      outstream << str_.str() << std::flush; \
      stxxl::logger::get_instance()->log_stream() << str_.str() << std::flush; \
    }

#define STXXL_MSG(x) _STXXL_PRINT("STXXL-MSG", std::cout, log_stream, x)

#define STXXL_ERRMSG(x) _STXXL_PRINT("STXXL-ERRMSG", std::cerr, errlog_stream, x)


#ifdef STXXL_FORCE_VERBOSE_LEVEL
#undef STXXL_VERBOSE_LEVEL
#define STXXL_VERBOSE_LEVEL STXXL_FORCE_VERBOSE_LEVEL
#endif

#ifdef STXXL_DEFAULT_VERBOSE_LEVEL
#ifndef STXXL_VERBOSE_LEVEL
#define STXXL_VERBOSE_LEVEL STXXL_DEFAULT_VERBOSE_LEVEL
#endif
#endif

#ifndef STXXL_VERBOSE_LEVEL
#define STXXL_VERBOSE_LEVEL -1
#endif

// STXXL_VERBOSE0 should be used for current debugging activity only,
// and afterwards be replaced by STXXL_VERBOSE1 or higher.
// Code that actively uses STXXL_VERBOSE0 should never get into a release.

#if STXXL_VERBOSE_LEVEL > -1
 #define STXXL_VERBOSE0(x) _STXXL_PRINT("STXXL-VERBOSE0", std::cout, log_stream, x)
#else
 #define STXXL_VERBOSE0(x)
#endif

#if STXXL_VERBOSE_LEVEL > 0
 #define STXXL_VERBOSE1(x) _STXXL_PRINT("STXXL-VERBOSE1", std::cout, log_stream, x)
#else
 #define STXXL_VERBOSE1(x)
#endif

#define STXXL_VERBOSE(x) STXXL_VERBOSE1(x)

#if STXXL_VERBOSE_LEVEL > 1
 #define STXXL_VERBOSE2(x) _STXXL_PRINT("STXXL-VERBOSE2", std::cout, log_stream, x)
#else
 #define STXXL_VERBOSE2(x)
#endif

#if STXXL_VERBOSE_LEVEL > 2
 #define STXXL_VERBOSE3(x) _STXXL_PRINT("STXXL-VERBOSE3", std::cout, log_stream, x)
#else
 #define STXXL_VERBOSE3(x)
#endif

////////////////////////////////////////////////////////////////////////////

#ifdef BOOST_MSVC

#define stxxl_win_lasterror_exit(errmsg, exception_type) \
    { \
        TCHAR szBuf[80]; \
        LPVOID lpMsgBuf; \
        DWORD dw = GetLastError(); \
        FormatMessage( \
            FORMAT_MESSAGE_ALLOCATE_BUFFER | \
            FORMAT_MESSAGE_FROM_SYSTEM, \
            NULL, \
            dw, \
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
            (LPTSTR)&lpMsgBuf, \
            0, NULL); \
        std::ostringstream str_; \
        str_ << "Error in " << errmsg << ", error code " << dw << ": " << ((char *)lpMsgBuf); \
        LocalFree(lpMsgBuf); \
        throw exception_type(str_.str()); \
    }

#endif

////////////////////////////////////////////////////////////////////////////

inline std::string
stxxl_tmpfilename(std::string dir, std::string prefix)
{
    //STXXL_VERBOSE0(" TMP:"<< dir.c_str() <<":"<< prefix.c_str());
    int rnd;
    char buffer[1024];
    std::string result;

#ifndef STXXL_BOOST_FILESYSTEM
    struct stat st;
#endif

    do
    {
        rnd = rand();
        sprintf(buffer, "%d", rnd);
        result = dir + prefix + buffer;
    }
#ifdef STXXL_BOOST_FILESYSTEM
    while (boost::filesystem::exists(result));

    return result;
#else
    while (!lstat(result.c_str(), &st));

    if (errno != ENOENT)
        stxxl_function_error(io_error);

    return result;
#endif
}

////////////////////////////////////////////////////////////////////////////

inline std::vector<std::string>
split(const std::string & str, const std::string & sep)
{
    std::vector<std::string> result;
    if (str.empty())
        return result;

    std::string::size_type CurPos(0), LastPos(0);
    while (1)
    {
        CurPos = str.find(sep, LastPos);
        if (CurPos == std::string::npos)
            break;

        std::string sub =
            str.substr(LastPos,
                       std::string::size_type(CurPos -
                                              LastPos));
        if (sub.size())
            result.push_back(sub);

        LastPos = CurPos + sep.size();
    }

    std::string sub = str.substr(LastPos);
    if (sub.size())
        result.push_back(sub);

    return result;
}

////////////////////////////////////////////////////////////////////////////

#define str2int(str) atoi(str.c_str())

inline std::string int2str(int i)
{
    char buf[32];
    sprintf(buf, "%d", i);
    return std::string(buf);
}

inline stxxl::int64 atoint64(const char * s)
{
#ifdef BOOST_MSVC
    return _atoi64(s);
#else
    return atoll(s);
#endif
}

////////////////////////////////////////////////////////////////////////////

#define STXXL_MIN(a, b) ((std::min)(a, b))
#define STXXL_MAX(a, b) ((std::max)(a, b))

#define STXXL_L2_SIZE  (512 * 1024)

#define STXXL_DIVRU(a, b) ((a) / (b) + !(!((a) % (b))))

inline double log2(double x)
{
    return (log(x) / log(2.));
}

////////////////////////////////////////////////////////////////////////////

//#define HAVE_BUILTIN_EXPECT

#ifdef HAVE_BUILTIN_EXPECT
 #define LIKELY(c)   __builtin_expect((c), 1)
#else
 #define LIKELY(c)   c
#endif

#ifdef HAVE_BUILTIN_EXPECT
 #define UNLIKELY(c)   __builtin_expect((c), 0)
#else
 #define UNLIKELY(c)   c
#endif

////////////////////////////////////////////////////////////////////////////

inline uint64 longhash1(uint64 key_)
{
    key_ += ~(key_ << 32);
    key_ ^= (key_ >> 22);
    key_ += ~(key_ << 13);
    key_ ^= (key_ >> 8);
    key_ += (key_ << 3);
    key_ ^= (key_ >> 15);
    key_ += ~(key_ << 27);
    key_ ^= (key_ >> 31);
    return key_;
}

////////////////////////////////////////////////////////////////////////////

template <class T>
inline void swap_1D_arrays(T * a, T * b, unsigned_type size)
{
    for (unsigned_type i = 0; i < size; ++i)
        std::swap(a[i], b[i]);
}

////////////////////////////////////////////////////////////////////////////

template <class T>
class new_alloc;

template <typename T, typename U>
struct new_alloc_rebind;

template <typename T>
struct new_alloc_rebind<T, T>{
    typedef new_alloc<T> other;
};

template <typename T, typename U>
struct new_alloc_rebind {
    typedef std::allocator<U> other;
};


// designed for typed_block (to use with std::vector )
template <class T>
class new_alloc {
public:
    // type definitions
    typedef T value_type;
    typedef T * pointer;
    typedef const T * const_pointer;
    typedef T & reference;
    typedef const T & const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    // rebind allocator to type U, use new_alloc only if U == T
    template <class U>
    struct rebind {
        typedef typename new_alloc_rebind<T, U>::other other;
    };

    // return address of values
    pointer address(reference value) const
    {
        return &value;
    }
    const_pointer address(const_reference value) const
    {
        return &value;
    }

    new_alloc() throw () { }
    new_alloc(const new_alloc &) throw () { }
    template <class U>
    new_alloc(const new_alloc<U> &) throw () { }
    ~new_alloc() throw () { }

    template <class U>
    operator std::allocator<U>()
    {
        static std::allocator<U> helper_allocator;
        return helper_allocator;
    }

    // return maximum number of elements that can be allocated
    size_type max_size() const throw ()
    {
        return (std::numeric_limits<std::size_t>::max) () / sizeof(T);
    }

    // allocate but don't initialize num elements of type T
    pointer allocate(size_type num, const void * = 0)
    {
        pointer ret = (pointer)(T::operator new (num * sizeof(T)));
        return ret;
    }

    // initialize elements of allocated storage p with value value
    void construct(pointer p, const T & value)
    {
        // initialize memory with placement new
        new ((void *)p)T(value);
    }

    // destroy elements of initialized storage p
    void destroy(pointer p)
    {
        // destroy objects by calling their destructor
        p->~T();
    }

    // deallocate storage p of deleted elements
    void deallocate(pointer p, size_type /*num*/)
    {
        T::operator delete ((void *)p);
    }
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
inline bool operator == (const new_alloc<T1> &,
                         const new_alloc<T2> &) throw ()
{
    return true;
}

template <class T1, class T2>
inline bool operator != (const new_alloc<T1> &,
                         const new_alloc<T2> &) throw ()
{
    return false;
}

__STXXL_END_NAMESPACE

#endif // !STXXL_UTILS_HEADER
