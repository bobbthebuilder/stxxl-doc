/***************************************************************************
 *  include/stxxl/bits/containers/matrix.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MATRIX_HEADER
#define STXXL_MATRIX_HEADER

#ifndef STXXL_BLAS
#define STXXL_BLAS 0
#endif

#include <complex>

#include <stxxl/bits/containers/matrix_layouts.h>
#include <stxxl/bits/mng/block_scheduler.h>
#include <stxxl/bits/common/shared_object.h>
#include <stxxl/bits/containers/vector.h>
#include <stxxl/bits/containers/matrix_low_level.h>


__STXXL_BEGIN_NAMESPACE

// +-+-+-+-+-+-+ matrix version with swappable_blocks +-+-+-+-+-+-+-+-+-+-+-+-+-+

/* index-variable naming convention:
 * [MODIFIER_][UNIT_]DIMENSION[_in_[MODIFIER_]ENVIRONMENT]
 *
 * e.g.:
 * block_row = number of row measured in rows consisting of blocks
 * element_row_in_block = number of row measured in rows consisting of elements in the (row of) block(s)
 *
 * size-variable naming convention:
 * [MODIFIER_][ENVIRONMENT_]DIMENSION[_in_UNITs]
 *
 * e.g.
 * height_in_blocks
 */

template <typename ValueType, unsigned BlockSideLength> class matrix;

struct matrix_operation_statistic_dataset
{
    int_type block_multiplication_calls,
             block_multiplications_saved_through_zero,
             block_addition_calls,
             block_additions_saved_through_zero;

    matrix_operation_statistic_dataset()
        : block_multiplication_calls(0),
          block_multiplications_saved_through_zero(0),
          block_addition_calls(0),
          block_additions_saved_through_zero(0) {}

    matrix_operation_statistic_dataset operator + (const matrix_operation_statistic_dataset & stat)
    {
        matrix_operation_statistic_dataset res(*this);
        res.block_multiplication_calls += stat.block_multiplication_calls;
        res.block_multiplications_saved_through_zero += stat.block_multiplications_saved_through_zero;
        res.block_addition_calls += stat.block_addition_calls;
        res.block_additions_saved_through_zero += stat.block_additions_saved_through_zero;
        return res;
    }

    matrix_operation_statistic_dataset operator - (const matrix_operation_statistic_dataset & stat)
    {
        matrix_operation_statistic_dataset res(*this);
        res.block_multiplication_calls -= stat.block_multiplication_calls;
        res.block_multiplications_saved_through_zero -= stat.block_multiplications_saved_through_zero;
        res.block_addition_calls -= stat.block_addition_calls;
        res.block_additions_saved_through_zero -= stat.block_additions_saved_through_zero;
        return res;
    }
};

struct matrix_operation_statistic
    : public singleton<matrix_operation_statistic>, public matrix_operation_statistic_dataset
{};

struct matrix_operation_statistic_data : public matrix_operation_statistic_dataset
{
    matrix_operation_statistic_data(const matrix_operation_statistic & stat = * matrix_operation_statistic::get_instance())
        : matrix_operation_statistic_dataset(stat) {}

    matrix_operation_statistic_data(const matrix_operation_statistic_dataset & stat)
        : matrix_operation_statistic_dataset(stat) {}

    matrix_operation_statistic_data & operator = (const matrix_operation_statistic & stat)
    {
        return *this = matrix_operation_statistic_data(stat);
    }

    void set()
    { operator = (* matrix_operation_statistic::get_instance()); }

    matrix_operation_statistic_data operator + (const matrix_operation_statistic_data & stat)
    { return matrix_operation_statistic_data(matrix_operation_statistic_dataset(*this) + matrix_operation_statistic_dataset(stat)); }

    matrix_operation_statistic_data operator - (const matrix_operation_statistic_data & stat)
    { return matrix_operation_statistic_data(matrix_operation_statistic_dataset(*this) - matrix_operation_statistic_dataset(stat)); }
};

std::ostream & operator << (std::ostream & o, const matrix_operation_statistic_data & statsd)
{
    o << "matrix operation statistics" << std::endl;
    o << "block multiplication calls                     : "
            << statsd.block_multiplication_calls << std::endl;
    o << "block multiplications saved through zero blocks: "
            << statsd.block_multiplications_saved_through_zero << std::endl;
    o << "block multiplications performed                : "
            << statsd.block_multiplication_calls - statsd.block_multiplications_saved_through_zero << std::endl;
    o << "block addition calls                           : "
            << statsd.block_addition_calls << std::endl;
    o << "block additions saved through zero blocks      : "
            << statsd.block_additions_saved_through_zero << std::endl;
    o << "block additions performed                      : "
            << statsd.block_addition_calls - statsd.block_additions_saved_through_zero << std::endl;
    return o;
}

//! \brief External column-vector container.
template <typename ValueType>
class column_vector : public vector<ValueType>
{
public:
    typedef vector<ValueType> vector_type;
    typedef typename vector_type::size_type size_type;

    using vector_type::size;

    column_vector(size_type n = 0)
        : vector_type(n) {}

    column_vector operator + (const column_vector & right) const
    {
        assert(size() == right.size());
        column_vector res(size());
        for (size_type i = 0; i < size(); ++i)
            res[i] = (*this)[i] + right[i];
        return res;
    }

    column_vector operator - (const column_vector & right) const
    {
        assert(size() == right.size());
        column_vector res(size());
        for (size_type i = 0; i < size(); ++i)
            res[i] = (*this)[i] - right[i];
        return res;
    }

    column_vector operator * (const ValueType scalar) const
    {
        column_vector res(size());
        for (size_type i = 0; i < size(); ++i)
            res[i] = (*this)[i] * scalar;
        return res;
    }

    column_vector & operator += (const column_vector & right)
    {
        assert(size() == right.size());
        for (size_type i = 0; i < size(); ++i)
            (*this)[i] += right[i];
        return *this;
    }

    column_vector & operator -= (const column_vector & right)
    {
        assert(size() == right.size());
        for (size_type i = 0; i < size(); ++i)
            (*this)[i] -= right[i];
        return *this;
    }

    column_vector & operator *= (const ValueType scalar)
    {
        for (size_type i = 0; i < size(); ++i)
            (*this)[i] *= scalar;
        return *this;
    }

    void set_zero()
    {
        for (typename vector_type::iterator it = vector_type::begin(); it != vector_type::end(); ++it)
            *it = 0;
    }
};

//! \brief External row-vector container.
template <typename ValueType>
class row_vector : public vector<ValueType>
{
public:
    typedef vector<ValueType> vector_type;
    typedef typename vector_type::size_type size_type;

    using vector_type::size;

    row_vector(size_type n = 0)
        : vector_type(n) {}

    row_vector operator + (const row_vector & right) const
    {
        assert(size() == right.size());
        row_vector res(size());
        for (size_type i = 0; i < size(); ++i)
            res[i] = (*this)[i] + right[i];
        return res;
    }

    row_vector operator - (const row_vector & right) const
    {
        assert(size() == right.size());
        row_vector res(size());
        for (size_type i = 0; i < size(); ++i)
            res[i] = (*this)[i] - right[i];
        return res;
    }

    row_vector operator * (const ValueType scalar) const
    {
        row_vector res(size());
        for (size_type i = 0; i < size(); ++i)
            res[i] = (*this)[i] * scalar;
        return res;
    }

    template <unsigned BlockSideLength>
    row_vector operator * (const matrix<ValueType, BlockSideLength> & right) const
    { return right.multiply_from_left(*this); }

    ValueType operator * (const column_vector<ValueType> & right) const
    {
        ValueType res = 0;
        for (size_type i = 0; i < size(); ++i)
            res += (*this)[i] * right[i];
        return res;
    }

    row_vector & operator += (const row_vector & right)
    {
        assert(size() == right.size());
        for (size_type i = 0; i < size(); ++i)
            (*this)[i] += right[i];
        return *this;
    }

    row_vector & operator -= (const row_vector & right)
    {
        assert(size() == right.size());
        for (size_type i = 0; i < size(); ++i)
            (*this)[i] -= right[i];
        return *this;
    }

    row_vector & operator *= (const ValueType scalar)
    {
        for (size_type i = 0; i < size(); ++i)
            (*this)[i] *= scalar;
        return *this;
    }

    void set_zero()
    {
        for (typename vector_type::iterator it = vector_type::begin(); it != vector_type::end(); ++it)
            *it = 0;
    }
};

template <typename ValueType, unsigned BlockSideLength>
struct matrix_operations;

//! \brief Specialized swappable_block that interprets uninitialized as containing zeros.
//! When initializing, all values are set to zero.
template <typename ValueType, unsigned BlockSideLength>
class matrix_swappable_block : public swappable_block<ValueType, BlockSideLength * BlockSideLength>
{
public:
    typedef typename swappable_block<ValueType, BlockSideLength * BlockSideLength>::internal_block_type internal_block_type;

    using swappable_block<ValueType, BlockSideLength * BlockSideLength>::get_internal_block;

    void fill_default()
    {
        // get_internal_block checks acquired
        internal_block_type & data = get_internal_block();
        #if STXXL_PARALLEL
        #pragma omp parallel for
        #endif
        for (int_type row = 0; row < int_type(BlockSideLength); ++row)
            for (int_type col = 0; col < int_type(BlockSideLength); ++col)
                data[row * BlockSideLength + col] = 0;
    }
};

//! \brief External container for the values of a (sub)matrix. Not intended for direct use.
//!
//! Stores blocks only, so all measures (height, width, row, col) are in blocks.
template <typename ValueType, unsigned BlockSideLength>
class swappable_block_matrix : public shared_object
{
public:
    typedef int_type size_type;
    typedef int_type elem_size_type;
    typedef block_scheduler< matrix_swappable_block<ValueType, BlockSideLength> > block_scheduler_type;
    typedef typename block_scheduler_type::swappable_block_identifier_type swappable_block_identifier_type;
    typedef std::vector<swappable_block_identifier_type> blocks_type;
    typedef matrix_operations<ValueType, BlockSideLength> Ops;

    block_scheduler_type & bs;

private:
    // assigning is not allowed
    swappable_block_matrix & operator = (const swappable_block_matrix & other);

protected:
    //! \brief height of the matrix in blocks
    size_type height,
    //! \brief width of the matrix in blocks
              width,
    //! \brief height copied from supermatrix
              height_from_supermatrix,
    //! \brief width copied from supermatrix
              width_from_supermatrix;
    //! \brief the matrice's blocks in row-major
    blocks_type blocks;
    //! \brief if the elements in each block are in col-major instead of row-major
    bool elements_in_blocks_transposed;

    swappable_block_identifier_type & bl(const size_type row, const size_type col)
    { return blocks[row * width + col]; }

public:
    //! \brief Create an empty swappable_block_matrix of given dimensions.
    swappable_block_matrix(block_scheduler_type & bs, const size_type height_in_blocks, const size_type width_in_blocks, const bool transposed = false)
        : bs(bs),
          height(height_in_blocks),
          width(width_in_blocks),
          height_from_supermatrix(0),
          width_from_supermatrix(0),
          blocks(height * width),
          elements_in_blocks_transposed(transposed)
    {
        for (size_type row = 0; row < height; ++row)
            for (size_type col = 0; col < width; ++col)
                bl(row, col) = bs.allocate_swappable_block();
    }

    //! \brief Create swappable_block_matrix of given dimensions that
    //!        represents the submatrix of supermatrix starting at (from_row_in_blocks, from_col_in_blocks).
    //!
    //! If supermatrix is not large enough, the submatrix is padded with empty blocks.
    //! The supermatrix must not be destructed or transposed before the submatrix is destructed.
    swappable_block_matrix(const swappable_block_matrix & supermatrix,
            const size_type height_in_blocks, const size_type width_in_blocks,
            const size_type from_row_in_blocks, const size_type from_col_in_blocks)
        : bs(supermatrix.bs),
          height(height_in_blocks),
          width(width_in_blocks),
          height_from_supermatrix(std::min(supermatrix.height - from_row_in_blocks, height)),
          width_from_supermatrix(std::min(supermatrix.width - from_col_in_blocks, width)),
          blocks(height * width),
          elements_in_blocks_transposed(supermatrix.elements_in_blocks_transposed)
    {
        for (size_type row = 0; row < height_from_supermatrix; ++row)
        {
            for (size_type col = 0; col < width_from_supermatrix; ++col)
                bl(row, col) = supermatrix.block(row + from_row_in_blocks, col + from_col_in_blocks);
            for (size_type col = width_from_supermatrix; col < width; ++col)
                bl(row, col) = bs.allocate_swappable_block();
        }
        for (size_type row = height_from_supermatrix; row < height; ++row)
            for (size_type col = 0; col < width; ++col)
                bl(row, col) = bs.allocate_swappable_block();
    }

    swappable_block_matrix(const swappable_block_matrix & other)
        : shared_object(other),
          bs(other.bs),
          height(other.height),
          width(other.width),
          height_from_supermatrix(0),
          width_from_supermatrix(0),
          blocks(height * width),
          elements_in_blocks_transposed(false)
    {
        for (size_type row = 0; row < height; ++row)
            for (size_type col = 0; col < width; ++col)
                bl(row, col) = bs.allocate_swappable_block();
        // * 1 is copying
        Ops::element_op(*this, other, typename Ops::scalar_multiplication(1));
    }

    ~swappable_block_matrix()
    {
        for (size_type row = 0; row < height_from_supermatrix; ++row)
        {
            for (size_type col = width_from_supermatrix; col < width; ++col)
                bs.free_swappable_block(bl(row, col));
        }
        for (size_type row = height_from_supermatrix; row < height; ++row)
            for (size_type col = 0; col < width; ++col)
                bs.free_swappable_block(bl(row, col));
    }

    static size_type block_index_from_elem(elem_size_type index)
    { return index / BlockSideLength; }

    static int_type elem_index_in_block_from_elem(elem_size_type index)
    { return index % BlockSideLength; }

    // takes care about transposed
    int_type elem_index_in_block_from_elem(elem_size_type row, elem_size_type col) const
    {
        return (is_transposed())
                 ? row % BlockSideLength + col % BlockSideLength * BlockSideLength
                 : row % BlockSideLength * BlockSideLength + col % BlockSideLength;
    }

    swappable_block_identifier_type block(const size_type row, const size_type col) const
    { return blocks[row * width + col]; }

    swappable_block_identifier_type operator () (const size_type row, const size_type col) const
    { return block(row, col); }

    const size_type & get_height() const
    { return height; }

    const size_type & get_width() const
    { return width; }

    const bool & is_transposed() const
    { return elements_in_blocks_transposed; }

    void transpose()
    {
        // transpose matrix of blocks
        blocks_type bl(blocks.size());
        for (size_type row = 1; row < height; ++row)
            for (size_type col = 0; col < row; ++col)
                bl[col * height + row] = bl(row,col);
        bl.swap(blocks);
        // swap dimensions
        std::swap(height, width);
        std::swap(height_from_supermatrix, width_from_supermatrix);
        elements_in_blocks_transposed = ! elements_in_blocks_transposed;
    }

    void set_zero()
    {
        for (typename blocks_type::iterator it = blocks.begin(); it != blocks.end(); ++it)
            bs.deinitialize(*it);
    }
};

template <typename ValueType, unsigned BlockSideLength>
class matrix_iterator
{
protected:
    typedef matrix<ValueType, BlockSideLength> matrix_type;
    typedef typename matrix_type::swappable_block_matrix_type swappable_block_matrix_type;
    typedef typename matrix_type::block_scheduler_type block_scheduler_type;
    typedef typename block_scheduler_type::internal_block_type internal_block_type;
    typedef typename matrix_type::elem_size_type elem_size_type;
    typedef typename matrix_type::block_size_type block_size_type;

    template <typename VT, unsigned BSL> friend class matrix;
    template <typename VT, unsigned BSL> friend class const_matrix_iterator;

    matrix_type * m;
    elem_size_type current_row, // \ both indices == -1 <=> empty iterator
                   current_col; // /
    block_size_type current_block_row,
                    current_block_col;
    internal_block_type * current_iblock; // NULL if block is not acquired

    void acquire_current_iblock()
    {
        if (! current_iblock)
            current_iblock = & m->data->bs.acquire(m->data->block(current_block_row, current_block_col));
    }

    void release_current_iblock()
    {
        if (current_iblock)
        {
            m->data->bs.release(m->data->block(current_block_row, current_block_col), true);
            current_iblock = 0;
        }
    }

    //! \brief create iterator pointing to given row and col
    matrix_iterator(matrix_type & matrix, const elem_size_type start_row, const elem_size_type start_col)
        : m(&matrix),
          current_row(start_row),
          current_col(start_col),
          current_block_row(m->data->block_index_from_elem(start_row)),
          current_block_col(m->data->block_index_from_elem(start_col)),
          current_iblock(0) {}

    //! \brief create empty iterator
    matrix_iterator(matrix_type & matrix)
        : m(&matrix),
          current_row(-1), // empty iterator
          current_col(-1),
          current_block_row(-1),
          current_block_col(-1),
          current_iblock(0) {}

    void set_empty()
    {
        release_current_iblock();
        current_row = -1;
        current_col = -1;
        current_block_row = -1;
        current_block_col = -1;
    }
public:
    matrix_iterator(const matrix_iterator & other)
        : m(other.m),
          current_row(other.current_row),
          current_col(other.current_col),
          current_block_row(other.current_block_row),
          current_block_col(other.current_block_col),
          current_iblock(0)
    {
        if (other.current_iblock)
            acquire_current_iblock();
    }

    matrix_iterator & operator = (const matrix_iterator & other)
    {
        set_pos(other.current_row, other.current_col);
        m = other.m;
        if (other.current_iblock)
            acquire_current_iblock();
        return *this;
    }

    ~matrix_iterator()
    { release_current_iblock(); }

    void set_row(const elem_size_type new_row)
    {
        const block_size_type new_block_row = m->data->block_index_from_elem(new_row);
        if (new_block_row != current_block_row)
        {
            release_current_iblock();
            current_block_row = new_block_row;
        }
        current_row = new_row;
    }

    void set_col(const elem_size_type new_col)
    {
        const block_size_type new_block_col = m->data->block_index_from_elem(new_col);
        if (new_block_col != current_block_col)
        {
            release_current_iblock();
            current_block_col = new_block_col;
        }
        current_col = new_col;
    }

    void set_pos(const elem_size_type new_row, const elem_size_type new_col)
    {
        const block_size_type new_block_row = m->data->block_index_from_elem(new_row),
                new_block_col = m->data->block_index_from_elem(new_col);
        if (new_block_col != current_block_col || new_block_row != current_block_row)
        {
            release_current_iblock();
            current_block_row = new_block_row;
            current_block_col = new_block_col;
        }
        current_row = new_row;
        current_col = new_col;
    }

    void set_pos(const std::pair<elem_size_type, elem_size_type> new_pos)
    { set_pos(new_pos.first, new_pos.second); }

    const elem_size_type & get_row() const
    { return current_row; }

    const elem_size_type & get_col() const
    { return current_col; }

    std::pair<elem_size_type, elem_size_type> get_pos() const
    { return std::make_pair(current_row, current_col); }

    bool empty() const
    { return current_row == -1 && current_col == -1; }

    operator bool () const
    { return ! empty(); }

    bool operator == (const matrix_iterator & other) const
    {
        return current_row == other.current_row && current_col == other.current_col && m == other.m;
    }

    // do not store reference
    ValueType & operator * ()
    {
        acquire_current_iblock();
        return (*current_iblock)[m->data->elem_index_in_block_from_elem(current_row, current_col)];
    }
};

template <typename ValueType, unsigned BlockSideLength>
class matrix_row_major_iterator : public matrix_iterator<ValueType, BlockSideLength>
{
protected:
    typedef matrix_iterator<ValueType, BlockSideLength> matrix_iterator_type;
    typedef typename matrix_iterator_type::matrix_type matrix_type;
    typedef typename matrix_iterator_type::elem_size_type elem_size_type;

    template <typename VT, unsigned BSL> friend class matrix;

    using matrix_iterator_type::m;
    using matrix_iterator_type::set_empty;

    //! \brief create iterator pointing to given row and col
    matrix_row_major_iterator(matrix_type & matrix, const elem_size_type start_row, const elem_size_type start_col)
        : matrix_iterator_type(matrix, start_row, start_col) {}

    //! \brief create empty iterator
    matrix_row_major_iterator(matrix_type & matrix)
        : matrix_iterator_type(matrix) {}

public:
    //! \brief convert from matrix_iterator
    matrix_row_major_iterator(const matrix_iterator_type & matrix_iterator)
        : matrix_iterator_type(matrix_iterator) {}

    // Has to be not empty, else behavior is undefined.
    matrix_row_major_iterator & operator ++ ()
    {
        if (get_col() + 1 < m->get_width())
            // => not matrix_row_major_iterator the end of row, move right
            set_col(get_col() + 1);
        else if (get_row() + 1 < m->get_height())
            // => at end of row but not last row, move to beginning of next row
            set_pos(get_row() + 1, 0);
        else
            // => at end of matrix, set to empty-state
            set_empty();
        return *this;
    }

    // Has to be not empty, else behavior is undefined.
    matrix_row_major_iterator & operator -- ()
    {
        if (get_col() - 1 >= 0)
            // => not at the beginning of row, move left
            set_col(get_col() - 1);
        else if (get_row() - 1 >= 0)
            // => at beginning of row but not first row, move to end of previous row
            set_pos(get_row() - 1, m.get_width() - 1);
        else
            // => at beginning of matrix, set to empty-state
            set_empty();
        return *this;
    }

    using matrix_iterator_type::get_row;
    using matrix_iterator_type::get_col;
    using matrix_iterator_type::set_col;
    using matrix_iterator_type::set_pos;
};

template <typename ValueType, unsigned BlockSideLength>
class matrix_col_major_iterator : public matrix_iterator<ValueType, BlockSideLength>
{
protected:
    typedef matrix_iterator<ValueType, BlockSideLength> matrix_iterator_type;
    typedef typename matrix_iterator_type::matrix_type matrix_type;
    typedef typename matrix_iterator_type::elem_size_type elem_size_type;

    template <typename VT, unsigned BSL> friend class matrix;

    using matrix_iterator_type::m;
    using matrix_iterator_type::set_empty;

    //! \brief create iterator pointing to given row and col
    matrix_col_major_iterator(matrix_type & matrix, const elem_size_type start_row, const elem_size_type start_col)
        : matrix_iterator_type(matrix, start_row, start_col) {}

    //! \brief create empty iterator
    matrix_col_major_iterator(matrix_type & matrix)
        : matrix_iterator_type(matrix) {}

public:
    //! \brief convert from matrix_iterator
    matrix_col_major_iterator(const matrix_iterator_type & matrix_iterator)
        : matrix_iterator_type(matrix_iterator) {}

    // Has to be not empty, else behavior is undefined.
    matrix_col_major_iterator & operator ++ ()
    {
        if (get_row() + 1 < m->get_height())
            // => not at the end of col, move down
            set_row(get_row() + 1);
        else if (get_col() + 1 < m->get_width())
            // => at end of col but not last col, move to beginning of next col
            set_pos(0, get_col() + 1);
        else
            // => at end of matrix, set to empty-state
            set_empty();
        return *this;
    }

    // Has to be not empty, else behavior is undefined.
    matrix_col_major_iterator & operator -- ()
    {
        if (get_row() - 1 >= 0)
            // => not at the beginning of col, move up
            set_row(get_row() - 1);
        else if (get_col() - 1 >= 0)
            // => at beginning of col but not first col, move to end of previous col
            set_pos(m->get_height() - 1, get_col() - 1);
        else
            // => at beginning of matrix, set to empty-state
            set_empty();
        return *this;
    }

    using matrix_iterator_type::get_row;
    using matrix_iterator_type::get_col;
    using matrix_iterator_type::set_row;
    using matrix_iterator_type::set_pos;
};

template <typename ValueType, unsigned BlockSideLength>
class const_matrix_iterator
{
protected:
    typedef matrix<ValueType, BlockSideLength> matrix_type;
    typedef typename matrix_type::swappable_block_matrix_type swappable_block_matrix_type;
    typedef typename matrix_type::block_scheduler_type block_scheduler_type;
    typedef typename block_scheduler_type::internal_block_type internal_block_type;
    typedef typename matrix_type::elem_size_type elem_size_type;
    typedef typename matrix_type::block_size_type block_size_type;

    template <typename VT, unsigned BSL> friend class matrix;

    const matrix_type * m;
    elem_size_type current_row, // \ both indices == -1 <=> empty iterator
                   current_col; // /
    block_size_type current_block_row,
                    current_block_col;
    internal_block_type * current_iblock; // NULL if block is not acquired

    void acquire_current_iblock()
    {
        if (! current_iblock)
            current_iblock = & m->data->bs.acquire(m->data->block(current_block_row, current_block_col));
    }

    void release_current_iblock()
    {
        if (current_iblock)
        {
            m->data->bs.release(m->data->block(current_block_row, current_block_col), false);
            current_iblock = 0;
        }
    }

    //! \brief create iterator pointing to given row and col
    const_matrix_iterator(const matrix_type & matrix, const elem_size_type start_row, const elem_size_type start_col)
        : m(&matrix),
          current_row(start_row),
          current_col(start_col),
          current_block_row(m->data->block_index_from_elem(start_row)),
          current_block_col(m->data->block_index_from_elem(start_col)),
          current_iblock(0) {}

    //! \brief create empty iterator
    const_matrix_iterator(const matrix_type & matrix)
        : m(&matrix),
          current_row(-1), // empty iterator
          current_col(-1),
          current_block_row(-1),
          current_block_col(-1),
          current_iblock(0) {}

    void set_empty()
    {
        release_current_iblock();
        current_row = -1;
        current_col = -1;
        current_block_row = -1;
        current_block_col = -1;
    }
public:
    const_matrix_iterator(const matrix_iterator<ValueType, BlockSideLength> & other)
        : m(other.m),
          current_row(other.current_row),
          current_col(other.current_col),
          current_block_row(other.current_block_row),
          current_block_col(other.current_block_col),
          current_iblock(0)
    {
        if (other.current_iblock)
            acquire_current_iblock();
    }

    const_matrix_iterator(const const_matrix_iterator & other)
        : m(other.m),
          current_row(other.current_row),
          current_col(other.current_col),
          current_block_row(other.current_block_row),
          current_block_col(other.current_block_col),
          current_iblock(0)
    {
        if (other.current_iblock)
            acquire_current_iblock();
    }

    const_matrix_iterator & operator = (const const_matrix_iterator & other)
    {
        set_pos(other.current_row, other.current_col);
        m = other.m;
        if (other.current_iblock)
            acquire_current_iblock();
        return *this;
    }

    ~const_matrix_iterator()
    { release_current_iblock(); }

    void set_row(const elem_size_type new_row)
    {
        const block_size_type new_block_row = m->data->block_index_from_elem(new_row);
        if (new_block_row != current_block_row)
        {
            release_current_iblock();
            current_block_row = new_block_row;
        }
        current_row = new_row;
    }

    void set_col(const elem_size_type new_col)
    {
        const block_size_type new_block_col = m->data->block_index_from_elem(new_col);
        if (new_block_col != current_block_col)
        {
            release_current_iblock();
            current_block_col = new_block_col;
        }
        current_col = new_col;
    }

    void set_pos(const elem_size_type new_row, const elem_size_type new_col)
    {
        const block_size_type new_block_row = m->data->block_index_from_elem(new_row),
                new_block_col = m->data->block_index_from_elem(new_col);
        if (new_block_col != current_block_col || new_block_row != current_block_row)
        {
            release_current_iblock();
            current_block_row = new_block_row;
            current_block_col = new_block_col;
        }
        current_row = new_row;
        current_col = new_col;
    }

    void set_pos(const std::pair<elem_size_type, elem_size_type> new_pos)
    { set_pos(new_pos.first, new_pos.second); }

    const elem_size_type & get_row() const
    { return current_row; }

    const elem_size_type & get_col() const
    { return current_col; }

    std::pair<elem_size_type, elem_size_type> get_pos() const
    { return std::make_pair(current_row, current_col); }

    bool empty() const
    { return current_row == -1 && current_col == -1; }

    operator bool () const
    { return ! empty(); }

    bool operator == (const const_matrix_iterator & other) const
    {
        return current_row == other.current_row && current_col == other.current_col && m == other.m;
    }

    // do not store reference
    const ValueType & operator * ()
    {
        acquire_current_iblock();
        return (*current_iblock)[m->data->elem_index_in_block_from_elem(current_row, current_col)];
    }
};

template <typename ValueType, unsigned BlockSideLength>
class const_matrix_row_major_iterator : public const_matrix_iterator<ValueType, BlockSideLength>
{
protected:
    typedef const_matrix_iterator<ValueType, BlockSideLength> const_matrix_iterator_type;
    typedef typename const_matrix_iterator_type::matrix_type matrix_type;
    typedef typename const_matrix_iterator_type::elem_size_type elem_size_type;

    template <typename VT, unsigned BSL> friend class matrix;

    using const_matrix_iterator_type::m;
    using const_matrix_iterator_type::set_empty;

    //! \brief create iterator pointing to given row and col
    const_matrix_row_major_iterator(const matrix_type & matrix, const elem_size_type start_row, const elem_size_type start_col)
        : const_matrix_iterator_type(matrix, start_row, start_col) {}

    //! \brief create empty iterator
    const_matrix_row_major_iterator(const matrix_type & matrix)
        : const_matrix_iterator_type(matrix) {}

public:
    //! \brief convert from matrix_iterator
    const_matrix_row_major_iterator(const const_matrix_row_major_iterator & matrix_iterator)
        : const_matrix_iterator_type(matrix_iterator) {}

    //! \brief convert from matrix_iterator
    const_matrix_row_major_iterator(const const_matrix_iterator_type & matrix_iterator)
        : const_matrix_iterator_type(matrix_iterator) {}

    // Has to be not empty, else behavior is undefined.
    const_matrix_row_major_iterator & operator ++ ()
    {
        if (get_col() + 1 < m->get_width())
            // => not matrix_row_major_iterator the end of row, move right
            set_col(get_col() + 1);
        else if (get_row() + 1 < m->get_height())
            // => at end of row but not last row, move to beginning of next row
            set_pos(get_row() + 1, 0);
        else
            // => at end of matrix, set to empty-state
            set_empty();
        return *this;
    }

    // Has to be not empty, else behavior is undefined.
    const_matrix_row_major_iterator & operator -- ()
    {
        if (get_col() - 1 >= 0)
            // => not at the beginning of row, move left
            set_col(get_col() - 1);
        else if (get_row() - 1 >= 0)
            // => at beginning of row but not first row, move to end of previous row
            set_pos(get_row() - 1, m.get_width() - 1);
        else
            // => at beginning of matrix, set to empty-state
            set_empty();
        return *this;
    }

    using const_matrix_iterator_type::get_row;
    using const_matrix_iterator_type::get_col;
    using const_matrix_iterator_type::set_col;
    using const_matrix_iterator_type::set_pos;
};

template <typename ValueType, unsigned BlockSideLength>
class const_matrix_col_major_iterator : public const_matrix_iterator<ValueType, BlockSideLength>
{
protected:
    typedef const_matrix_iterator<ValueType, BlockSideLength> const_matrix_iterator_type;
    typedef typename const_matrix_iterator_type::matrix_type matrix_type;
    typedef typename const_matrix_iterator_type::elem_size_type elem_size_type;

    template <typename VT, unsigned BSL> friend class matrix;

    using const_matrix_iterator_type::m;
    using const_matrix_iterator_type::set_empty;

    //! \brief create iterator pointing to given row and col
    const_matrix_col_major_iterator(const matrix_type & matrix, const elem_size_type start_row, const elem_size_type start_col)
        : const_matrix_iterator_type(matrix, start_row, start_col) {}

    //! \brief create empty iterator
    const_matrix_col_major_iterator(const matrix_type & matrix)
        : const_matrix_iterator_type(matrix) {}

public:
    //! \brief convert from matrix_iterator
    const_matrix_col_major_iterator(const matrix_iterator<ValueType, BlockSideLength> & matrix_iterator)
        : const_matrix_iterator_type(matrix_iterator) {}

    //! \brief convert from matrix_iterator
    const_matrix_col_major_iterator(const const_matrix_iterator_type & matrix_iterator)
        : const_matrix_iterator_type(matrix_iterator) {}

    // Has to be not empty, else behavior is undefined.
    const_matrix_col_major_iterator & operator ++ ()
    {
        if (get_row() + 1 < m->get_height())
            // => not at the end of col, move down
            set_row(get_row() + 1);
        else if (get_col() + 1 < m->get_width())
            // => at end of col but not last col, move to beginning of next col
            set_pos(0, get_col() + 1);
        else
            // => at end of matrix, set to empty-state
            set_empty();
        return *this;
    }

    // Has to be not empty, else behavior is undefined.
    const_matrix_col_major_iterator & operator -- ()
    {
        if (get_row() - 1 >= 0)
            // => not at the beginning of col, move up
            set_row(get_row() - 1);
        else if (get_col() - 1 >= 0)
            // => at beginning of col but not first col, move to end of previous col
            set_pos(m->get_height() - 1, get_col() - 1);
        else
            // => at beginning of matrix, set to empty-state
            set_empty();
        return *this;
    }

    using const_matrix_iterator_type::get_row;
    using const_matrix_iterator_type::get_col;
    using const_matrix_iterator_type::set_row;
    using const_matrix_iterator_type::set_pos;
};

//! \brief External matrix container.
template <typename ValueType, unsigned BlockSideLength>
class matrix
{
protected:
    typedef matrix<ValueType, BlockSideLength> matrix_type;
    typedef swappable_block_matrix<ValueType, BlockSideLength> swappable_block_matrix_type;
    typedef shared_object_pointer<swappable_block_matrix_type> swappable_block_matrix_pointer_type;
    typedef typename swappable_block_matrix_type::block_scheduler_type block_scheduler_type;
    typedef typename swappable_block_matrix_type::size_type block_size_type;
    typedef typename swappable_block_matrix_type::elem_size_type elem_size_type;
    typedef matrix_operations<ValueType, BlockSideLength> Ops;
    typedef matrix_swappable_block<ValueType, BlockSideLength> swappable_block_type;

public:
    typedef matrix_iterator<ValueType, BlockSideLength> iterator;
    typedef const_matrix_iterator<ValueType, BlockSideLength> const_iterator;
    typedef matrix_row_major_iterator<ValueType, BlockSideLength> row_major_iterator;
    typedef matrix_col_major_iterator<ValueType, BlockSideLength> col_major_iterator;
    typedef const_matrix_row_major_iterator<ValueType, BlockSideLength> const_row_major_iterator;
    typedef const_matrix_col_major_iterator<ValueType, BlockSideLength> const_col_major_iterator;
    typedef column_vector<ValueType> column_vector_type;
    typedef row_vector<ValueType> row_vector_type;

protected:
    template <typename VT, unsigned BSL> friend class matrix_iterator;
    template <typename VT, unsigned BSL> friend class const_matrix_iterator;

    elem_size_type height,
                   width;
    swappable_block_matrix_pointer_type data;

public:
    //! \brief Creates a new matrix of given dimensions. Elements' values are set to zero.
    //! \param height height of the created matrix
    //! \param width width of the created matrix
    matrix(block_scheduler_type & bs, const elem_size_type height, const elem_size_type width)
        : height(height),
          width(width),
          data(new swappable_block_matrix_type
                  (bs, div_ceil(height, BlockSideLength), div_ceil(width, BlockSideLength)))
    {}

    matrix(block_scheduler_type & bs, const column_vector_type & left, const row_vector_type & right)
        : height(left.size()),
          width(right.size()),
          data(new swappable_block_matrix_type
                  (bs, div_ceil(height, BlockSideLength), div_ceil(width, BlockSideLength)))
    { Ops::recursive_matrix_from_vectors(*data, left, right); }

    ~matrix() {}

    const elem_size_type & get_height() const
    { return height; }

    const elem_size_type & get_width() const
    { return width; }

    iterator begin()
    {
        data.unify();
        return iterator(*this, 0, 0);
    }
    const_iterator begin() const
    { return const_iterator(*this, 0, 0); }
    const_iterator cbegin() const
    { return const_iterator(*this, 0, 0); }

    iterator end()
    {
        data.unify();
        return iterator(*this);
    }
    const_iterator end() const
    { return const_iterator(*this); }
    const_iterator cend() const
    { return const_iterator(*this); }

    const_iterator operator () (const elem_size_type row, const elem_size_type col) const
    { return const_iterator(*this, row, col); }

    iterator operator () (const elem_size_type row, const elem_size_type col)
    {
        data.unify();
        return iterator(*this, row, col);
    }

    void transpose()
    {
        data.unify();
        data->transpose();
        std::swap(height, width);
    }

    void set_zero()
    {
        if (data.unique())
            data->set_zero();
        else
            data = new swappable_block_matrix_type
                    (data->bs, div_ceil(height, BlockSideLength), div_ceil(width, BlockSideLength));
    }

    matrix_type operator + (const matrix_type & right) const
    {
        assert(height == right.height && width == right.width);
        matrix_type res(data->bs, height, width);
        Ops::element_op(*res.data, *data, *right.data, typename Ops::addition()); // more efficient than copying this and then adding right
        return res;
    }

    matrix_type operator - (const matrix_type & right) const
    {
        assert(height == right.height && width == right.width);
        matrix_type res(data->bs, height, width);
        Ops::element_op(*res.data, *data, *right.data, typename Ops::subtraction()); // more efficient than copying this and then subtracting right
        return res;
    }

    matrix_type operator * (const matrix_type & right) const
    { return multiply(right); }

    matrix_type operator * (const ValueType scalar) const
    {
        matrix_type res(data->bs, height, width);
        Ops::element_op(*res.data, *data, typename Ops::scalar_multiplication(scalar));
        return res;
    }

    matrix_type & operator += (const matrix_type & right)
    {
        assert(height == right.height && width == right.width);
        data.unify();
        Ops::element_op(*data, *right.data, typename Ops::addition());
        return *this;
    }

    matrix_type & operator -= (const matrix_type & right)
    {
        assert(height == right.height && width == right.width);
        data.unify();
        Ops::element_op(*data, *right.data, typename Ops::subtraction());
        return *this;
    }

    matrix_type & operator *= (const matrix_type & right)
    { return *this = operator * (right); } // implicitly unifies by constructing a result-matrix

    matrix_type & operator *= (const ValueType scalar)
    {
        data.unify();
        Ops::element_op(*data, typename Ops::scalar_multiplication(scalar));
        return *this;
    }

    column_vector_type operator * (const column_vector_type & right) const
    {
        assert(elem_size_type(right.size()) == width);
        column_vector_type res(height);
        res.set_zero();
        Ops::recursive_matrix_col_vector_multiply_and_add(*data, right, res);
        return res;
    }

    row_vector_type multiply_from_left(const row_vector_type & left) const
    {
        assert(elem_size_type(left.size()) == height);
        row_vector_type res(width);
        res.set_zero();
        Ops::recursive_matrix_row_vector_multiply_and_add(left, *data, res);
        return res;
    }

    //! \brief multiply with another matrix
    //! \param algorithm allows to choose the applied algorithm
    //! Available algorithms are: \n
    //!    0: naive_multiply_and_add \n
    //!    1: recursive_multiply_and_add \n
    //!    2: strassen_winograd_multiply_and_add \n
    //!    3: multi_level_strassen_winograd_multiply_and_add \n
    //!    4: strassen_winograd_multiply (optimized pre- and postadditions)
    matrix_type multiply (const matrix_type & right, const int_type multiplication_algorithm = 2, const int_type scheduling_algorithm = 2) const
    {
        assert(width == right.height);
        assert(& data->bs == & right.data->bs);
        matrix_type res(data->bs, height, right.width);

        if (scheduling_algorithm > 0)
        {
            // all offline algos need a simulation-run
            delete data->bs.switch_algorithm_to(new
                    block_scheduler_algorithm_simulation<swappable_block_type>(data->bs));
            switch (multiplication_algorithm)
            {
            case 0:
                Ops::naive_multiply_and_add(*data, *right.data, *res.data);
                break;
            case 1:
                Ops::recursive_multiply_and_add(*data, *right.data, *res.data);
                break;
            case 2:
                Ops::strassen_winograd_multiply_and_add(*data, *right.data, *res.data);
                break;
            case 3:
                Ops::multi_level_strassen_winograd_multiply_and_add(*data, *right.data, *res.data);
                break;
            case 4:
                Ops::strassen_winograd_multiply(*data, *right.data, *res.data);
                break;
            case 5:
                Ops::strassen_winograd_multiply_and_add_interleaved(*data, *right.data, *res.data);
                break;
            default:
                STXXL_ERRMSG("invalid multiplication-algorithm number");
                break;
            }
        }
        switch (scheduling_algorithm)
        {
        case 0:
            delete data->bs.switch_algorithm_to(new
                    block_scheduler_algorithm_online_lru<swappable_block_type>(data->bs));
            break;
        case 1:
            delete data->bs.switch_algorithm_to(new
                    block_scheduler_algorithm_offline_lfd<swappable_block_type>(data->bs));
            break;
        case 2:
            delete data->bs.switch_algorithm_to(new
                    block_scheduler_algorithm_offline_lru_prefetching<swappable_block_type>(data->bs));
            break;
        default:
            STXXL_ERRMSG("invalid scheduling-algorithm number");
        }
        switch (multiplication_algorithm)
        {
        case 0:
            Ops::naive_multiply_and_add(*data, *right.data, *res.data);
            break;
        case 1:
            Ops::recursive_multiply_and_add(*data, *right.data, *res.data);
            break;
        case 2:
            Ops::strassen_winograd_multiply_and_add(*data, *right.data, *res.data);
            break;
        case 3:
            Ops::multi_level_strassen_winograd_multiply_and_add(*data, *right.data, *res.data);
            break;
        case 4:
            Ops::strassen_winograd_multiply(*data, *right.data, *res.data);
            break;
        case 5:
            Ops::strassen_winograd_multiply_and_add_interleaved(*data, *right.data, *res.data);
            break;
        default:
            STXXL_ERRMSG("invalid multiplication-algorithm number");
            break;
        }
        delete data->bs.switch_algorithm_to(new
                    block_scheduler_algorithm_online_lru<swappable_block_type>(data->bs));
        return res;
    }
};

template <typename ValueType, unsigned BlockSideLength, unsigned Level, bool AExists, bool BExists>
struct feedable_strassen_winograd;

template <typename ValueType, unsigned BlockSideLength, unsigned Level>
struct matrix_to_quadtree;

template <typename ValueType, unsigned BlockSideLength>
struct matrix_operations
{
    typedef swappable_block_matrix<ValueType, BlockSideLength> swappable_block_matrix_type;
    typedef typename swappable_block_matrix_type::block_scheduler_type block_scheduler_type;
    typedef typename swappable_block_matrix_type::swappable_block_identifier_type swappable_block_identifier_type;
    typedef typename block_scheduler_type::internal_block_type internal_block_type;
    typedef typename swappable_block_matrix_type::size_type size_type;
    typedef column_vector<ValueType> column_vector_type;
    typedef row_vector<ValueType> row_vector_type;
    typedef typename column_vector_type::size_type vector_size_type;

    // +-+-+-+ addition +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    struct addition
    {
        /* op(c,a,b) means c = a <op> b  e.g. assign sum
         * op(c,a)   means c <op>= a     e.g. add up
         * op(a)     means <op>a         e.g. sign
         *
         * it should hold:
         * op(c,0,0) equivalent c = 0
         * op(c=0,a) equivalent c = op(a)
         * op(c,0)   equivalent {}
         */

        inline ValueType & operator ()  (ValueType & c, const ValueType & a, const ValueType & b) { return c = a + b; }
        inline ValueType & operator ()  (ValueType & c, const ValueType & a)                      { return     c += a; }
        inline ValueType operator ()                   (const ValueType & a)                      { return       +a; }
    };

    struct subtraction
    {
        inline ValueType & operator ()  (ValueType & c, const ValueType & a, const ValueType & b) { return c = a - b; }
        inline ValueType & operator ()  (ValueType & c, const ValueType & a)                      { return     c -= a; }
        inline ValueType operator ()                   (const ValueType & a)                      { return       -a; }
    };

    struct scalar_multiplication
    {
        const ValueType s;

        scalar_multiplication(const ValueType scalar = 1)
            : s(scalar) {}

        inline ValueType & operator ()  (ValueType & c, const ValueType & a, const ValueType &  ) { return c = a * s; }
        inline ValueType & operator ()  (ValueType & c, const ValueType & a)                      { return c = a * s; }
        inline ValueType operator ()                   (const ValueType & a)                      { return     a * s; }
    };

    // element_op<Op>(C,A,B) calculates C = A <Op> B
    template <class Op> static swappable_block_matrix_type &
    element_op(swappable_block_matrix_type & C,
               const swappable_block_matrix_type & A,
               const swappable_block_matrix_type & B, Op op = Op())
    {
        for (size_type row = 0; row < C.get_height(); ++row)
            for (size_type col = 0; col < C.get_width(); ++col)
                element_op_swappable_block(
                        C(row, col), C.is_transposed(), C.bs,
                        A(row, col), A.is_transposed(), A.bs,
                        B(row, col), B.is_transposed(), B.bs, op);
        return C;
    }

    // element_op<Op>(C,A) calculates C <Op>= A
    template <class Op> static swappable_block_matrix_type &
    element_op(swappable_block_matrix_type & C,
               const swappable_block_matrix_type & A, Op op = Op())
    {
        for (size_type row = 0; row < C.get_height(); ++row)
            for (size_type col = 0; col < C.get_width(); ++col)
                element_op_swappable_block(
                        C(row, col), C.is_transposed(), C.bs,
                        A(row, col), A.is_transposed(), A.bs, op);
        return C;
    }

    // element_op<Op>(C) calculates C = <Op>C
    template <class Op> static swappable_block_matrix_type &
    element_op(swappable_block_matrix_type & C, Op op = Op())
    {
        for (size_type row = 0; row < C.get_height(); ++row)
            for (size_type col = 0; col < C.get_width(); ++col)
                element_op_swappable_block(
                        C(row, col), C.is_transposed(), C.bs, op);
        return C;
    }

    // calculates c = a <Op> b
    template <class Op> static void
    element_op_swappable_block(
            const swappable_block_identifier_type c, const bool c_is_transposed, block_scheduler_type & bs_c,
            const swappable_block_identifier_type a, bool a_is_transposed, block_scheduler_type & bs_a,
            const swappable_block_identifier_type b, bool b_is_transposed, block_scheduler_type & bs_b, Op = Op())
    {
        if (! bs_c.is_simulating())
            ++ matrix_operation_statistic::get_instance()->block_addition_calls;
        // check if zero-block (== ! initialized)
        if (! bs_a.is_initialized(a) && ! bs_b.is_initialized(b))
        {
            // => a and b are zero -> set c zero
            bs_c.deinitialize(c);
            if (! bs_c.is_simulating())
                ++ matrix_operation_statistic::get_instance()->block_additions_saved_through_zero;
            return;
        }
        a_is_transposed = a_is_transposed != c_is_transposed;
        b_is_transposed = b_is_transposed != c_is_transposed;
        internal_block_type & ic = bs_c.acquire(c, true);
        if (! bs_a.is_initialized(a))
        {
            // a is zero -> copy b
            internal_block_type & ib = bs_b.acquire(b);
            if (! bs_c.is_simulating())
            {
                if (b_is_transposed)
                    low_level_matrix_op_3<ValueType, BlockSideLength, false, true, Op>(& ic[0], 0, & ib[0]);
                else
                    low_level_matrix_op_3<ValueType, BlockSideLength, false, false, Op>(& ic[0], 0, & ib[0]);
            }
            bs_b.release(b, false);
        }
        else if (! bs_b.is_initialized(b))
        {
            // b is zero -> copy a
            internal_block_type & ia = bs_a.acquire(a);
            if (! bs_c.is_simulating())
            {
                if (a_is_transposed)
                    low_level_matrix_op_3<ValueType, BlockSideLength, true, false, Op>(& ic[0], & ia[0], 0);
                else
                    low_level_matrix_op_3<ValueType, BlockSideLength, false, false, Op>(& ic[0], & ia[0], 0);
            }
            bs_a.release(a, false);
        }
        else
        {
            internal_block_type & ia = bs_a.acquire(a),
                                & ib = bs_b.acquire(b);
            if (! bs_c.is_simulating())
            {
                if (a_is_transposed)
                {
                    if (b_is_transposed)
                        low_level_matrix_op_3<ValueType, BlockSideLength, true, true, Op>(& ic[0], & ia[0], & ib[0]);
                    else
                        low_level_matrix_op_3<ValueType, BlockSideLength, true, false, Op>(& ic[0], & ia[0], & ib[0]);
                }
                else
                {
                    if (b_is_transposed)
                        low_level_matrix_op_3<ValueType, BlockSideLength, false, true, Op>(& ic[0], & ia[0], & ib[0]);
                    else
                        low_level_matrix_op_3<ValueType, BlockSideLength, false, false, Op>(& ic[0], & ia[0], & ib[0]);
                }
            }
            bs_a.release(a, false);
            bs_b.release(b, false);
        }
        bs_c.release(c, true);
    }

    // calculates c <op>= a
    template <class Op> static void
    element_op_swappable_block(
            const swappable_block_identifier_type c, const bool c_is_transposed, block_scheduler_type & bs_c,
            const swappable_block_identifier_type a, const bool a_is_transposed, block_scheduler_type & bs_a, Op = Op())
    {
        if (! bs_c.is_simulating())
            ++ matrix_operation_statistic::get_instance()->block_addition_calls;
        // check if zero-block (== ! initialized)
        if (! bs_a.is_initialized(a))
        {
            // => b is zero => nothing to do
            if (! bs_c.is_simulating())
                ++ matrix_operation_statistic::get_instance()->block_additions_saved_through_zero;
            return;
        }
        const bool c_is_zero = ! bs_c.is_initialized(c);
        // acquire
        internal_block_type & ic = bs_c.acquire(c, c_is_zero),
                            & ia = bs_a.acquire(a);
        // add
        if (! bs_c.is_simulating())
        {
            if (c_is_zero)
                if (c_is_transposed == a_is_transposed)
                    low_level_matrix_op_1<ValueType, BlockSideLength, false, Op>(& ic[0], & ia[0]);
                else
                    low_level_matrix_op_1<ValueType, BlockSideLength, true, Op>(& ic[0], & ia[0]);
            else
                if (c_is_transposed == a_is_transposed)
                    low_level_matrix_op_2<ValueType, BlockSideLength, false, Op>(& ic[0], & ia[0]);
                else
                    low_level_matrix_op_2<ValueType, BlockSideLength, true, Op>(& ic[0], & ia[0]);
        }
        // release
        bs_c.release(c, true);
        bs_a.release(a, false);
    }

    // calculates c = <op>c
    template <class Op> static void
    element_op_swappable_block(
            const swappable_block_identifier_type c, const bool, block_scheduler_type & bs_c, Op = Op())
    {
        if (! bs_c.is_simulating())
            ++ matrix_operation_statistic::get_instance()->block_addition_calls;
        // check if zero-block (== ! initialized)
        if (! bs_c.is_initialized(c))
        {
            // => c is zero => nothing to do
            if (! bs_c.is_simulating())
                ++ matrix_operation_statistic::get_instance()->block_additions_saved_through_zero;
            return;
        }
        // acquire
        internal_block_type & ic = bs_c.acquire(c);
        // add
        if (! bs_c.is_simulating())
            low_level_matrix_op_1<ValueType, BlockSideLength, false, Op>(& ic[0], & ic[0]);
        // release
        bs_c.release(c, true);
    }

    // additions for strassen-winograd

    static void
    strassen_winograd_preaddition_a(swappable_block_matrix_type & a11,
                                    swappable_block_matrix_type & a12,
                                    swappable_block_matrix_type & a21,
                                    swappable_block_matrix_type & a22,
                                    swappable_block_matrix_type & s1,
                                    swappable_block_matrix_type & s2,
                                    swappable_block_matrix_type & s3,
                                    swappable_block_matrix_type & s4)
    {
        for (size_type row = 0; row < a11.get_height(); ++row)
            for (size_type col = 0; col < a11.get_width(); ++col)
            {
                op_swappable_block_nontransposed(s3, a11, subtraction(), a21, row, col);
                op_swappable_block_nontransposed(s1, a21,    addition(), a22, row, col);
                op_swappable_block_nontransposed(s2,  s1, subtraction(), a11, row, col);
                op_swappable_block_nontransposed(s4, a12, subtraction(),  s2, row, col);
            }
    }

    static void
    strassen_winograd_preaddition_b(swappable_block_matrix_type & b11,
                                    swappable_block_matrix_type & b12,
                                    swappable_block_matrix_type & b21,
                                    swappable_block_matrix_type & b22,
                                    swappable_block_matrix_type & t1,
                                    swappable_block_matrix_type & t2,
                                    swappable_block_matrix_type & t3,
                                    swappable_block_matrix_type & t4)
    {
        for (size_type row = 0; row < b11.get_height(); ++row)
            for (size_type col = 0; col < b11.get_width(); ++col)
            {
                op_swappable_block_nontransposed(t3, b22, subtraction(), b12, row, col);
                op_swappable_block_nontransposed(t1, b12, subtraction(), b11, row, col);
                op_swappable_block_nontransposed(t2, b22, subtraction(),  t1, row, col);
                op_swappable_block_nontransposed(t4, b21, subtraction(),  t2, row, col);
            }
    }

    static void
    strassen_winograd_postaddition(swappable_block_matrix_type & c11, // = p2
                                   swappable_block_matrix_type & c12, // = p6
                                   swappable_block_matrix_type & c21, // = p7
                                   swappable_block_matrix_type & c22, // = p4
                                   swappable_block_matrix_type & p1,
                                   swappable_block_matrix_type & p3,
                                   swappable_block_matrix_type & p5)
    {
        for (size_type row = 0; row < c11.get_height(); ++row)
            for (size_type col = 0; col < c11.get_width(); ++col)
            {
                op_swappable_block_nontransposed(c11,     addition(),  p1, row, col); // (u1)
                op_swappable_block_nontransposed( p1,     addition(), c22, row, col); // (u2)
                op_swappable_block_nontransposed( p5,     addition(),  p1, row, col); // (u3)
                op_swappable_block_nontransposed(c21,     addition(),  p5, row, col); // (u4)
                op_swappable_block_nontransposed(c22, p5, addition(),  p3, row, col); // (u5)
                op_swappable_block_nontransposed( p1,     addition(),  p3, row, col); // (u6)
                op_swappable_block_nontransposed(c12,     addition(),  p1, row, col); // (u7)
            }
    }

    // calculates c1 += a; c2 += a
    template <class Op> static void
    element_op_twice_nontransposed(swappable_block_matrix_type & c1,
                     swappable_block_matrix_type & c2,
                     const swappable_block_matrix_type & a, Op op = Op())
    {
        for (size_type row = 0; row < a.get_height(); ++row)
            for (size_type col = 0; col < a.get_width(); ++col)
            {
                element_op_swappable_block(
                        c1(row, col), false, c1.bs,
                        a(row, col),  false, a.bs, op);
                element_op_swappable_block(
                        c2(row, col), false, c2.bs,
                        a(row, col),  false, a.bs, op);
            }
    }

    template <class Op> static void
    op_swappable_block_nontransposed(swappable_block_matrix_type & c,
            swappable_block_matrix_type & a, Op op, swappable_block_matrix_type & b,
            size_type & row, size_type & col)
    {
        element_op_swappable_block(
                        c(row, col), false, c.bs,
                        a(row, col), false, a.bs,
                        b(row, col), false, b.bs, op);
    }

    template <class Op> static void
    op_swappable_block_nontransposed(swappable_block_matrix_type & c, Op op, swappable_block_matrix_type & a,
            size_type & row, size_type & col)
    {
        element_op_swappable_block(
                        c(row, col), false, c.bs,
                        a(row, col), false, a.bs, op);
    }

    // +-+ end addition +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    // +-+-+-+ matrix multiplication +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    /*  n, m and l denote the three dimensions of a matrix multiplication, according to the following ascii-art diagram:
     *
     *                 +--m--+
     *  +----l-----+   |     |   +--m--+
     *  |          |   |     |   |     |
     *  n    A     | • l  B  | = n  C  |
     *  |          |   |     |   |     |
     *  +----------+   |     |   +-----+
     *                 +-----+
     *
     * The index-variables are called i, j, k for dimension
     *                                n, m, l .
     */

    // requires height and width divisible by 2
    struct swappable_block_matrix_quarterer
    {
        swappable_block_matrix_type  upleft,   upright,
                                    downleft, downright,
                & ul, & ur, & dl, & dr;

        swappable_block_matrix_quarterer(const swappable_block_matrix_type & whole)
            : upleft   (whole, whole.get_height()/2, whole.get_width()/2,                    0,                   0),
              upright  (whole, whole.get_height()/2, whole.get_width()/2,                    0, whole.get_width()/2),
              downleft (whole, whole.get_height()/2, whole.get_width()/2, whole.get_height()/2,                   0),
              downright(whole, whole.get_height()/2, whole.get_width()/2, whole.get_height()/2, whole.get_width()/2),
              ul(upleft), ur(upright), dl(downleft), dr(downright)
        { assert(! (whole.get_height() % 2 | whole.get_width() % 2)); }
    };

    struct swappable_block_matrix_padding_quarterer
    {
        swappable_block_matrix_type  upleft,   upright,
                                    downleft, downright,
                & ul, & ur, & dl, & dr;

        swappable_block_matrix_padding_quarterer(const swappable_block_matrix_type & whole)
            : upleft   (whole, div_ceil(whole.get_height(),2), div_ceil(whole.get_width(),2),                              0,                             0),
              upright  (whole, div_ceil(whole.get_height(),2), div_ceil(whole.get_width(),2),                              0, div_ceil(whole.get_width(),2)),
              downleft (whole, div_ceil(whole.get_height(),2), div_ceil(whole.get_width(),2), div_ceil(whole.get_height(),2),                             0),
              downright(whole, div_ceil(whole.get_height(),2), div_ceil(whole.get_width(),2), div_ceil(whole.get_height(),2), div_ceil(whole.get_width(),2)),
              ul(upleft), ur(upright), dl(downleft), dr(downright) {}
    };

    struct swappable_block_matrix_approximative_quarterer
    {
        swappable_block_matrix_type  upleft,   upright,
                                    downleft, downright,
                & ul, & ur, & dl, & dr;

        swappable_block_matrix_approximative_quarterer(const swappable_block_matrix_type & whole)
            : upleft   (whole,                      whole.get_height()/2,                     whole.get_width()/2,                    0,                   0),
              upright  (whole,                      whole.get_height()/2, whole.get_width() - whole.get_width()/2,                    0, whole.get_width()/2),
              downleft (whole, whole.get_height() - whole.get_height()/2,                     whole.get_width()/2, whole.get_height()/2,                   0),
              downright(whole, whole.get_height() - whole.get_height()/2, whole.get_width() - whole.get_width()/2, whole.get_height()/2, whole.get_width()/2),
              ul(upleft), ur(upright), dl(downleft), dr(downright) {}
    };

    //! \brief calculates C = A * B + C
    // requires fitting dimensions
    static swappable_block_matrix_type &
    multi_level_strassen_winograd_multiply_and_add(const swappable_block_matrix_type & A,
                                                   const swappable_block_matrix_type & B,
                                                   swappable_block_matrix_type & C)
    {
        int_type p = log2_ceil(std::min(A.get_width(), std::min(C.get_width(), C.get_height())));

        swappable_block_matrix_type padded_a(A, round_up_to_power_of_two(A.get_height(), p),
                                             round_up_to_power_of_two(A.get_width(), p), 0, 0),
                                    padded_b(B, round_up_to_power_of_two(B.get_height(), p),
                                             round_up_to_power_of_two(B.get_width(), p), 0, 0),
                                    padded_c(C, round_up_to_power_of_two(C.get_height(), p),
                                             round_up_to_power_of_two(C.get_width(), p), 0, 0);
        choose_level_for_feedable_sw(padded_a, padded_b, padded_c);
        return C;
    }

    // input matrices have to be padded
    static void choose_level_for_feedable_sw(const swappable_block_matrix_type & A,
                                             const swappable_block_matrix_type & B,
                                             swappable_block_matrix_type & C)
    {
        switch (log2_ceil(std::min(A.get_width(), std::min(C.get_width(), C.get_height()))))
        {
        default: // todo possibly more Levels
            /*
            use_feedable_sw<4>(A, B, C);
            break;
        case 3:
            use_feedable_sw<3>(A, B, C);
            break;
        case 2:*/
            use_feedable_sw<2>(A, B, C);
            break;
        case 1:
            use_feedable_sw<1>(A, B, C);
            break;
        case 0:
            // base case
            recursive_multiply_and_add(A, B, C);
            break;
        }
    }

    // input matrices have to be padded
    template <unsigned Level>
    static void use_feedable_sw(const swappable_block_matrix_type & A,
                                const swappable_block_matrix_type & B,
                                swappable_block_matrix_type & C)
    {
        feedable_strassen_winograd<ValueType, BlockSideLength, Level, true, true>
                fsw(A, 0, 0, C.bs, C.get_height(), C.get_width(), A.get_width(), B, 0, 0);
        // preadditions for A
        matrix_to_quadtree<ValueType, BlockSideLength, Level>
                mtq_a (A);
        for (size_type block_row = 0; block_row < mtq_a.get_height_in_blocks(); ++block_row)
            for (size_type block_col = 0; block_col < mtq_a.get_width_in_blocks(); ++block_col)
            {
                fsw.begin_feeding_a_block(block_row, block_col,
                        mtq_a.begin_reading_block(block_row, block_col));
                #if STXXL_PARALLEL
                #pragma omp parallel for
                #endif
                for (int_type element_row_in_block = 0; element_row_in_block < int_type(BlockSideLength); ++element_row_in_block)
                    for (int_type element_col_in_block = 0; element_col_in_block < int_type(BlockSideLength); ++element_col_in_block)
                        fsw.feed_a_element(element_row_in_block * BlockSideLength + element_col_in_block,
                                mtq_a.read_element(element_row_in_block * BlockSideLength + element_col_in_block));
                fsw.end_feeding_a_block(block_row, block_col,
                        mtq_a.end_reading_block(block_row, block_col));
            }
        // preadditions for B
        matrix_to_quadtree<ValueType, BlockSideLength, Level>
                mtq_b (B);
        for (size_type block_row = 0; block_row < mtq_b.get_height_in_blocks(); ++block_row)
            for (size_type block_col = 0; block_col < mtq_b.get_width_in_blocks(); ++block_col)
            {
                fsw.begin_feeding_b_block(block_row, block_col,
                        mtq_b.begin_reading_block(block_row, block_col));
                #if STXXL_PARALLEL
                #pragma omp parallel for
                #endif
                for (int_type element_row_in_block = 0; element_row_in_block < int_type(BlockSideLength); ++element_row_in_block)
                    for (int_type element_col_in_block = 0; element_col_in_block < int_type(BlockSideLength); ++element_col_in_block)
                        fsw.feed_b_element(element_row_in_block * BlockSideLength + element_col_in_block,
                                mtq_b.read_element(element_row_in_block * BlockSideLength + element_col_in_block));
                fsw.end_feeding_b_block(block_row, block_col,
                        mtq_b.end_reading_block(block_row, block_col));
            }
        // recursive multiplications
        fsw.multiply();
        // postadditions
        matrix_to_quadtree<ValueType, BlockSideLength, Level>
                mtq_c (C);
        for (size_type block_row = 0; block_row < mtq_c.get_height_in_blocks(); ++block_row)
            for (size_type block_col = 0; block_col < mtq_c.get_width_in_blocks(); ++block_col)
            {
                mtq_c.begin_feeding_block(block_row, block_col,
                        fsw.begin_reading_block(block_row, block_col));
                #if STXXL_PARALLEL
                #pragma omp parallel for
                #endif
                for (int_type element_row_in_block = 0; element_row_in_block < int_type(BlockSideLength); ++element_row_in_block)
                    for (int_type element_col_in_block = 0; element_col_in_block < int_type(BlockSideLength); ++element_col_in_block)
                        mtq_c.feed_and_add_element(element_row_in_block * BlockSideLength + element_col_in_block,
                                fsw.read_element(element_row_in_block * BlockSideLength + element_col_in_block));
                mtq_c.end_feeding_block(block_row, block_col,
                        fsw.end_reading_block(block_row, block_col));
            }

    }

    //! \brief calculates C = A * B
    // assumes fitting dimensions
    static swappable_block_matrix_type &
    strassen_winograd_multiply(const swappable_block_matrix_type & A,
                               const swappable_block_matrix_type & B,
                               swappable_block_matrix_type & C)
    {
        // base case
        if (C.get_height() == 1 || C.get_width() == 1 || A.get_width() == 1)
        {
            C.set_zero();
            return recursive_multiply_and_add(A, B, C);
        }

        // partition matrix
        swappable_block_matrix_padding_quarterer qa(A), qb(B), qc(C);
        // preadditions
        swappable_block_matrix_type s1(C.bs, qa.ul.get_height(), qa.ul.get_width(), qa.ul.is_transposed()),
                                    s2(C.bs, qa.ul.get_height(), qa.ul.get_width(), qa.ul.is_transposed()),
                                    s3(C.bs, qa.ul.get_height(), qa.ul.get_width(), qa.ul.is_transposed()),
                                    s4(C.bs, qa.ul.get_height(), qa.ul.get_width(), qa.ul.is_transposed()),
                                    t1(C.bs, qb.ul.get_height(), qb.ul.get_width(), qb.ul.is_transposed()),
                                    t2(C.bs, qb.ul.get_height(), qb.ul.get_width(), qb.ul.is_transposed()),
                                    t3(C.bs, qb.ul.get_height(), qb.ul.get_width(), qb.ul.is_transposed()),
                                    t4(C.bs, qb.ul.get_height(), qb.ul.get_width(), qb.ul.is_transposed());
        strassen_winograd_preaddition_a(qa.ul, qa.ur, qa.dl, qa.dr, s1, s2, s3, s4);
        strassen_winograd_preaddition_b(qb.ul, qb.ur, qb.dl, qb.dr, t1, t2, t3, t4);
        // recursive multiplications
        swappable_block_matrix_type p1(C.bs, qc.ul.get_height(), qc.ul.get_width(), qc.ul.is_transposed()),
                                 // p2 stored in qc.ul
                                    p3(C.bs, qc.ul.get_height(), qc.ul.get_width(), qc.ul.is_transposed()),
                                 // p4 stored in qc.dr
                                    p5(C.bs, qc.ul.get_height(), qc.ul.get_width(), qc.ul.is_transposed());
                                 // p6 stored in qc.ur
                                 // p7 stored in qc.dl
        strassen_winograd_multiply(qa.ul, qb.ul,    p1);
        strassen_winograd_multiply(qa.ur, qb.dl, qc.ul);
        strassen_winograd_multiply(   s1,    t1,    p3);
        strassen_winograd_multiply(   s2,    t2, qc.dr);
        strassen_winograd_multiply(   s3,    t3,    p5);
        strassen_winograd_multiply(   s4, qb.dr, qc.ur);
        strassen_winograd_multiply(qa.dr,    t4, qc.dl);
        // postadditions
        strassen_winograd_postaddition(qc.ul, qc.ur, qc.dl, qc.dr, p1, p3, p5);
        return C;
    }

    //! \brief calculates C = A * B + C
    // assumes fitting dimensions
    static swappable_block_matrix_type &
    strassen_winograd_multiply_and_add_interleaved(const swappable_block_matrix_type & A,
                                         const swappable_block_matrix_type & B,
                                         swappable_block_matrix_type & C)
    {
        // base case
        if (C.get_height() == 1 || C.get_width() == 1 || A.get_width() == 1)
            return recursive_multiply_and_add(A, B, C);

        // partition matrix
        swappable_block_matrix_padding_quarterer qa(A), qb(B), qc(C);
        // preadditions
        swappable_block_matrix_type s1(C.bs, qa.ul.get_height(), qa.ul.get_width(), qa.ul.is_transposed()),
                                    s2(C.bs, qa.ul.get_height(), qa.ul.get_width(), qa.ul.is_transposed()),
                                    s3(C.bs, qa.ul.get_height(), qa.ul.get_width(), qa.ul.is_transposed()),
                                    s4(C.bs, qa.ul.get_height(), qa.ul.get_width(), qa.ul.is_transposed()),
                                    t1(C.bs, qb.ul.get_height(), qb.ul.get_width(), qb.ul.is_transposed()),
                                    t2(C.bs, qb.ul.get_height(), qb.ul.get_width(), qb.ul.is_transposed()),
                                    t3(C.bs, qb.ul.get_height(), qb.ul.get_width(), qb.ul.is_transposed()),
                                    t4(C.bs, qb.ul.get_height(), qb.ul.get_width(), qb.ul.is_transposed());
        strassen_winograd_preaddition_a(qa.ul, qa.ur, qa.dl, qa.dr, s1, s2, s3, s4);
        strassen_winograd_preaddition_b(qb.ul, qb.ur, qb.dl, qb.dr, t1, t2, t3, t4);
        // recursive multiplications and postadditions
        swappable_block_matrix_type px(C.bs, qc.ul.get_height(), qc.ul.get_width(), qc.ul.is_transposed());
        strassen_winograd_multiply_and_add_interleaved(qa.ur, qb.dl, qc.ul); // p2
        strassen_winograd_multiply_and_add_interleaved(qa.ul, qb.ul, px); // p1
        element_op<addition>(qc.ul, px);
        strassen_winograd_multiply_and_add_interleaved(s2, t2, px); // p4
        s2.set_zero();
        t2.set_zero();
        element_op<addition>(qc.ur, px);
        strassen_winograd_multiply_and_add_interleaved(s3, t3, px); // p5
        s3.set_zero();
        t3.set_zero();
        element_op_twice_nontransposed<addition>(qc.dl, qc.dr, px);
        px.set_zero();
        strassen_winograd_multiply_and_add_interleaved(qa.dr, t4, qc.dl); // p7
        t4.set_zero();
        strassen_winograd_multiply_and_add_interleaved(s1, t1, px); // p3
        s1.set_zero();
        t1.set_zero();
        element_op_twice_nontransposed<addition>(qc.dr, qc.ur, px);
        px.set_zero();
        strassen_winograd_multiply_and_add_interleaved(s4, qb.dr, qc.ur); // p6
        return C;
    }

    //! \brief calculates C = A * B + C
    // assumes fitting dimensions
    static swappable_block_matrix_type &
    strassen_winograd_multiply_and_add(const swappable_block_matrix_type & A,
                                       const swappable_block_matrix_type & B,
                                       swappable_block_matrix_type & C)
    {
        // base case
        if (C.get_height() == 1 || C.get_width() == 1 || A.get_width() == 1)
            return recursive_multiply_and_add(A, B, C);

        // partition matrix
        swappable_block_matrix_padding_quarterer qa(A), qb(B), qc(C);
        // preadditions
        swappable_block_matrix_type s1(C.bs, qa.ul.get_height(), qa.ul.get_width()),
                                    s2(C.bs, qa.ul.get_height(), qa.ul.get_width()),
                                    s3(C.bs, qa.ul.get_height(), qa.ul.get_width()),
                                    s4(C.bs, qa.ul.get_height(), qa.ul.get_width()),
                                    t1(C.bs, qb.ul.get_height(), qb.ul.get_width()),
                                    t2(C.bs, qb.ul.get_height(), qb.ul.get_width()),
                                    t3(C.bs, qb.ul.get_height(), qb.ul.get_width()),
                                    t4(C.bs, qb.ul.get_height(), qb.ul.get_width());
        element_op<subtraction>(s3, qa.ul, qa.dl);
        element_op<addition>(s1, qa.dl, qa.dr);
        element_op<subtraction>(s2, s1, qa.ul);
        element_op<subtraction>(s4, qa.ur, s2);
        element_op<subtraction>(t3, qb.dr, qb.ur);
        element_op<subtraction>(t1, qb.ur, qb.ul);
        element_op<subtraction>(t2, qb.dr, t1);
        element_op<subtraction>(t4, qb.dl, t2);
        // recursive multiplications and postadditions
        swappable_block_matrix_type px(C.bs, qc.ul.get_height(), qc.ul.get_width());
        strassen_winograd_multiply_and_add(qa.ur, qb.dl, qc.ul); // p2
        strassen_winograd_multiply_and_add(qa.ul, qb.ul, px); // p1
        element_op<addition>(qc.ul, px);
        strassen_winograd_multiply_and_add(s2, t2, px); // p4
        element_op<addition>(qc.ur, px);
        strassen_winograd_multiply_and_add(s3, t3, px); // p5
        element_op<addition>(qc.dl, px);
        element_op<addition>(qc.dr, px);
        px.set_zero();
        strassen_winograd_multiply_and_add(qa.dr, t4, qc.dl); // p7
        strassen_winograd_multiply_and_add(s1, t1, px); // p3
        element_op<addition>(qc.dr, px);
        element_op<addition>(qc.ur, px);
        strassen_winograd_multiply_and_add(s4, qb.dr, qc.ur); // p6
        return C;
    }

    //! \brief calculates C = A * B + C
    // assumes fitting dimensions
    static swappable_block_matrix_type &
    recursive_multiply_and_add(const swappable_block_matrix_type & A,
                               const swappable_block_matrix_type & B,
                               swappable_block_matrix_type & C)
    {
        // catch empty intervals
        if (C.get_height() * C.get_width() * A.get_width() == 0)
            return C;
        // base case
        if ((C.get_height() == 1) + (C.get_width() == 1) + (A.get_width() == 1) >= 2)
            return naive_multiply_and_add(A, B, C);

        // partition matrix
        swappable_block_matrix_approximative_quarterer qa(A), qb(B), qc(C);
        // recursive multiplication
        // The order of recursive calls is optimized to enhance locality. C has priority because it has to be read and written.
        recursive_multiply_and_add(qa.ul, qb.ul, qc.ul);
        recursive_multiply_and_add(qa.ur, qb.dl, qc.ul);
        recursive_multiply_and_add(qa.ur, qb.dr, qc.ur);
        recursive_multiply_and_add(qa.ul, qb.ur, qc.ur);
        recursive_multiply_and_add(qa.dl, qb.ur, qc.dr);
        recursive_multiply_and_add(qa.dr, qb.dr, qc.dr);
        recursive_multiply_and_add(qa.dr, qb.dl, qc.dl);
        recursive_multiply_and_add(qa.dl, qb.ul, qc.dl);

        return C;
    }

    //! \brief calculates C = A * B + C
    // requires fitting dimensions
    static swappable_block_matrix_type &
    naive_multiply_and_add(const swappable_block_matrix_type & A,
                           const swappable_block_matrix_type & B,
                           swappable_block_matrix_type & C)
    {
        const size_type & n = C.get_height(),
                        & m = C.get_width(),
                        & l = A.get_width();
        for (size_type i = 0; i < n; ++i)
            for (size_type j = 0; j < m; ++j)
                for (size_type k = 0; k < l; ++k)
                    multiply_and_add_swappable_block(A(i,k), A.is_transposed(), A.bs,
                                                     B(k,j), B.is_transposed(), B.bs,
                                                     C(i,j), C.is_transposed(), C.bs);
        return C;
    }

    static void multiply_and_add_swappable_block(
            const swappable_block_identifier_type a, const bool a_is_transposed, block_scheduler_type & bs_a,
            const swappable_block_identifier_type b, const bool b_is_transposed, block_scheduler_type & bs_b,
            const swappable_block_identifier_type c, const bool c_is_transposed, block_scheduler_type & bs_c)
    {
        if (! bs_c.is_simulating())
            ++ matrix_operation_statistic::get_instance()->block_multiplication_calls;
        // check if zero-block (== ! initialized)
        if (! bs_a.is_initialized(a) || ! bs_b.is_initialized(b))
        {
            // => one factor is zero => product is zero
            if (! bs_c.is_simulating())
                ++ matrix_operation_statistic::get_instance()->block_multiplications_saved_through_zero;
            return;
        }
        // acquire
        ValueType * ap = bs_a.acquire(a).begin(),
                  * bp = bs_b.acquire(b).begin(),
                  * cp = bs_c.acquire(c).begin();
        // multiply
        if (! bs_c.is_simulating())
            low_level_matrix_multiply_and_add<ValueType, BlockSideLength>
                    (ap, a_is_transposed, bp, b_is_transposed, cp, c_is_transposed);
        // release
        bs_a.release(a, false);
        bs_b.release(b, false);
        bs_c.release(c, true);
    }

    // +-+ end matrix multiplication +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    // +-+-+-+ matrix-vector multiplication +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    //! \brief calculates z = A * x
    static column_vector_type &
    recursive_matrix_col_vector_multiply_and_add(const swappable_block_matrix_type & A,
                                         const column_vector_type & x, column_vector_type & z,
                                         const vector_size_type offset_x = 0, const vector_size_type offset_z = 0)
    {
        // catch empty intervals
        if (A.get_height() * A.get_width() == 0)
            return z;
        // base case
        if (A.get_height() == 1 || A.get_width() == 1)
            return naive_matrix_col_vector_multiply_and_add(A, x, z, offset_x, offset_z);

        // partition matrix
        swappable_block_matrix_approximative_quarterer qa(A);
        // recursive multiplication
        // The order of recursive calls is optimized to enhance locality.
        recursive_matrix_col_vector_multiply_and_add(qa.ul, x, z, offset_x,                     offset_z                     );
        recursive_matrix_col_vector_multiply_and_add(qa.ur, x, z, offset_x + qa.ul.get_width(), offset_z                     );
        recursive_matrix_col_vector_multiply_and_add(qa.dr, x, z, offset_x + qa.ul.get_width(), offset_z + qa.ul.get_height());
        recursive_matrix_col_vector_multiply_and_add(qa.dl, x, z, offset_x,                     offset_z + qa.ul.get_height());

        return z;
    }

    static column_vector_type &
    naive_matrix_col_vector_multiply_and_add(const swappable_block_matrix_type & A,
                                     const column_vector_type & x, column_vector_type & z,
                                     const vector_size_type offset_x = 0, const vector_size_type offset_z = 0)
    {
        for (size_type row = 0; row < A.get_height(); ++row)
            for (size_type col = 0; col < A.get_width(); ++col)
                matrix_col_vector_multiply_and_add_swappable_block(A(row, col), A.is_transposed(), A.bs,
                        x, z, (offset_x + col) * BlockSideLength, (offset_z + row) * BlockSideLength);
        return z;
    }

    static void matrix_col_vector_multiply_and_add_swappable_block(
            const swappable_block_identifier_type a, const bool a_is_transposed, block_scheduler_type & bs_a,
            const column_vector_type & x, column_vector_type & z,
            const vector_size_type offset_x = 0, const vector_size_type offset_z = 0)
    {
        // check if zero-block (== ! initialized)
        if (! bs_a.is_initialized(a))
        {
            // => matrix is zero => product is zero
            return;
        }
        // acquire
        internal_block_type & ia = bs_a.acquire(a);
        // multiply
        if (! bs_a.is_simulating())
        {
            int_type row_limit = std::min(BlockSideLength, unsigned(z.size() - offset_z)),
                     col_limit = std::min(BlockSideLength, unsigned(x.size() - offset_x));
            if (a_is_transposed)
                for (int_type col = 0; col < col_limit; ++col)
                    for (int_type row = 0; row < row_limit; ++row)
                        z[offset_z + row] += x[offset_x + col] * ia[row + col * BlockSideLength];
            else
                for (int_type row = 0; row < row_limit; ++row)
                    for (int_type col = 0; col < col_limit; ++col)
                        z[offset_z + row] += x[offset_x + col] * ia[row * BlockSideLength + col];
        }
        // release
        bs_a.release(a, false);
    }

    //! \brief calculates z = y * A
    static row_vector_type &
    recursive_matrix_row_vector_multiply_and_add(const row_vector_type & y,
            const swappable_block_matrix_type & A, row_vector_type & z,
            const vector_size_type offset_y = 0, const vector_size_type offset_z = 0)
    {
        // catch empty intervals
        if (A.get_height() * A.get_width() == 0)
            return z;
        // base case
        if (A.get_height() == 1 || A.get_width() == 1)
            return naive_matrix_row_vector_multiply_and_add(y, A, z, offset_y, offset_z);

        // partition matrix
        swappable_block_matrix_approximative_quarterer qa(A);
        // recursive multiplication
        // The order of recursive calls is optimized to enhance locality.
        recursive_matrix_row_vector_multiply_and_add(y, qa.ul, z, offset_y,                      offset_z                    );
        recursive_matrix_row_vector_multiply_and_add(y, qa.dl, z, offset_y + qa.ul.get_height(), offset_z                    );
        recursive_matrix_row_vector_multiply_and_add(y, qa.dr, z, offset_y + qa.ul.get_height(), offset_z + qa.ul.get_width());
        recursive_matrix_row_vector_multiply_and_add(y, qa.ur, z, offset_y,                      offset_z + qa.ul.get_width());

        return z;
    }

    static row_vector_type &
    naive_matrix_row_vector_multiply_and_add(const row_vector_type & y, const swappable_block_matrix_type & A,
                                     row_vector_type & z,
                                     const vector_size_type offset_y = 0, const vector_size_type offset_z = 0)
    {
        for (size_type row = 0; row < A.get_height(); ++row)
            for (size_type col = 0; col < A.get_width(); ++col)
                matrix_row_vector_multiply_and_add_swappable_block(y, A(row, col), A.is_transposed(), A.bs,
                        z, (offset_y + row) * BlockSideLength, (offset_z + col) * BlockSideLength);
        return z;
    }

    static void matrix_row_vector_multiply_and_add_swappable_block(const row_vector_type & y,
            const swappable_block_identifier_type a, const bool a_is_transposed, block_scheduler_type & bs_a,
            row_vector_type & z,
            const vector_size_type offset_y = 0, const vector_size_type offset_z = 0)
    {
        // check if zero-block (== ! initialized)
        if (! bs_a.is_initialized(a))
        {
            // => matrix is zero => product is zero
            return;
        }
        // acquire
        internal_block_type & ia = bs_a.acquire(a);
        // multiply
        if (! bs_a.is_simulating())
        {
            int_type row_limit = std::min(BlockSideLength, unsigned(y.size() - offset_y)),
                     col_limit = std::min(BlockSideLength, unsigned(z.size() - offset_z));
            if (a_is_transposed)
                for (int_type col = 0; col < col_limit; ++col)
                    for (int_type row = 0; row < row_limit; ++row)
                        z[offset_z + col] += ia[row + col * BlockSideLength] * y[offset_y + row];
            else
                for (int_type row = 0; row < row_limit; ++row)
                    for (int_type col = 0; col < col_limit; ++col)
                        z[offset_z + col] += ia[row * BlockSideLength + col] * y[offset_y + row];
        }
        // release
        bs_a.release(a, false);
    }

    // +-+ end matrix-vector multiplication +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    // +-+-+-+ vector-vector multiplication +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    static void recursive_matrix_from_vectors(swappable_block_matrix_type A, const column_vector_type & l,
            const row_vector_type & r, vector_size_type offset_l = 0, vector_size_type offset_r = 0)
    {
        // catch empty intervals
        if (A.get_height() * A.get_width() == 0)
            return;
        // base case
        if (A.get_height() == 1 || A.get_width() == 1)
        {
            naive_matrix_from_vectors(A, l, r, offset_l, offset_r);
            return;
        }

        // partition matrix
        swappable_block_matrix_approximative_quarterer qa(A);
        // recursive creation
        // The order of recursive calls is optimized to enhance locality.
        recursive_matrix_from_vectors(qa.ul, l, r, offset_l,                      offset_r                    );
        recursive_matrix_from_vectors(qa.ur, l, r, offset_l,                      offset_r + qa.ul.get_width());
        recursive_matrix_from_vectors(qa.dr, l, r, offset_l + qa.ul.get_height(), offset_r + qa.ul.get_width());
        recursive_matrix_from_vectors(qa.dl, l, r, offset_l + qa.ul.get_height(), offset_r                    );
    }

    static void naive_matrix_from_vectors(swappable_block_matrix_type A, const column_vector_type & l,
            const row_vector_type & r, vector_size_type offset_l = 0, vector_size_type offset_r = 0)
    {
        for (size_type row = 0; row < A.get_height(); ++row)
            for (size_type col = 0; col < A.get_width(); ++col)
                matrix_from_vectors_swappable_block(A(row, col), A.is_transposed(), A.bs,
                        l, r, (offset_l + row) * BlockSideLength, (offset_r + col) * BlockSideLength);
    }

    static void matrix_from_vectors_swappable_block(swappable_block_identifier_type a,
            const bool a_is_transposed, block_scheduler_type & bs_a,
            const column_vector_type & l, const row_vector_type & r,
            vector_size_type offset_l, vector_size_type offset_r)
    {
        // acquire
        internal_block_type & ia = bs_a.acquire(a, true);
        // multiply
        if (! bs_a.is_simulating())
        {
            int_type row_limit = std::min(BlockSideLength, unsigned(l.size() - offset_l)),
                     col_limit = std::min(BlockSideLength, unsigned(r.size() - offset_r));
            if (a_is_transposed)
                for (int_type col = 0; col < col_limit; ++col)
                    for (int_type row = 0; row < row_limit; ++row)
                        ia[row + col * BlockSideLength] = l[row + offset_l] * r[col + offset_r];
            else
                for (int_type row = 0; row < row_limit; ++row)
                    for (int_type col = 0; col < col_limit; ++col)
                        ia[row * BlockSideLength + col] = l[row + offset_l] * r[col + offset_r];
        }
        // release
        bs_a.release(a, true);
    }

    // +-+ end vector-vector multiplication +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
};

template <typename ValueType, unsigned Level>
struct static_quadtree
{
    typedef static_quadtree<ValueType, Level - 1> smaller_static_quadtree;

    smaller_static_quadtree ul, ur, dl, dr;

    static_quadtree(smaller_static_quadtree ul, smaller_static_quadtree ur,
            smaller_static_quadtree dl, smaller_static_quadtree dr)
        : ul(ul), ur(ur), dl(dl), dr(dr) {}

    static_quadtree() {}

    static_quadtree & operator &= (const static_quadtree & right)
    {
        ul &= right.ul; ur &= right.ur; dl &= right.dl; dr &= right.dr;
        return *this;
    }

    static_quadtree & operator += (const static_quadtree & right)
    {
        ul += right.ul; ur += right.ur; dl += right.dl; dr += right.dr;
        return *this;
    }

    static_quadtree operator & (const static_quadtree & right) const
    { return static_quadtree(ul & right.ul, ur & right.ur, dl & right.dl, dr & right.dr); }

    static_quadtree operator + (const static_quadtree & right) const
    { return static_quadtree(ul + right.ul, ur + right.ur, dl + right.dl, dr + right.dr); }

    static_quadtree operator - (const static_quadtree & right) const
    { return static_quadtree(ul - right.ul, ur - right.ur, dl - right.dl, dr - right.dr); }
};

template <typename ValueType>
struct static_quadtree<ValueType, 0>
{
    ValueType val;

    static_quadtree(const ValueType & v)
        : val(v) {}

    static_quadtree() {}

    operator const ValueType & () const
    { return val; }

    operator ValueType & ()
    { return val; }

    static_quadtree & operator &= (const static_quadtree & right)
    {
        val &= right.val;
        return *this;
    }

    static_quadtree & operator += (const static_quadtree & right)
    {
        val += right.val;
        return *this;
    }

    static_quadtree operator ! () const
    { return static_quadtree(! val); }

    static_quadtree operator & (const static_quadtree & right) const
    { return val & right.val; }

    static_quadtree operator + (const static_quadtree & right) const
    { return val + right.val; }

    static_quadtree operator - (const static_quadtree & right) const
    { return val - right.val; }
};

template <typename ValueType, unsigned BlockSideLength, bool AExists, bool BExists>
struct feedable_strassen_winograd<ValueType, BlockSideLength, 0, AExists, BExists>
{
    typedef static_quadtree<bool, 0> zbt;     // true <=> is a zero-block
    typedef static_quadtree<ValueType, 0> vt;

    typedef swappable_block_matrix<ValueType, BlockSideLength> swappable_block_matrix_type;
    typedef typename swappable_block_matrix_type::block_scheduler_type block_scheduler_type;
    typedef typename block_scheduler_type::internal_block_type internal_block_type;
    typedef typename swappable_block_matrix_type::size_type size_type;

    swappable_block_matrix_type a, b, c;
    const size_type n, m, l;
    internal_block_type * iblock;

    feedable_strassen_winograd(
            const swappable_block_matrix_type & existing_a, const size_type a_from_row, const size_type a_from_col,
            block_scheduler_type & bs_c, const size_type n, const size_type m, const size_type l,
            const swappable_block_matrix_type & existing_b, const size_type b_from_row, const size_type b_from_col)
        : a(existing_a, n, l, a_from_row, a_from_col),
          b(existing_b, n, l, b_from_row, b_from_col),
          c(bs_c, n, m),
          n(n), m(m), l(l),
          iblock(0) {}

    feedable_strassen_winograd(
            const swappable_block_matrix_type & existing_a, const size_type a_from_row, const size_type a_from_col,
            block_scheduler_type & bs_c, const size_type n, const size_type m, const size_type l)
        : a(existing_a, n, l, a_from_row, a_from_col),
          b(bs_c, n, l),
          c(bs_c, n, m),
          n(n), m(m), l(l),
          iblock(0) {}

    feedable_strassen_winograd(
            block_scheduler_type & bs_c, const size_type n, const size_type m, const size_type l,
            const swappable_block_matrix_type & existing_b, const size_type b_from_row, const size_type b_from_col)
        : a(bs_c, n, l),
          b(existing_b, n, l, b_from_row, b_from_col),
          c(bs_c, n, m),
          n(n), m(m), l(l),
          iblock(0) {}

    feedable_strassen_winograd(
            block_scheduler_type & bs_c, const size_type n, const size_type m, const size_type l)
        : a(bs_c, n, l),
          b(bs_c, n, l),
          c(bs_c, n, m),
          n(n), m(m), l(l),
          iblock(0) {}

    void begin_feeding_a_block(const size_type & block_row, const size_type & block_col, const zbt)
    {
        if (! AExists)
            iblock = & a.bs.acquire(a(block_row, block_col), true);
    }

    void feed_a_element(const int_type element_num, const vt v)
    {
        if (! AExists)
            (*iblock)[element_num] = v;
    }

    void end_feeding_a_block(const size_type & block_row, const size_type & block_col, const zbt zb)
    {
        if (! AExists)
        {
            a.bs.release(a(block_row, block_col), ! zb);
            iblock = 0;
        }
    }

    void begin_feeding_b_block(const size_type & block_row, const size_type & block_col, const zbt)
    {
        if (! BExists)
            iblock = & b.bs.acquire(b(block_row, block_col), true);
    }

    void feed_b_element(const int_type element_num, const vt v)
    {
        if (! BExists)
            (*iblock)[element_num] = v;
    }

    void end_feeding_b_block(const size_type & block_row, const size_type & block_col, const zbt zb)
    {
        if (! BExists)
        {
            b.bs.release(b(block_row, block_col), ! zb);
            iblock = 0;
        }
    }

    void multiply()
    { matrix_operations<ValueType, BlockSideLength>::choose_level_for_feedable_sw(a, b, c); }

    zbt begin_reading_block(const size_type & block_row, const size_type & block_col)
    {
        bool zb = ! c.bs.is_initialized(c(block_row, block_col));
        iblock = & c.bs.acquire(c(block_row, block_col));
        return zb;
    }

    vt read_element(const int_type element_num)
    { return (*iblock)[element_num]; }

    zbt end_reading_block(const size_type & block_row, const size_type & block_col)
    {
        c.bs.release(c(block_row, block_col), false);
        iblock = 0;
        return ! c.bs.is_initialized(c(block_row, block_col));
    }
};

template <typename ValueType, unsigned BlockSideLength, unsigned Level, bool AExists, bool BExists>
struct feedable_strassen_winograd
{
    typedef static_quadtree<bool, Level> zbt;     // true <=> is a zero-block
    typedef static_quadtree<ValueType, Level> vt;

    typedef feedable_strassen_winograd<ValueType, BlockSideLength, Level - 1, AExists, BExists> smaller_feedable_strassen_winograd_ab;
    typedef feedable_strassen_winograd<ValueType, BlockSideLength, Level - 1, AExists, false> smaller_feedable_strassen_winograd_a;
    typedef feedable_strassen_winograd<ValueType, BlockSideLength, Level - 1, false, BExists> smaller_feedable_strassen_winograd_b;
    typedef feedable_strassen_winograd<ValueType, BlockSideLength, Level - 1, false, false> smaller_feedable_strassen_winograd_n;

    typedef swappable_block_matrix<ValueType, BlockSideLength> swappable_block_matrix_type;
    typedef typename swappable_block_matrix_type::block_scheduler_type block_scheduler_type;
    typedef typename block_scheduler_type::internal_block_type internal_block_type;
    typedef typename swappable_block_matrix_type::size_type size_type;

    const size_type n, m, l;
    smaller_feedable_strassen_winograd_ab p1, p2;
    smaller_feedable_strassen_winograd_n  p3, p4, p5;
    smaller_feedable_strassen_winograd_b  p6;
    smaller_feedable_strassen_winograd_a  p7;

    feedable_strassen_winograd(
            const swappable_block_matrix_type & existing_a, const size_type a_from_row, const size_type a_from_col,
            block_scheduler_type & bs_c, const size_type n, const size_type m, const size_type l,
            const swappable_block_matrix_type & existing_b, const size_type b_from_row, const size_type b_from_col)
        : n(n), m(m), l(l),
          p1(existing_a, a_from_row,       a_from_col,       bs_c, n/2, m/2, l/2, existing_b, b_from_row,       b_from_col),
          p2(existing_a, a_from_row,       a_from_col + l/2, bs_c, n/2, m/2, l/2, existing_b, b_from_row + l/2, b_from_col),
          p3(                                                bs_c, n/2, m/2, l/2),
          p4(                                                bs_c, n/2, m/2, l/2),
          p5(                                                bs_c, n/2, m/2, l/2),
          p6(                                                bs_c, n/2, m/2, l/2, existing_b, b_from_row + l/2, b_from_col + m/2),
          p7(existing_a, a_from_row + n/2, a_from_col + l/2, bs_c, n/2, m/2, l/2) {}

    feedable_strassen_winograd(
            const swappable_block_matrix_type & existing_a, const size_type a_from_row, const size_type a_from_col,
            block_scheduler_type & bs_c, const size_type n, const size_type m, const size_type l)
        : n(n), m(m), l(l),
          p1(existing_a, a_from_row,       a_from_col,       bs_c, n/2, m/2, l/2),
          p2(existing_a, a_from_row,       a_from_col + l/2, bs_c, n/2, m/2, l/2),
          p3(                                                bs_c, n/2, m/2, l/2),
          p4(                                                bs_c, n/2, m/2, l/2),
          p5(                                                bs_c, n/2, m/2, l/2),
          p6(                                                bs_c, n/2, m/2, l/2),
          p7(existing_a, a_from_row + n/2, a_from_col + l/2, bs_c, n/2, m/2, l/2) {}

    feedable_strassen_winograd(
            block_scheduler_type & bs_c, const size_type n, const size_type m, const size_type l,
            const swappable_block_matrix_type & existing_b, const size_type b_from_row, const size_type b_from_col)
        : n(n), m(m), l(l),
          p1(bs_c, n/2, m/2, l/2, existing_b, b_from_row,       b_from_col),
          p2(bs_c, n/2, m/2, l/2, existing_b, b_from_row + l/2, b_from_col),
          p3(bs_c, n/2, m/2, l/2),
          p4(bs_c, n/2, m/2, l/2),
          p5(bs_c, n/2, m/2, l/2),
          p6(bs_c, n/2, m/2, l/2, existing_b, b_from_row + l/2, b_from_col + m/2),
          p7(bs_c, n/2, m/2, l/2) {}

    feedable_strassen_winograd(
            block_scheduler_type & bs_c, const size_type n, const size_type m, const size_type l)
        : n(n), m(m), l(l),
          p1(bs_c, n/2, m/2, l/2),
          p2(bs_c, n/2, m/2, l/2),
          p3(bs_c, n/2, m/2, l/2),
          p4(bs_c, n/2, m/2, l/2),
          p5(bs_c, n/2, m/2, l/2),
          p6(bs_c, n/2, m/2, l/2),
          p7(bs_c, n/2, m/2, l/2) {}

    void begin_feeding_a_block(const size_type & block_row, const size_type & block_col, const zbt zb)
    {
        typename zbt::smaller_static_quadtree s1 = zb.dl & zb.dr,
                                              s2 = s1    & zb.ul,
                                              s3 = zb.ul & zb.dl,
                                              s4 = zb.ur & s2;
        p1.begin_feeding_a_block(block_row, block_col, zb.ul);
        p2.begin_feeding_a_block(block_row, block_col, zb.ur);
        p3.begin_feeding_a_block(block_row, block_col, s1);
        p4.begin_feeding_a_block(block_row, block_col, s2);
        p5.begin_feeding_a_block(block_row, block_col, s3);
        p6.begin_feeding_a_block(block_row, block_col, s4);
        p7.begin_feeding_a_block(block_row, block_col, zb.dr);
    }

    void feed_a_element(const int_type element_num, const vt v)
    {
        typename vt::smaller_static_quadtree s1 = v.dl + v.dr,
                                             s2 = s1   - v.ul,
                                             s3 = v.ul - v.dl,
                                             s4 = v.ur - s2;
        p1.feed_a_element(element_num, v.ul);
        p2.feed_a_element(element_num, v.ur);
        p3.feed_a_element(element_num, s1);
        p4.feed_a_element(element_num, s2);
        p5.feed_a_element(element_num, s3);
        p6.feed_a_element(element_num, s4);
        p7.feed_a_element(element_num, v.dr);
    }

    void end_feeding_a_block(const size_type & block_row, const size_type & block_col, const zbt zb)
    {
        typename zbt::smaller_static_quadtree s1 = zb.dl & zb.dr,
                                              s2 = s1    & zb.ul,
                                              s3 = zb.ul & zb.dl,
                                              s4 = zb.ur & s2;
        p1.end_feeding_a_block(block_row, block_col, zb.ul);
        p2.end_feeding_a_block(block_row, block_col, zb.ur);
        p3.end_feeding_a_block(block_row, block_col, s1);
        p4.end_feeding_a_block(block_row, block_col, s2);
        p5.end_feeding_a_block(block_row, block_col, s3);
        p6.end_feeding_a_block(block_row, block_col, s4);
        p7.end_feeding_a_block(block_row, block_col, zb.dr);
    }

    void begin_feeding_b_block(const size_type & block_row, const size_type & block_col, const zbt zb)
    {
        typename zbt::smaller_static_quadtree t1 = zb.ur & zb.ul,
                                              t2 = zb.dr & t1,
                                              t3 = zb.dr & zb.ur,
                                              t4 = zb.dl & t2;
        p1.begin_feeding_b_block(block_row, block_col, zb.ul);
        p2.begin_feeding_b_block(block_row, block_col, zb.dl);
        p3.begin_feeding_b_block(block_row, block_col, t1);
        p4.begin_feeding_b_block(block_row, block_col, t2);
        p5.begin_feeding_b_block(block_row, block_col, t3);
        p6.begin_feeding_b_block(block_row, block_col, zb.dr);
        p7.begin_feeding_b_block(block_row, block_col, t4);
    }

    void feed_b_element(const int_type element_num, const vt v)
    {
        typename vt::smaller_static_quadtree t1 = v.ur - v.ul,
                                             t2 = v.dr - t1,
                                             t3 = v.dr - v.ur,
                                             t4 = v.dl - t2;
        p1.feed_b_element(element_num, v.ul);
        p2.feed_b_element(element_num, v.dl);
        p3.feed_b_element(element_num, t1);
        p4.feed_b_element(element_num, t2);
        p5.feed_b_element(element_num, t3);
        p6.feed_b_element(element_num, v.dr);
        p7.feed_b_element(element_num, t4);
    }

    void end_feeding_b_block(const size_type & block_row, const size_type & block_col, const zbt zb)
    {
        typename zbt::smaller_static_quadtree t1 = zb.ur & zb.ul,
                                              t2 = zb.dr & t1,
                                              t3 = zb.dr & zb.ur,
                                              t4 = zb.dl & t2;
        p1.end_feeding_b_block(block_row, block_col, zb.ul);
        p2.end_feeding_b_block(block_row, block_col, zb.dl);
        p3.end_feeding_b_block(block_row, block_col, t1);
        p4.end_feeding_b_block(block_row, block_col, t2);
        p5.end_feeding_b_block(block_row, block_col, t3);
        p6.end_feeding_b_block(block_row, block_col, zb.dr);
        p7.end_feeding_b_block(block_row, block_col, t4);
    }

    void multiply()
    {
        p1.multiply();
        p2.multiply();
        p3.multiply();
        p4.multiply();
        p5.multiply();
        p6.multiply();
        p7.multiply();
    }

    zbt begin_reading_block(const size_type & block_row, const size_type & block_col)
    {
        zbt r;
        r.ur = r.ul = p1.begin_reading_block(block_row, block_col);
        r.ul &= p2.begin_reading_block(block_row, block_col);
        r.ur &= p4.begin_reading_block(block_row, block_col);
        r.dr = r.dl = p5.begin_reading_block(block_row, block_col);
        r.dl &= r.ur;
        r.dl &= p7.begin_reading_block(block_row, block_col);
        r.ur &= p3.begin_reading_block(block_row, block_col);
        r.dr &= r.ur;
        r.ur &= p6.begin_reading_block(block_row, block_col);
        return r;
    }

    vt read_element(int_type element_num)
    {
        vt r;
        r.ur = r.ul = p1.read_element(element_num);
        r.ul += p2.read_element(element_num);
        r.ur += p4.read_element(element_num);
        r.dr = r.dl = p5.read_element(element_num);
        r.dl += r.ur;
        r.dl += p7.read_element(element_num);
        r.ur += p3.read_element(element_num);
        r.dr += r.ur;
        r.ur += p6.read_element(element_num);
        return r;
    }

    zbt end_reading_block(const size_type & block_row, const size_type & block_col)
    {
        zbt r;
        r.ur = r.ul = p1.end_reading_block(block_row, block_col);
        r.ul &= p2.end_reading_block(block_row, block_col);
        r.ur &= p4.end_reading_block(block_row, block_col);
        r.dr = r.dl = p5.end_reading_block(block_row, block_col);
        r.dl &= r.ur;
        r.dl &= p7.end_reading_block(block_row, block_col);
        r.ur &= p3.end_reading_block(block_row, block_col);
        r.dr &= r.ur;
        r.ur &= p6.end_reading_block(block_row, block_col);
        return r;
    }
};

template <typename ValueType, unsigned BlockSideLength>
struct matrix_to_quadtree<ValueType, BlockSideLength, 0>
{
    typedef static_quadtree<bool, 0> zbt;     // true <=> is a zero-block
    typedef static_quadtree<ValueType, 0> vt;

    typedef swappable_block_matrix<ValueType, BlockSideLength> swappable_block_matrix_type;
    typedef typename swappable_block_matrix_type::block_scheduler_type block_scheduler_type;
    typedef typename block_scheduler_type::internal_block_type internal_block_type;
    typedef typename swappable_block_matrix_type::size_type size_type;

    swappable_block_matrix_type m;
    internal_block_type * iblock;

    matrix_to_quadtree(const swappable_block_matrix_type & matrix)
        : m(matrix, matrix.get_height(), matrix.get_width(), 0, 0),
          iblock(0) {}

    matrix_to_quadtree(const swappable_block_matrix_type & matrix,
            const size_type height, const size_type width, const size_type from_row, const size_type from_col)
        : m(matrix, height, width, from_row, from_col),
          iblock(0) {}

    void begin_feeding_block(const size_type & block_row, const size_type & block_col, const zbt)
    { iblock = & m.bs.acquire(m(block_row, block_col)); }

    void feed_element(const int_type element_num, const vt v)
    { (*iblock)[element_num] = v; }

    void feed_and_add_element(const int_type element_num, const vt v)
    { (*iblock)[element_num] += v; }

    void end_feeding_block(const size_type & block_row, const size_type & block_col, const zbt zb)
    {
        m.bs.release(m(block_row, block_col), ! zb);
        iblock = 0;
    }

    zbt begin_reading_block(const size_type & block_row, const size_type & block_col)
    {
        zbt zb = ! m.bs.is_initialized(m(block_row, block_col));
        iblock = & m.bs.acquire(m(block_row, block_col));
        return zb;
    }

    vt read_element(const int_type element_num)
    { return (*iblock)[element_num]; }

    zbt end_reading_block(const size_type & block_row, const size_type & block_col)
    {
        m.bs.release(m(block_row, block_col), false);
        iblock = 0;
        return  ! m.bs.is_initialized(m(block_row, block_col));
    }

    const size_type & get_height_in_blocks()
    { return m.get_height(); }

    const size_type & get_width_in_blocks()
    { return m.get_width(); }
};

template <typename ValueType, unsigned BlockSideLength, unsigned Level>
struct matrix_to_quadtree
{
    typedef static_quadtree<bool, Level> zbt;     // true <=> is a zero-block
    typedef static_quadtree<ValueType, Level> vt;

    typedef matrix_to_quadtree<ValueType, BlockSideLength, Level - 1> smaller_matrix_to_quadtree;

    typedef swappable_block_matrix<ValueType, BlockSideLength> swappable_block_matrix_type;
    typedef typename swappable_block_matrix_type::block_scheduler_type block_scheduler_type;
    typedef typename block_scheduler_type::internal_block_type internal_block_type;
    typedef typename swappable_block_matrix_type::size_type size_type;

    smaller_matrix_to_quadtree ul, ur, dl, dr;

    matrix_to_quadtree(const swappable_block_matrix_type & matrix)
        : ul(matrix, matrix.get_height()/2, matrix.get_width()/2,                     0,                    0),
          ur(matrix, matrix.get_height()/2, matrix.get_width()/2,                     0, matrix.get_width()/2),
          dl(matrix, matrix.get_height()/2, matrix.get_width()/2, matrix.get_height()/2,                    0),
          dr(matrix, matrix.get_height()/2, matrix.get_width()/2, matrix.get_height()/2, matrix.get_width()/2)
    { assert(! (matrix.get_height() % 2 | matrix.get_width() % 2)); }

    matrix_to_quadtree(const swappable_block_matrix_type & matrix,
            const size_type height, const size_type width, const size_type from_row, const size_type from_col)
        : ul(matrix, height/2, width/2, from_row,            from_col),
          ur(matrix, height/2, width/2, from_row,            from_col + width/2),
          dl(matrix, height/2, width/2, from_row + height/2, from_col),
          dr(matrix, height/2, width/2, from_row + height/2, from_col + width/2)
    { assert(! (height % 2 | width % 2)); }

    void begin_feeding_block(const size_type & block_row, const size_type & block_col, const zbt zb)
    {
        ul.begin_feeding_block(block_row, block_col, zb.ul);
        ur.begin_feeding_block(block_row, block_col, zb.ur);
        dl.begin_feeding_block(block_row, block_col, zb.dl);
        dr.begin_feeding_block(block_row, block_col, zb.dr);
    }

    void feed_element(const int_type element_num, const vt v)
    {
        ul.feed_element(element_num, v.ul);
        ur.feed_element(element_num, v.ur);
        dl.feed_element(element_num, v.dl);
        dr.feed_element(element_num, v.dr);
    }

    void feed_and_add_element(const int_type element_num, const vt v)
    {
        ul.feed_and_add_element(element_num, v.ul);
        ur.feed_and_add_element(element_num, v.ur);
        dl.feed_and_add_element(element_num, v.dl);
        dr.feed_and_add_element(element_num, v.dr);
    }

    void end_feeding_block(const size_type & block_row, const size_type & block_col, const zbt zb)
    {
        ul.end_feeding_block(block_row, block_col, zb.ul);
        ur.end_feeding_block(block_row, block_col, zb.ur);
        dl.end_feeding_block(block_row, block_col, zb.dl);
        dr.end_feeding_block(block_row, block_col, zb.dr);
    }

    zbt begin_reading_block(const size_type & block_row, const size_type & block_col)
    {
        zbt zb;
        zb.ul = ul.begin_reading_block(block_row, block_col);
        zb.ur = ur.begin_reading_block(block_row, block_col);
        zb.dl = dl.begin_reading_block(block_row, block_col);
        zb.dr = dr.begin_reading_block(block_row, block_col);
        return zb;
    }

    vt read_element(const int_type element_num)
    {
        vt v;
        v.ul = ul.read_element(element_num);
        v.ur = ur.read_element(element_num);
        v.dl = dl.read_element(element_num);
        v.dr = dr.read_element(element_num);
        return v;
    }

    zbt end_reading_block(const size_type & block_row, const size_type & block_col)
    {
        zbt zb;
        zb.ul = ul.end_reading_block(block_row, block_col);
        zb.ur = ur.end_reading_block(block_row, block_col);
        zb.dl = dl.end_reading_block(block_row, block_col);
        zb.dr = dr.end_reading_block(block_row, block_col);
        return zb;
    }

    const size_type & get_height_in_blocks()
    { return ul.get_height_in_blocks(); }

    const size_type & get_width_in_blocks()
    { return ul.get_width_in_blocks(); }
};

// +-+-+-+-+-+-+ blocked-matrix version +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

//forward declaration
template <typename ValueType, unsigned BlockSideLength>
class blocked_matrix;

//forward declaration for friend
template <typename ValueType, unsigned BlockSideLength>
blocked_matrix<ValueType, BlockSideLength> &
multiply(
    const blocked_matrix<ValueType, BlockSideLength> & A,
    const blocked_matrix<ValueType, BlockSideLength> & B,
    blocked_matrix<ValueType, BlockSideLength> & C,
    unsigned_type max_temp_mem_raw
    );

//! \brief External matrix container

//! \tparam ValueType type of contained objects (POD with no references to internal memory)
//! \tparam BlockSideLength side length of one square matrix block, default is 1024
//!         BlockSideLength*BlockSideLength*sizeof(ValueType) must be divisible by 4096
template <typename ValueType, unsigned BlockSideLength = 1024>
class blocked_matrix
{
    static const unsigned_type block_size = BlockSideLength * BlockSideLength;
    static const unsigned_type raw_block_size = block_size * sizeof(ValueType);

public:
    typedef typed_block<raw_block_size, ValueType> block_type;

private:
    typedef typename block_type::bid_type bid_type;
    typedef typename block_type::iterator element_iterator_type;

    bid_type * bids;
    block_manager * bm;
    const unsigned_type num_rows, num_cols;
    const unsigned_type num_block_rows, num_block_cols;
    MatrixBlockLayout * layout;

public:
    blocked_matrix(unsigned_type num_rows, unsigned_type num_cols, MatrixBlockLayout * given_layout = NULL)
        : num_rows(num_rows), num_cols(num_cols),
          num_block_rows(div_ceil(num_rows, BlockSideLength)),
          num_block_cols(div_ceil(num_cols, BlockSideLength)),
          layout((given_layout != NULL) ? given_layout : new RowMajor)
    {
        layout->set_dimensions(num_block_rows, num_block_cols);
        bm = block_manager::get_instance();
        bids = new bid_type[num_block_rows * num_block_cols];
        bm->new_blocks(striping(), bids, bids + num_block_rows * num_block_cols);
    }

    ~blocked_matrix()
    {
        bm->delete_blocks(bids, bids + num_block_rows * num_block_cols);
        delete[] bids;
        delete layout;
    }

    bid_type & bid(unsigned_type row, unsigned_type col) const
    {
        return *(bids + layout->coords_to_index(row, col));
    }

    //! \brief read in matrix from stream, assuming row-major order
    template <class InputIterator>
    void materialize_from_row_major(InputIterator & i, unsigned_type /*max_temp_mem_raw*/)
    {
        element_iterator_type current_element, first_element_of_row_in_block;

        // if enough space
        // allocate one row of blocks
        block_type * row_of_blocks = new block_type[num_block_cols];

        // iterate block-rows therein element-rows therein block-cols therein element-col
        // fill with elements from iterator rsp. padding with zeros
        for (unsigned_type b_row = 0; b_row < num_block_rows; ++b_row)
        {
            unsigned_type num_e_rows = (b_row < num_block_rows - 1)
                                       ? BlockSideLength : (num_rows - 1) % BlockSideLength + 1;
            // element-rows
            unsigned_type e_row;
            for (e_row = 0; e_row < num_e_rows; ++e_row)
                // block-cols
                for (unsigned_type b_col = 0; b_col < num_block_cols; ++b_col)
                {
                    first_element_of_row_in_block =
                        row_of_blocks[b_col].begin() + e_row * BlockSideLength;
                    unsigned_type num_e_cols = (b_col < num_block_cols - 1)
                                               ? BlockSideLength : (num_cols - 1) % BlockSideLength + 1;
                    // element-cols
                    for (current_element = first_element_of_row_in_block;
                         current_element < first_element_of_row_in_block + num_e_cols;
                         ++current_element)
                    {
                        // read element
                        //todo if (i.empty()) throw exception
                        *current_element = *i;
                        ++i;
                    }
                    // padding element-cols
                    for ( ; current_element < first_element_of_row_in_block + BlockSideLength;
                          ++current_element)
                        *current_element = 0;
                }
            // padding element-rows
            for ( ; e_row < BlockSideLength; ++e_row)
                // padding block-cols
                for (unsigned_type b_col = 0; b_col < num_block_cols; ++b_col)
                {
                    first_element_of_row_in_block =
                        row_of_blocks[b_col].begin() + e_row * BlockSideLength;
                    // padding element-cols
                    for (current_element = first_element_of_row_in_block;
                         current_element < first_element_of_row_in_block + BlockSideLength;
                         ++current_element)
                        *current_element = 0;
                }
            // write block-row to disk
            std::vector<request_ptr> requests;
            for (unsigned_type col = 0; col < num_block_cols; ++col)
                requests.push_back(row_of_blocks[col].write(bid(b_row, col)));
            wait_all(requests.begin(), requests.end());
        }
    }

    template <class OutputIterator>
    void output_to_row_major(OutputIterator & i, unsigned_type /*max_temp_mem_raw*/) const
    {
        element_iterator_type current_element, first_element_of_row_in_block;

        // if enough space
        // allocate one row of blocks
        block_type * row_of_blocks = new block_type[num_block_cols];

        // iterate block-rows therein element-rows therein block-cols therein element-col
        // write elements to iterator
        for (unsigned_type b_row = 0; b_row < num_block_rows; ++b_row)
        {
            // read block-row from disk
            std::vector<request_ptr> requests;
            for (unsigned_type col = 0; col < num_block_cols; ++col)
                requests.push_back(row_of_blocks[col].read(bid(b_row, col)));
            wait_all(requests.begin(), requests.end());

            unsigned_type num_e_rows = (b_row < num_block_rows - 1)
                                       ? BlockSideLength : (num_rows - 1) % BlockSideLength + 1;
            // element-rows
            unsigned_type e_row;
            for (e_row = 0; e_row < num_e_rows; ++e_row)
                // block-cols
                for (unsigned_type b_col = 0; b_col < num_block_cols; ++b_col)
                {
                    first_element_of_row_in_block =
                        row_of_blocks[b_col].begin() + e_row * BlockSideLength;
                    unsigned_type num_e_cols = (b_col < num_block_cols - 1)
                                               ? BlockSideLength : (num_cols - 1) % BlockSideLength + 1;
                    // element-cols
                    for (current_element = first_element_of_row_in_block;
                         current_element < first_element_of_row_in_block + num_e_cols;
                         ++current_element)
                    {
                        // write element
                        //todo if (i.empty()) throw exception
                        *i = *current_element;
                        ++i;
                    }
                }
        }
    }

    friend
    blocked_matrix<ValueType, BlockSideLength> &
    multiply<>(
        const blocked_matrix<ValueType, BlockSideLength> & A,
        const blocked_matrix<ValueType, BlockSideLength> & B,
        blocked_matrix<ValueType, BlockSideLength> & C,
        unsigned_type max_temp_mem_raw);
};

template <typename matrix_type>
class blocked_matrix_row_major_iterator
{
    typedef typename matrix_type::block_type block_type;
    typedef typename matrix_type::value_type value_type;

    matrix_type * matrix;
    block_type * row_of_blocks;
    bool * dirty;
    unsigned_type loaded_row_in_blocks,
        current_element;

public:
    blocked_matrix_row_major_iterator(matrix_type & m)
        : loaded_row_in_blocks(-1),
          current_element(0)
    {
        matrix = &m;
        // allocate one row of blocks
        row_of_blocks = new block_type[m.num_block_cols];
        dirty = new bool[m.num_block_cols];
    }

    blocked_matrix_row_major_iterator(const blocked_matrix_row_major_iterator& other)
    {
        matrix = other.matrix;
        row_of_blocks = NULL;
        dirty = NULL;
        loaded_row_in_blocks = -1;
        current_element = other.current_element;
    }

    blocked_matrix_row_major_iterator& operator=(const blocked_matrix_row_major_iterator& other)
    {

        matrix = other.matrix;
        row_of_blocks = NULL;
        dirty = NULL;
        loaded_row_in_blocks = -1;
        current_element = other.current_element;

        return *this;
    }

    ~blocked_matrix_row_major_iterator()
    {
        //TODO write out

        delete[] row_of_blocks;
        delete[] dirty;
    }

    blocked_matrix_row_major_iterator & operator ++ ()
    {
        ++current_element;
        return *this;
    }

    bool empty() const { return (current_element >= *matrix.num_rows * *matrix.num_cols); }

    value_type& operator * () { return 1 /*TODO*/; }

    const value_type& operator * () const { return 1 /*TODO*/; }
};

//! \brief submatrix of a matrix containing blocks (type block_type) that reside in main memory
template <typename matrix_type>
class panel
{
public:
    typedef typename matrix_type::block_type block_type;
    typedef typename block_type::iterator element_iterator_type;

    block_type * blocks;
    const RowMajor layout;
    unsigned_type height, width;

    panel(const unsigned_type max_height, const unsigned_type max_width)
        : layout(max_width, max_height),
          height(max_height), width(max_width)
    {
        blocks = new block_type[max_height * max_width];
    }

    ~panel()
    {
        delete[] blocks;
    }

    // fill the blocks specified by height and width with zeros
    void clear()
    {
        element_iterator_type elements;

        // iterate blocks
        for (unsigned_type row = 0; row < height; ++row)
            for (unsigned_type col = 0; col < width; ++col)
                // iterate elements
                for (elements = block(row, col).begin(); elements < block(row, col).end(); ++elements)
                    // set element zero
                    *elements = 0;
    }

    // read the blocks specified by height and width
    void read_sync(const matrix_type & from, unsigned_type first_row, unsigned_type first_col) const
    {
        std::vector<request_ptr> requests = read_async(from, first_row, first_col);

        wait_all(requests.begin(), requests.end());
    }

    // read the blocks specified by height and width
    std::vector<request_ptr> &
    read_async(const matrix_type & from, unsigned_type first_row, unsigned_type first_col) const
    {
        std::vector<request_ptr> * requests = new std::vector<request_ptr>; // todo is this the way to go?

        // iterate blocks
        for (unsigned_type row = 0; row < height; ++row)
            for (unsigned_type col = 0; col < width; ++col)
                // post request and save pointer
                (*requests).push_back(block(row, col).read(from.bid(first_row + row, first_col + col)));

        return *requests;
    }

    // write the blocks specified by height and width
    void write_sync(const matrix_type & to, unsigned_type first_row, unsigned_type first_col) const
    {
        std::vector<request_ptr> requests = write_async(to, first_row, first_col);

        wait_all(requests.begin(), requests.end());
    }

    // read the blocks specified by height and width
    std::vector<request_ptr> &
    write_async(const matrix_type & to, unsigned_type first_row, unsigned_type first_col) const
    {
        std::vector<request_ptr> * requests = new std::vector<request_ptr>; // todo is this the way to go?

        // iterate blocks
        for (unsigned_type row = 0; row < height; ++row)
            for (unsigned_type col = 0; col < width; ++col)
                // post request and save pointer
                (*requests).push_back(block(row, col).write(to.bid(first_row + row, first_col + col)));

        return *requests;
    }

    block_type & block(unsigned_type row, unsigned_type col) const
    {
        return *(blocks + layout.coords_to_index(row, col));
    }
};

//! \brief multiplies matrices A and B, adds result to C
//! param pointer to blocks of A,B,C; elements in blocks have to be in row-major
template <typename value_type, unsigned BlockSideLength>
struct low_level_multiply;

//! \brief multiplies matrices A and B, adds result to C, for double entries
//! param pointer to blocks of A,B,C; elements in blocks have to be in row-major
template <unsigned BlockSideLength>
struct low_level_multiply<double, BlockSideLength>
{
    void operator () (double * a, double * b, double * c)
    {
    #if STXXL_BLAS
        gemm_wrapper(BlockSideLength, BlockSideLength, BlockSideLength,
                1.0, false, a,
                     false, b,
                1.0, false, c);
    #else
        for (unsigned_type k = 0; k < BlockSideLength; ++k)
            #if STXXL_PARALLEL
            #pragma omp parallel for
            #endif
            for (int_type i = 0; i < int_type(BlockSideLength); ++i)    //OpenMP does not like unsigned iteration variables
                for (unsigned_type j = 0; j < BlockSideLength; ++j)
                    c[i * BlockSideLength + j] += a[i * BlockSideLength + k] * b[k * BlockSideLength + j];
    #endif
    }
};

template <typename value_type, unsigned BlockSideLength>
struct low_level_multiply
{
    void operator () (value_type * a, value_type * b, value_type * c)
    {
        for (unsigned_type k = 0; k < BlockSideLength; ++k)
            #if STXXL_PARALLEL
            #pragma omp parallel for
            #endif
            for (int_type i = 0; i < int_type(BlockSideLength); ++i)    //OpenMP does not like unsigned iteration variables
                for (unsigned_type j = 0; j < BlockSideLength; ++j)
                    c[i * BlockSideLength + j] += a[i * BlockSideLength + k] * b[k * BlockSideLength + j];
    }
};


//! \brief multiplies blocks of A and B, adds result to C
//! param pointer to blocks of A,B,C; elements in blocks have to be in row-major
template <typename block_type, unsigned BlockSideLength>
void multiply_block(/*const*/ block_type & BlockA, /*const*/ block_type & BlockB, block_type & BlockC)
{
    typedef typename block_type::value_type value_type;

    value_type * a = BlockA.begin(), * b = BlockB.begin(), * c = BlockC.begin();
    low_level_multiply<value_type, BlockSideLength> llm;
    llm(a, b, c);
}

// multiply panels from A and B, add result to C
// param BlocksA pointer to first Block of A assumed in row-major
template <typename matrix_type, unsigned BlockSideLength>
void multiply_panel(const panel<matrix_type> & PanelA, const panel<matrix_type> & PanelB, panel<matrix_type> & PanelC)
{
    typedef typename matrix_type::block_type block_type;

    assert(PanelA.width == PanelB.height);
    assert(PanelC.height == PanelA.height);
    assert(PanelC.width == PanelB.width);

    for (unsigned_type row = 0; row < PanelC.height; ++row)
        for (unsigned_type col = 0; col < PanelC.width; ++col)
            for (unsigned_type l = 0; l < PanelA.width; ++l)
                multiply_block<block_type, BlockSideLength>(PanelA.block(row, l), PanelB.block(l, col), PanelC.block(row, col));
}

//! \brief multiply the matrices A and B, gaining C
template <typename ValueType, unsigned BlockSideLength>
blocked_matrix<ValueType, BlockSideLength> &
multiply(
    const blocked_matrix<ValueType, BlockSideLength> & A,
    const blocked_matrix<ValueType, BlockSideLength> & B,
    blocked_matrix<ValueType, BlockSideLength> & C,
    unsigned_type max_temp_mem_raw
    )
{
    typedef blocked_matrix<ValueType, BlockSideLength> matrix_type;
    typedef typename matrix_type::block_type block_type;

    assert(A.num_cols == B.num_rows);
    assert(C.num_rows == A.num_rows);
    assert(C.num_cols == B.num_cols);

    // preparation:
    // calculate panel size from blocksize and max_temp_mem_raw
    unsigned_type panel_max_side_length_in_blocks = sqrt(double(max_temp_mem_raw / 3 / block_type::raw_size));
    unsigned_type panel_max_num_n_in_blocks = panel_max_side_length_in_blocks, 
            panel_max_num_k_in_blocks = panel_max_side_length_in_blocks, 
            panel_max_num_m_in_blocks = panel_max_side_length_in_blocks,
            matrix_num_n_in_panels = div_ceil(C.num_block_rows, panel_max_num_n_in_blocks),
            matrix_num_k_in_panels = div_ceil(A.num_block_cols, panel_max_num_k_in_blocks),
            matrix_num_m_in_panels = div_ceil(C.num_block_cols, panel_max_num_m_in_blocks);
    /*  n, k and m denote the three dimensions of a matrix multiplication, according to the following ascii-art diagram:
     * 
     *                 +--m--+          
     *  +----k-----+   |     |   +--m--+
     *  |          |   |     |   |     |
     *  n    A     | • k  B  | = n  C  |
     *  |          |   |     |   |     |
     *  +----------+   |     |   +-----+
     *                 +-----+          
     */
    
    // reserve mem for a,b,c-panel
    panel<matrix_type> panelA(panel_max_num_n_in_blocks, panel_max_num_k_in_blocks);
    panel<matrix_type> panelB(panel_max_num_k_in_blocks, panel_max_num_m_in_blocks);
    panel<matrix_type> panelC(panel_max_num_n_in_blocks, panel_max_num_m_in_blocks);
    
    // multiply:
    // iterate rows and cols (panel wise) of c
    for (unsigned_type panel_row = 0; panel_row < matrix_num_n_in_panels; ++panel_row)
    {	//for each row
        panelC.height = panelA.height = (panel_row < matrix_num_n_in_panels -1) ?
                panel_max_num_n_in_blocks : /*last row*/ (C.num_block_rows-1) % panel_max_num_n_in_blocks +1;
        for (unsigned_type panel_col = 0; panel_col < matrix_num_m_in_panels; ++panel_col)
        {	//for each column

        	//for each panel of C
            panelC.width = panelB.width = (panel_col < matrix_num_m_in_panels -1) ?
                    panel_max_num_m_in_blocks : (C.num_block_cols-1) % panel_max_num_m_in_blocks +1;
            // initialize c-panel
            //TODO: acquire panelC (uninitialized)
            panelC.clear();
            // iterate a-row,b-col
            for (unsigned_type l = 0; l < matrix_num_k_in_panels; ++l)
            {	//scalar product over row/column
                panelA.width = panelB.height = (l < matrix_num_k_in_panels -1) ?
                        panel_max_num_k_in_blocks : (A.num_block_cols-1) % panel_max_num_k_in_blocks +1;
                // load a,b-panel
                //TODO: acquire panelA
                panelA.read_sync(A, panel_row*panel_max_num_n_in_blocks, l*panel_max_num_k_in_blocks);
                //TODO: acquire panelB
                panelB.read_sync(B, l * panel_max_num_k_in_blocks, panel_col * panel_max_num_m_in_blocks);
                // multiply and add to c
                multiply_panel<matrix_type, BlockSideLength>(panelA, panelB, panelC);
                //TODO: release panelA
                //TODO: release panelB
            }
            //TODO: release panelC (write)
            // write c-panel
            panelC.write_sync(C, panel_row * panel_max_num_n_in_blocks, panel_col * panel_max_num_m_in_blocks);
        }
    }

    return C;
}

__STXXL_END_NAMESPACE

#endif /* STXXL_MATRIX_HEADER */
// vim: et:ts=4:sw=4
