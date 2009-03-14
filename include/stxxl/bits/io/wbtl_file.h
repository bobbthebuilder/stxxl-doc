/***************************************************************************
 *  include/stxxl/bits/io/wbtl_file.h
 *
 *  a write-buffered-translation-layer pseudo file
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008-2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_WBTL_FILE_HEADER
#define STXXL_WBTL_FILE_HEADER

#include <map>

#include <stxxl/bits/io/file_request_basic.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implementation of file based on buffered writes and
//!        block remapping via a translation layer.
class wbtl_file : public file_request_basic
{
    typedef std::pair<offset_type, offset_type> place;
    typedef std::map<offset_type, offset_type> sortseq;
    typedef std::map<offset_type, place> place_map;

    // thy physical disk use as backend
    file * storage;
    offset_type sz;
    size_type write_block_size;

    mutex mapping_mutex;
    // logical to physical address translation
    sortseq address_mapping;
    // physical to (logical address, size) translation
    place_map reverse_mapping;
    // list of free (physical) regions
    sortseq free_space;
    offset_type free_bytes;

    // the write buffers:
    // write_buffer[curbuf] is the current write buffer
    // write_buffer[1-curbuf] is the previous write buffer
    // buffer_address if the start offset on the backend file
    // curpos is the next writing position in write_buffer[curbuf]
    mutex buffer_mutex;
    char * write_buffer[2];
    offset_type buffer_address[2];
    int curbuf;
    size_type curpos;
    request_ptr backend_request;

    struct FirstFit : public std::binary_function<place, offset_type, bool>
    {
        bool operator () (
            const place & entry,
            const offset_type size) const
        {
            return (entry.second >= size);
        }
    };

public:
    //! \brief constructs file object
    //! \param filename path of file
    //! \attention filename must be resided at memory disk partition
    //! \param mode open mode, see \c stxxl::file::open_modes
    //! \param disk disk(file) identifier
    wbtl_file(
        const std::string & filename,
        int mode,
        int disk = -1);
    ~wbtl_file();
    request_ptr aread(
        void * buffer,
        offset_type pos,
        size_t bytes,
        completion_handler on_cmpl);
    offset_type size();
    void set_size(offset_type newsize);
    void lock();
    void serve(const request * req) throw(io_error);
    void delete_region(offset_type offset, size_type size);
    const char * io_type() const;

private:
    void _add_free_region(offset_type offset, offset_type size);
protected:
    void sread(void * buffer, offset_type offset, size_type bytes);
    void swrite(void * buffer, offset_type offset, size_type bytes);
    offset_type get_next_write_block();
    void check_corruption(offset_type region_pos, offset_type region_size,
                          sortseq::iterator pred, sortseq::iterator succ);
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_WBTL_FILE_HEADER
