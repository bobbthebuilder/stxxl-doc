/***************************************************************************
 *  include/stxxl/bits/containers/matrix.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010 Raoul Steffen <R-Steffen@gmx.de>
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

#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/mng/typed_block.h>
#include <stxxl/bits/containers/matrix_layouts.h>


__STXXL_BEGIN_NAMESPACE

//! \brief External matrix container

//! \tparam ValueType type of contained objects (POD with no references to internal memory)
//! \tparam BlockSideLength side length of one square matrix block, default is 1024
//!         BlockSideLength*BlockSideLength*sizeof(ValueType) must be divisible by 4096
//! \tparam Layout layout in which the blocks are ordered on disk among each other
template <typename ValueType, unsigned BlockSideLength = 1024, class Layout = RowMajor>
class matrix
{
    static const unsigned_type block_size = BlockSideLength * BlockSideLength;
    static const unsigned_type raw_block_size = block_size * sizeof(ValueType);

public:
    //typedef typename matrix<ValueType, BlockSideLength, Layout> matrix_type;
    typedef typed_block<raw_block_size, ValueType> block_type;

private:
    typedef typename block_type::bid_type bid_type;
    typedef typename block_type::iterator element_iterator_type;

    bid_type * bids;
    block_manager * bm;
    const unsigned_type num_rows, num_cols;
    const unsigned_type num_block_rows, num_block_cols;
    const Layout layout;

public:
    matrix(unsigned_type num_rows, unsigned_type num_cols)
        : num_rows(num_rows), num_cols(num_cols),
          num_block_rows(div_ceil(num_rows, BlockSideLength)),
          num_block_cols(div_ceil(num_cols, BlockSideLength)),
          layout(num_block_rows, num_block_cols)
    {
        bm = block_manager::get_instance();
        bids = new bid_type[num_block_rows * num_block_cols];
        bm->new_blocks(striping(), bids, bids + num_block_rows * num_block_cols);
    }

    ~matrix()
    {
        bm->delete_blocks(bids, bids + num_block_rows * num_block_cols);
        delete[] bids;
    }

    bid_type & bid(unsigned_type row, unsigned_type col) const
    {
        return *(bids + layout.coords_to_index(row, col));
//        int foo;  // this line is here to cause a compiler warning, to see if warnings are displayed by the IDE
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

    //friend declaration
    template <typename SomeValueType, unsigned SomeBlockSideLength, class SomeLayout>
    friend matrix<SomeValueType, SomeBlockSideLength, SomeLayout> &
    multiply(
        const matrix<SomeValueType, SomeBlockSideLength, SomeLayout> & A,
        const matrix<SomeValueType, SomeBlockSideLength, SomeLayout> & B,
        matrix<SomeValueType, SomeBlockSideLength, SomeLayout> & C,
        unsigned_type max_temp_mem_raw);
};

template <typename matrix_type>
class matrix_row_major_iterator
{
    typedef typename matrix_type::block_type block_type;

    matrix_type * matrix;
    block_type * row_of_blocks;
    bool * dirty;
    unsigned_type loaded_row_in_blocks,
        current_element;

public:
    matrix_row_major_iterator(matrix_type & m)
        : loaded_row_in_blocks(-1),
          current_element(0)
    {
        matrix = &m;
        // allocate one row of blocks
        row_of_blocks = new block_type[m.num_block_cols];
        dirty = new bool[m.num_block_cols];
    }

    ~matrix_row_major_iterator()
    {
        //TODO write out

        delete[] row_of_blocks;
        delete[] dirty;
    }

    matrix_row_major_iterator & operator ++ ()
    {
        ++current_element;
        return *this;
    }

    bool empty() const { return (current_element >= *matrix.num_rows * *matrix.num_cols); }

    int operator * () { return 1; }
};

//! \brief submatrix of a matrix containing blocks (type block_type) that reside in main memory
template <typename matrix_type, class Layout = RowMajor>
class panel
{
public:
    typedef typename matrix_type::block_type block_type;
    typedef typename block_type::iterator element_iterator_type;

    block_type * blocks;
    const Layout layout;
    unsigned_type height, width;

    panel(const unsigned_type max_height, const unsigned_type max_width)
        : layout(max_height, max_width),
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

#if STXXL_BLAS == 1
extern "C" void dgemm_(void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *);
#elif STXXL_BLAS == 2
enum blas_order_type {
            blas_rowmajor = 101,
            blas_colmajor = 102 };
enum blas_trans_type {
            blas_no_trans   = 111,
            blas_trans      = 112,
            blas_conj_trans = 113 };
extern "C" void cblas_dgemm( enum blas_order_type order, enum blas_trans_type transa,
                 enum blas_trans_type transb, int m, int n, int k,
                 double alpha, const double *a, int lda, const double *b,
                 int ldb, double beta, double *c, int ldc );
#endif


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
    #if STXXL_BLAS == 1
        int n = BlockSideLength;
        char transpose = 'N';
        double alpha = 1.0;
        double beta = 1.0;

        dgemm_(&transpose, &transpose, &n, &n, &n, &alpha, a, &n, b, &n, &beta, c, &n);
    #elif STXXL_BLAS == 2
        cblas_dgemm(blas_rowmajor, blas_no_trans,
                blas_no_trans, BlockSideLength, BlockSideLength, BlockSideLength,
                1.0, a, BlockSideLength, b,
                BlockSideLength, 1.0, c, BlockSideLength);
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
template <typename ValueType, unsigned BlockSideLength, class Layout>
matrix<ValueType, BlockSideLength, Layout> &
multiply(
    const matrix<ValueType, BlockSideLength, Layout> & A,
    const matrix<ValueType, BlockSideLength, Layout> & B,
    matrix<ValueType, BlockSideLength, Layout> & C,
    unsigned_type max_temp_mem_raw
    )
{
    typedef matrix<ValueType, BlockSideLength, Layout> matrix_type;
    typedef typename matrix_type::block_type block_type;

    assert(A.num_cols == B.num_rows);
    assert(C.num_rows == A.num_rows);
    assert(C.num_cols == B.num_cols);

    // preparation:
    // calculate panel size from blocksize and max_temp_mem_raw
    unsigned_type panel_max_side_length_in_blocks = sqrt(double(max_temp_mem_raw / 3 / block_type::raw_size));
    unsigned_type panel_max_num_n_in_blocks = panel_max_side_length_in_blocks, 
            panel_max_num_l_in_blocks = panel_max_side_length_in_blocks, 
            panel_max_num_m_in_blocks = panel_max_side_length_in_blocks,
            matrix_num_n_in_panels = div_ceil(C.num_block_rows, panel_max_num_n_in_blocks),
            matrix_num_l_in_panels = div_ceil(A.num_block_cols, panel_max_num_l_in_blocks),
            matrix_num_m_in_panels = div_ceil(C.num_block_cols, panel_max_num_m_in_blocks);
    /*  n, m and l denote the three dimensions of a matrix multiplication, according to the following ascii-art diagram:
     * 
     *                 +--m--+          
     *  +----l-----+   |     |   +--m--+
     *  |          |   |     |   |     |
     *  n    A     | • l  B  | = n  C  |
     *  |          |   |     |   |     |
     *  +----------+   |     |   +-----+
     *                 +-----+          
     */
    
    // reserve mem for a,b,c-panel
    panel<matrix_type> panelA(panel_max_num_n_in_blocks, panel_max_num_l_in_blocks);
    panel<matrix_type> panelB(panel_max_num_l_in_blocks, panel_max_num_m_in_blocks);
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
            panelC.clear();
            // iterate a-row,b-col
            for (unsigned_type l = 0; l < matrix_num_l_in_panels; ++l)
            {	//scalar product over row/column
                panelA.width = panelB.height = (l < matrix_num_l_in_panels -1) ?
                        panel_max_num_l_in_blocks : (A.num_block_cols-1) % panel_max_num_l_in_blocks +1;
                // load a,b-panel
                panelA.read_sync(A, panel_row*panel_max_num_n_in_blocks, l*panel_max_num_l_in_blocks);
                panelB.read_sync(B, l * panel_max_num_l_in_blocks, panel_col * panel_max_num_m_in_blocks);
                // multiply and add to c
                multiply_panel<matrix_type, BlockSideLength>(panelA, panelB, panelC);
            }
            // write c-panel
            panelC.write_sync(C, panel_row * panel_max_num_n_in_blocks, panel_col * panel_max_num_m_in_blocks);
        }
    }

    return C;
}

__STXXL_END_NAMESPACE

#endif /* STXXL_MATRIX_HEADER */
// vim: et:ts=4:sw=4
