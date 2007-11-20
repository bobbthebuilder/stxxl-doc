#ifndef IN_MEMORY_SORT_HEADER
#define IN_MEMORY_SORT_HEADER

/***************************************************************************
 *            inmemsort.h
 *
 *  Mon Apr  7 14:23:34 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stxxl/bits/namespace.h"
#include "stxxl/bits/common/simple_vector.h"
#include "stxxl/bits/algo/adaptor.h"
#include "stxxl/bits/mng/adaptor.h"

#include <algorithm>


__STXXL_BEGIN_NAMESPACE

template <typename ExtIterator_, typename StrictWeakOrdering_>
void stl_in_memory_sort(ExtIterator_ first, ExtIterator_ last, StrictWeakOrdering_ cmp)
{
    typedef typename ExtIterator_::vector_type::value_type value_type;
    typedef typename ExtIterator_::block_type block_type;

    STXXL_VERBOSE("stl_in_memory_sort, range: " << (last - first) );
    unsigned_type nblocks = last.bid() - first.bid() + (last.block_offset() ? 1 : 0);
    simple_vector<block_type> blocks(nblocks);
    simple_vector<request_ptr> reqs(nblocks);
    unsigned_type i;

    for (i = 0; i < nblocks; ++i)
        reqs[i] = blocks[i].read(*(first.bid() + i));


    wait_all(reqs.begin(), nblocks);

    unsigned_type last_block_correction = last.block_offset() ? (block_type::size - last.block_offset()) : 0;
    if (block_type::has_filler)
        std::sort(
            ArrayOfSequencesIterator<
                block_type,
                typename block_type::value_type,
                block_type::size>
              (blocks.begin(), first.block_offset()),
            ArrayOfSequencesIterator<
                block_type,
                typename block_type::value_type,
                block_type::size>
              (blocks.begin(), nblocks * block_type::size - last_block_correction),
/*            TwoToOneDimArrayRowAdaptor < block_type,
            typename block_type::value_type, block_type::size > (blocks.begin(), first.block_offset() ),
            TwoToOneDimArrayRowAdaptor < block_type,
            typename block_type::value_type, block_type::size > (blocks.begin(),
                                                                 nblocks * block_type::size - last_block_correction),*/
            cmp);

    else
        std::sort(blocks[0].elem + first.block_offset(),
                  blocks[nblocks].elem - last_block_correction, cmp);


    for (i = 0; i < nblocks; ++i)
        reqs[i] = blocks[i].write(*(first.bid() + i));


    wait_all(reqs.begin(), nblocks);
}


__STXXL_END_NAMESPACE

#endif
