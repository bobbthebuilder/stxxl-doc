/***************************************************************************
 *  include/stxxl/bits/parallel.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_PARALLEL_HEADER
#define STXXL_PARALLEL_HEADER


#undef STXXL_PARALLEL
#if defined(_GLIBCXX_PARALLEL) && defined (__MCSTL__)
#error Both _GLIBCXX_PARALLEL and __MCSTL__ are defined
#endif
#if defined(_GLIBCXX_PARALLEL) || defined (__MCSTL__)
#define STXXL_PARALLEL 1
#else
#define STXXL_PARALLEL 0
#endif


#include <cassert>

#ifdef _GLIBCXX_PARALLEL
 #include <omp.h>
#endif

#ifdef __MCSTL__
 #include <mcstl.h>
 #include <bits/mcstl_multiway_merge.h>
#endif

#if STXXL_PARALLEL
 #include <algorithm>
#endif

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/common/settings.h>


#if defined(_GLIBCXX_PARALLEL)
#define __STXXL_FORCE_SEQUENTIAL , __gnu_parallel::sequential_tag()
#elif defined(__MCSTL__)
#define __STXXL_FORCE_SEQUENTIAL , mcstl::sequential_tag()
#else
#define __STXXL_FORCE_SEQUENTIAL
#endif

#if !STXXL_PARALLEL
#undef STXXL_PARALLEL_MULTIWAY_MERGE
#define STXXL_PARALLEL_MULTIWAY_MERGE 0
#endif

#if !defined(STXXL_PARALLEL_MULTIWAY_MERGE)
#define STXXL_PARALLEL_MULTIWAY_MERGE 1
#endif

#if !defined(STXXL_NOT_CONSIDER_SORT_MEMORY_OVERHEAD)
#define STXXL_NOT_CONSIDER_SORT_MEMORY_OVERHEAD 0
#endif


__STXXL_BEGIN_NAMESPACE

inline unsigned sort_memory_usage_factor()
{
#if STXXL_PARALLEL && !STXXL_NOT_CONSIDER_SORT_MEMORY_OVERHEAD && defined(_GLIBCXX_PARALLEL)
    return (__gnu_parallel::_Settings::get().sort_algorithm == __gnu_parallel::MWMS && omp_get_max_threads() > 1) ? 2 : 1;   //memory overhead for multiway mergesort
#elif STXXL_PARALLEL && !STXXL_NOT_CONSIDER_SORT_MEMORY_OVERHEAD && defined(__MCSTL__)
    return (mcstl::SETTINGS::sort_algorithm == mcstl::SETTINGS::MWMS && mcstl::SETTINGS::num_threads > 1) ? 2 : 1;           //memory overhead for multiway mergesort
#else
    return 1;                                                                                                                //no overhead
#endif
}

inline bool do_parallel_merge()
{
#if STXXL_PARALLEL_MULTIWAY_MERGE && defined(_GLIBCXX_PARALLEL)
    return !stxxl::SETTINGS::native_merge && omp_get_max_threads() >= 1;
#elif STXXL_PARALLEL_MULTIWAY_MERGE && defined(__MCSTL__)
    return !stxxl::SETTINGS::native_merge && mcstl::SETTINGS::num_threads >= 1;
#else
    return false;
#endif
}


namespace parallel
{
#if STXXL_PARALLEL

/** @brief Multi-way merging dispatcher.
 *  @param seqs_begin Begin iterator of iterator pair input sequence.
 *  @param seqs_end End iterator of iterator pair input sequence.
 *  @param target Begin iterator out output sequence.
 *  @param comp Comparator.
 *  @param length Maximum length to merge.
 *  @return End iterator of output sequence. */
    template <typename RandomAccessIteratorPairIterator,
              typename RandomAccessIterator3, typename DiffType, typename Comparator>
    RandomAccessIterator3
    multiway_merge(RandomAccessIteratorPairIterator seqs_begin,
                   RandomAccessIteratorPairIterator seqs_end,
                   RandomAccessIterator3 target,
                   Comparator comp,
                   DiffType length)
    {
#if defined(_GLIBCXX_PARALLEL) && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40400)
        return __gnu_parallel::multiway_merge(seqs_begin, seqs_end, target, length, comp);
#elif defined(_GLIBCXX_PARALLEL)
        return __gnu_parallel::multiway_merge(seqs_begin, seqs_end, target, comp, length);
#elif defined(__MCSTL__)
        return mcstl::multiway_merge(seqs_begin, seqs_end, target, comp, length, false);
#else
        assert(0);
        abort();
#endif
    }

/** @brief Stable multi-way merging dispatcher.
 *  @param seqs_begin Begin iterator of iterator pair input sequence.
 *  @param seqs_end End iterator of iterator pair input sequence.
 *  @param target Begin iterator out output sequence.
 *  @param comp Comparator.
 *  @param length Maximum length to merge.
 *  @return End iterator of output sequence. */
    template <typename RandomAccessIteratorPairIterator,
              typename RandomAccessIterator3, typename DiffType, typename Comparator>
    RandomAccessIterator3
    multiway_merge_stable(RandomAccessIteratorPairIterator seqs_begin,
                   RandomAccessIteratorPairIterator seqs_end,
                   RandomAccessIterator3 target,
                   Comparator comp,
                   DiffType length)
    {
#if defined(_GLIBCXX_PARALLEL) && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40400)
        return __gnu_parallel::stable_multiway_merge(seqs_begin, seqs_end, target, length, comp);
#elif defined(_GLIBCXX_PARALLEL)
        return __gnu_parallel::stable_multiway_merge(seqs_begin, seqs_end, target, comp, length);
#elif defined(__MCSTL__)
        return mcstl::multiway_merge(seqs_begin, seqs_end, target, comp, length, true);
#else
        assert(0);
        abort();
#endif
    }
#endif
}

__STXXL_END_NAMESPACE

#endif // !STXXL_PARALLEL_HEADER
