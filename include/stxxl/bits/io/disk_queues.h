/***************************************************************************
 *  include/stxxl/bits/io/disk_queues.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_DISK_QUEUES_HEADER
#define STXXL_IO_DISK_QUEUES_HEADER

#include <map>

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/singleton.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/disk_queue.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

typedef stxxl::int64 DISKID;

//! \brief Encapsulates disk queues
//! \remark is a singleton
class disk_queues : public singleton<disk_queues>
{
    friend class singleton<disk_queues>;

protected:
    std::map<DISKID, disk_queue *> queues;
    disk_queues() { }

public:
    void add_readreq(request_ptr & req, DISKID disk)
    {
        if (queues.find(disk) == queues.end())
        {
            // create new disk queue
            queues[disk] = new disk_queue();
        }
        queues[disk]->add_readreq(req);
    }
    void add_writereq(request_ptr & req, DISKID disk)
    {
        if (queues.find(disk) == queues.end())
        {
            // create new disk queue
            queues[disk] = new disk_queue();
        }
        queues[disk]->add_writereq(req);
    }
    ~disk_queues()
    {
        // deallocate all queues
        for (std::map<DISKID, disk_queue *>::iterator i =
                 queues.begin(); i != queues.end(); i++)
            delete (*i).second;
    }
    //! \brief Changes requests priorities
    //! \param op one of:
    //!                 - READ, read requests are served before write requests within a disk queue
    //!                 - WRITE, write requests are served before read requests within a disk queue
    //!                 - NONE, read and write requests are served by turns, alternately
    void set_priority_op(disk_queue::priority_op op)
    {
        for (std::map<DISKID, disk_queue *>::iterator i =
                 queues.begin(); i != queues.end(); i++)
            i->second->set_priority_op(op);
    }
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO_DISK_QUEUES_HEADER
// vim: et:ts=4:sw=4
