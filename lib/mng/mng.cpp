/***************************************************************************
 *  mng/mng.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/mng/mng.h>


__STXXL_BEGIN_NAMESPACE

block_manager::block_manager()
{
    config * cfg = config::get_instance();

    ndisks = cfg->disks_number();
    disk_allocators = new DiskAllocator *[ndisks];
    disk_files = new file *[ndisks];

    for (unsigned i = 0; i < ndisks; ++i)
    {
        disk_files[i] = create_file(cfg->disk_io_impl(i),
                                    cfg->disk_path(i),
                                    file::CREAT | file::RDWR | file::DIRECT,
                                    i,          // physical_device_id
                                    i);         // allocator_id
        disk_allocators[i] = new DiskAllocator(disk_files[i], cfg->disk_size(i));
    }

#if STXXL_MNG_COUNT_ALLOCATION
    m_current_allocation = 0;
    m_total_allocation = 0;
    m_maximum_allocation = 0;
#endif // STXXL_MNG_COUNT_ALLOCATION
}

block_manager::~block_manager()
{
    STXXL_VERBOSE1("Block manager destructor");
    for (size_t i = ndisks; i > 0; )
    {
        --i;
        delete disk_allocators[i];
        delete disk_files[i];
    }
    delete[] disk_allocators;
    delete[] disk_files;
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
