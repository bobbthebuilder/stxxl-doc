/***************************************************************************
 *  include/stxxl/bits/algo/sort_helper.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_SORT_HELPER_HEADER
#define STXXL_SORT_HELPER_HEADER

#include <algorithm>
#include <functional>
#include <stxxl/bits/algo/run_cursor.h>
#include <stxxl/bits/verbose.h>


__STXXL_BEGIN_NAMESPACE

//! \internal
namespace sort_helper
{
    template <typename StrictWeakOrdering>
    inline void verify_sentinel_strict_weak_ordering(StrictWeakOrdering cmp)
    {
        assert(!cmp(cmp.min_value(), cmp.min_value()));
        assert(cmp(cmp.min_value(), cmp.max_value()));
        assert(!cmp(cmp.max_value(), cmp.min_value()));
        assert(!cmp(cmp.max_value(), cmp.max_value()));
        STXXL_UNUSED(cmp);
    }

    template <typename BlockTp_, typename ValTp_ = typename BlockTp_::value_type>
    struct trigger_entry
    {
        typedef BlockTp_ block_type;
        typedef typename block_type::bid_type bid_type;
        typedef ValTp_ value_type;

        bid_type bid;
        value_type value;

        operator bid_type ()
        {
            return bid;
        }
    };

    template <typename TriggerEntryTp_, typename ValueCmp_>
    struct trigger_entry_cmp : public std::binary_function<TriggerEntryTp_, TriggerEntryTp_, bool>
    {
        typedef TriggerEntryTp_ trigger_entry_type;
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
    template <typename SequenceVector, typename ValueType, typename Comparator>
    inline
    unsigned_type count_elements_less_equal(const SequenceVector & seqs, const ValueType & bound, Comparator cmp)
    {
        typedef typename SequenceVector::size_type seqs_size_type;
        typedef typename SequenceVector::value_type::first_type iterator;
        unsigned_type count = 0;

        for (seqs_size_type i = 0; i < seqs.size(); ++i)
        {
            iterator position = std::upper_bound(seqs[i].first, seqs[i].second, bound, cmp);
            STXXL_VERBOSE1("less equal than " << position - seqs[i].first);
            count += position - seqs[i].first;
        }
        STXXL_VERBOSE1("finished loop");
        return count;
    }

    // this function is used by parallel mergers
    template <typename SequenceVector, typename BufferPtrVector, typename Prefetcher>
    inline
    void refill_or_remove_empty_sequences(SequenceVector & seqs,
                                          BufferPtrVector & buffers,
                                          Prefetcher & prefetcher)
    {
        typedef typename SequenceVector::size_type seqs_size_type;

        for (seqs_size_type i = 0; i < seqs.size(); ++i)
        {
            if (seqs[i].first == seqs[i].second)                // run empty
            {
                if (prefetcher.block_consumed(buffers[i]))
                {
                    seqs[i].first = buffers[i]->begin();        // reset iterator
                    seqs[i].second = buffers[i]->end();
                    STXXL_VERBOSE1("block ran empty " << i);
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
    }
}

__STXXL_END_NAMESPACE

#endif // !STXXL_SORT_HELPER_HEADER
// vim: et:ts=4:sw=4
