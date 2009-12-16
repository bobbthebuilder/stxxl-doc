/***************************************************************************
 *  include/stxxl/bits/algo/sort_helper.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_SORT_HELPER_HEADER
#define STXXL_SORT_HELPER_HEADER

#include <functional>
#include <stxxl/bits/algo/run_cursor.h>


__STXXL_BEGIN_NAMESPACE

//! \internal
namespace sort_helper
{
    template <typename BIDTp_, typename ValTp_>
    struct trigger_entry
    {
        typedef BIDTp_ bid_type;
        typedef ValTp_ value_type;

        bid_type bid;
        value_type value;

        operator bid_type ()
        {
            return bid;
        }
    };

    template <typename BIDTp_, typename ValTp_, typename ValueCmp_>
    struct trigger_entry_cmp : public std::binary_function<trigger_entry<BIDTp_, ValTp_>, trigger_entry<BIDTp_, ValTp_>, bool>
    {
        typedef trigger_entry<BIDTp_, ValTp_> trigger_entry_type;
        ValueCmp_ cmp;
        trigger_entry_cmp(ValueCmp_ c) : cmp(c) { }
        trigger_entry_cmp(const trigger_entry_cmp & a) : cmp(a.cmp) { }
        bool operator () (const trigger_entry_type & a, const trigger_entry_type & b) const
        {
            return cmp(a.value, b.value);
        }
    };

    template <typename block_type,
              typename prefetcher_type,
              typename value_cmp>
    struct run_cursor2_cmp : public std::binary_function<run_cursor2<block_type, prefetcher_type>, run_cursor2<block_type, prefetcher_type>, bool>
    {
        typedef run_cursor2<block_type, prefetcher_type> cursor_type;
        value_cmp cmp;

        run_cursor2_cmp(value_cmp c) : cmp(c) { }
        run_cursor2_cmp(const run_cursor2_cmp & a) : cmp(a.cmp) { }
        inline bool operator () (const cursor_type & a, const cursor_type & b) const
        {
            if (UNLIKELY(b.empty()))
                return true;
            // sentinel emulation
            if (UNLIKELY(a.empty()))
                return false;
            // sentinel emulation

            return (cmp(a.current(), b.current()));
        }
    };

    // this function is used by parallel mergers
    // returns true if new data was loaded
    template <typename SequenceVector, typename BufferPtrVector, typename Prefetcher>
    inline
    bool refill_or_remove_empty_sequences(SequenceVector & seqs,
                                          BufferPtrVector & buffers,
                                          Prefetcher & prefetcher)
    {
        typedef typename SequenceVector::size_type seqs_size_type;
        bool did_load = false;

        for (seqs_size_type i = 0; i < seqs.size(); ++i)
        {
            if (seqs[i].first == seqs[i].second)                // run empty
            {
                if (prefetcher.block_consumed(buffers[i]))
                {
                    seqs[i].first = buffers[i]->begin();        // reset iterator
                    seqs[i].second = buffers[i]->end();
                    STXXL_VERBOSE1("block ran empty " << i);
                    did_load = true;
                }
                else
                {
                    seqs.erase(seqs.begin() + i);               // remove this sequence
                    buffers.erase(buffers.begin() + i);
                    STXXL_VERBOSE1("seq removed " << i);
                    --i;                                        // don't skip the next sequence
                }
            }
        }
        return did_load;
    }
}

__STXXL_END_NAMESPACE

#endif // !STXXL_SORT_HELPER_HEADER
// vim: et:ts=4:sw=4
