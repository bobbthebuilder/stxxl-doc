#ifndef IOSTATS_HEADER
#define IOSTATS_HEADER

/***************************************************************************
 *            iostats.h
 *
 *  Sat Aug 24 23:54:50 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stxxl/bits/namespace.h"
#include "stxxl/bits/common/mutex.h"
#include "stxxl/bits/common/types.h"

#include <iostream>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//!
//! \{

#ifdef COUNT_WAIT_TIME
extern double wait_time_counter;
#endif

class disk_queue;

//! \brief Collects various I/O statistics
//! \remarks is a singleton
class stats
{
    friend class disk_queue;
    unsigned reads, writes;             // number of operations
    int64 volume_read, volume_written;            // number of bytes read/written
    double t_reads, t_writes;                   //  seconds spent in operations
    double p_reads, p_writes;                   // seconds spent in parallel operations
    double p_begin_read, p_begin_write;                 // start time of parallel operation
    double p_ios;               // seconds spent in all parallel I/O operations (read and write)
    double p_begin_io;
    int acc_ios;
    int acc_reads, acc_writes;                  // number of requests, participating in parallel operation
    double last_reset;
#ifdef STXXL_BOOST_THREADS
    boost::mutex read_mutex, write_mutex, io_mutex;
#else
    mutex read_mutex, write_mutex, io_mutex;
#endif

    static stats * instance;

    stats ();
public:
    //! \brief Call this function in order to access an instance of stats
    //! \return pointer to an instance of stats
    static stats * get_instance ();
    //! \brief Returns total number of reads
    //! \return total number of reads
    unsigned get_reads () const;
    //! \brief Returns total number of writes
    //! \return total number of writes
    unsigned get_writes () const;
    //! \brief Returns number of bytes read from disks
    //! \return number of bytes read
    int64 get_read_volume () const;
    //! \brief Returns number of bytes written to the disks
    //! \return number of bytes written
    int64 get_written_volume () const;
    //! \brief Time that would be spent in read syscalls if all parallel reads were serialized.
    //! \return seconds spent in reading
    double get_read_time () const;
    //! \brief Time that would be spent in write syscalls if all parallel writes were serialized.
    //! \return seconds spent in writing
    double get_write_time () const;
    //! \brief Period of time when at least one I/O thread was executing a read.
    //! \return seconds spent in reading
    double get_pread_time() const;
    //! \brief Period of time when at least one I/O thread was executing a write.
    //! \return seconds spent in writing
    double get_pwrite_time() const;
    //! \brief Period of time when at least one I/O thread was executing a read or a write.
    //! \return seconds spent in I/O
    double get_pio_time() const;
    //! \brief Return time of the last reset
    //! \return seconds passed from the last reset()
    double get_last_reset_time() const;
    //! \brief Resets I/O time counters (including I/O wait counter)
    void reset();


#ifdef COUNT_WAIT_TIME
    //! \brief Resets I/O wait time counter
    void _reset_io_wait_time();
    // void reset_io_wait_time() { stxxl::wait_time_counter = 0.0; }
    //! \brief I/O wait time counter
    //! \return number of seconds spent in I/O waiting functions
    //!  \link request::wait request::wait \endlink,
    //!  \c wait_any and
    //!  \c wait_all
    double get_io_wait_time() const;
    //! \brief Increments I/O wait time counter
    //! \param val increment value in seconds
    //! \return new value of I/O wait time counter in seconds
    double increment_io_wait_time(double val);
#else
    //! \brief Resets I/O wait time counter
    void _reset_io_wait_time();
    //! \brief Returns I/O wait time counter
    //! \return number of seconds spent in I/O waiting functions
    //!  \link request::wait request::wait \endlink,
    //!  \c wait_any and
    //!  \c wait_all
    //! \return number of seconds
    double get_io_wait_time() const;
    //! \brief Increments I/O wait time counter
    //! \param val increment value in seconds
    //! \return new value of I/O wait time counter in seconds
    double increment_io_wait_time(double val);
#endif

    // for library use
    void write_started (unsigned size_);
    void write_finished ();
    void read_started (unsigned size_);
    void read_finished ();
};

#ifndef DISKQUEUE_HEADER

std::ostream & operator << (std::ostream & o, const stats & s);

#endif

//! \}

__STXXL_END_NAMESPACE

#endif
