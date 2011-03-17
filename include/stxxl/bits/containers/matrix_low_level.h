/***************************************************************************
 *  include/stxxl/bits/containers/matrix_low_level.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MATRIX_LOW_LEVEL_HEADER
#define STXXL_MATRIX_LOW_LEVEL_HEADER

#ifndef STXXL_BLAS
#define STXXL_BLAS 0
#endif

#include <stxxl/bits/namespace.h>

__STXXL_BEGIN_NAMESPACE

template <unsigned BlockSideLength, bool transposed>
struct rmindex;

template <unsigned BlockSideLength>
struct rmindex<BlockSideLength, false>
{
    inline rmindex(const int_type row, const int_type col) : i(row * BlockSideLength + col) {}
    inline operator int_type & () { return i; }
private:
    int_type i;
};

template <unsigned BlockSideLength>
struct rmindex<BlockSideLength, true>
{
    inline rmindex(const int_type row, const int_type col) : i(row + col * BlockSideLength) {}
    inline operator int_type & () { return i; }
private:
    int_type i;
};

template <typename ValueType, unsigned BlockSideLength, bool a_transposed, bool b_transposed, class Op>
struct low_level_matrix_op_3
{
    //! \brief c = a <op> b; for arbitrary entries
    low_level_matrix_op_3(ValueType * c, const ValueType * a, const ValueType * b, Op op = Op())
    {
        if (a)
            if (b)
                #if STXXL_PARALLEL
                #pragma omp parallel for
                #endif
                for (int_type row = 0; row < int_type(BlockSideLength); ++row)
                    for (int_type col = 0; col < int_type(BlockSideLength); ++col)
                        op(c[rmindex<BlockSideLength, false>(row, col)],
                                a[rmindex<BlockSideLength, a_transposed>(row, col)],
                                b[rmindex<BlockSideLength, b_transposed>(row, col)]);
            else
                #if STXXL_PARALLEL
                #pragma omp parallel for
                #endif
                for (int_type row = 0; row < int_type(BlockSideLength); ++row)
                    for (int_type col = 0; col < int_type(BlockSideLength); ++col)
                        op(c[rmindex<BlockSideLength, false>(row, col)],
                                a[rmindex<BlockSideLength, a_transposed>(row, col)], 0);
        else
        {
            assert(b /* do not add nothing to nothing */);
            #if STXXL_PARALLEL
            #pragma omp parallel for
            #endif
            for (int_type row = 0; row < int_type(BlockSideLength); ++row)
                for (int_type col = 0; col < int_type(BlockSideLength); ++col)
                    op(c[rmindex<BlockSideLength, false>(row, col)],
                                0, b[rmindex<BlockSideLength, b_transposed>(row, col)]);
        }
    }
};

template <typename ValueType, unsigned BlockSideLength, bool a_transposed, class Op>
struct low_level_matrix_op_2
{
    //! \brief c <op>= a; for arbitrary entries
    low_level_matrix_op_2(ValueType * c, const ValueType * a, Op op = Op())
    {
        if (a)
            #if STXXL_PARALLEL
            #pragma omp parallel for
            #endif
            for (int_type row = 0; row < int_type(BlockSideLength); ++row)
                for (int_type col = 0; col < int_type(BlockSideLength); ++col)
                    op(c[rmindex<BlockSideLength, false>(row, col)],
                            a[rmindex<BlockSideLength, a_transposed>(row, col)]);
    }
};

//! \brief c = a; for arbitrary entries
template <typename ValueType, unsigned BlockSideLength, bool a_transposed, class Op>
struct low_level_matrix_op_1
{
    low_level_matrix_op_1(ValueType * c, const ValueType * a, Op op = Op())
    {
        assert(a);
        #if STXXL_PARALLEL
        #pragma omp parallel for
        #endif
        for (int_type row = 0; row < int_type(BlockSideLength); ++row)
            for (int_type col = 0; col < int_type(BlockSideLength); ++col)
                c[rmindex<BlockSideLength, false>(row, col)] =
                        op(a[rmindex<BlockSideLength, a_transposed>(row, col)]);
    }
};

#if STXXL_BLAS
typedef int blas_int;
extern "C" void dgemm_(const char *transa, const char *transb,
        const blas_int *m, const blas_int *n, const blas_int *k,
        const double *alpha, const double *a, const blas_int *lda,
        const double *b, const blas_int *ldb,
        const double *beta, double *c, const blas_int *ldc);

extern "C" void sgemm_(const char *transa, const char *transb,
        const blas_int *m, const blas_int *n, const blas_int *k,
        const float *alpha, const float *a, const blas_int *lda,
        const float *b, const blas_int *ldb,
        const float *beta, float *c, const blas_int *ldc);

typedef std::complex<double> blas_double_complex;
extern "C" void zgemm_(const char *transa, const char *transb,
        const blas_int *m, const blas_int *n, const blas_int *k,
        const blas_double_complex *alpha, const blas_double_complex *a, const blas_int *lda,
        const blas_double_complex *b, const blas_int *ldb,
        const blas_double_complex *beta, blas_double_complex *c, const blas_int *ldc);

typedef std::complex<float> blas_single_complex;
extern "C" void cgemm_(const char *transa, const char *transb,
        const blas_int *m, const blas_int *n, const blas_int *k,
        const blas_single_complex *alpha, const blas_single_complex *a, const blas_int *lda,
        const blas_single_complex *b, const blas_int *ldb,
        const blas_single_complex *beta, blas_single_complex *c, const blas_int *ldc);

template <typename ValueType>
void gemm_(const char *transa, const char *transb,
        const blas_int *m, const blas_int *n, const blas_int *k,
        const ValueType *alpha, const ValueType *a, const blas_int *lda,
        const ValueType *b, const blas_int *ldb,
        const ValueType *beta, ValueType *c, const blas_int *ldc);

//! \brief calculates c = alpha * a * b + beta * c
//! \tparam ValueType type of elements
//! \param n height of a and c
//! \param l width of a and height of b
//! \param m width of b and c
//! \param a_in_col_major if a is stored in column-major rather than row-major
//! \param b_in_col_major if b is stored in column-major rather than row-major
//! \param c_in_col_major if c is stored in column-major rather than row-major
template <typename ValueType>
void gemm_wrapper(const blas_int n, const blas_int l, const blas_int m,
        const ValueType alpha, const bool a_in_col_major, const ValueType *a,
                               const bool b_in_col_major, const ValueType *b,
        const ValueType beta,  const bool c_in_col_major,       ValueType *c)
{
    const blas_int& stride_in_a = a_in_col_major ? n : l;
    const blas_int& stride_in_b = b_in_col_major ? l : m;
    const blas_int& stride_in_c = c_in_col_major ? n : m;
    const char transa = a_in_col_major xor c_in_col_major ? 'T' : 'N';
    const char transb = b_in_col_major xor c_in_col_major ? 'T' : 'N';
    if (c_in_col_major)
        // blas expects matrices in column-major unless specified via transa rsp. transb
        gemm_(&transa, &transb, &n, &m, &l, &alpha, a, &stride_in_a, b, &stride_in_b, &beta, c, &stride_in_c);
    else
        // blas expects matrices in column-major, so we calculate c^T = alpha * b^T * a^T + beta * c^T
        gemm_(&transb, &transa, &m, &n, &l, &alpha, b, &stride_in_b, a, &stride_in_a, &beta, c, &stride_in_c);
}

template <>
void gemm_(const char *transa, const char *transb,
        const blas_int *m, const blas_int *n, const blas_int *k,
        const double *alpha, const double *a, const blas_int *lda,
        const double *b, const blas_int *ldb,
        const double *beta, double *c, const blas_int *ldc)
{ dgemm_(transa, transb, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc); }

template <>
void gemm_(const char *transa, const char *transb,
        const blas_int *m, const blas_int *n, const blas_int *k,
        const float *alpha, const float *a, const blas_int *lda,
        const float *b, const blas_int *ldb,
        const float *beta, float *c, const blas_int *ldc)
{ sgemm_(transa, transb, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc); }

template <>
void gemm_(const char *transa, const char *transb,
        const blas_int *m, const blas_int *n, const blas_int *k,
        const blas_double_complex *alpha, const blas_double_complex *a, const blas_int *lda,
        const blas_double_complex *b, const blas_int *ldb,
        const blas_double_complex *beta, blas_double_complex *c, const blas_int *ldc)
{ zgemm_(transa, transb, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc); }

template <>
void gemm_(const char *transa, const char *transb,
        const blas_int *m, const blas_int *n, const blas_int *k,
        const blas_single_complex *alpha, const blas_single_complex *a, const blas_int *lda,
        const blas_single_complex *b, const blas_int *ldb,
        const blas_single_complex *beta, blas_single_complex *c, const blas_int *ldc)
{ cgemm_(transa, transb, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc); }

//! \brief multiplies matrices A and B, adds result to C, for arbitrary entries
//! param pointer to blocks of A,B,C; elements in blocks have to be in row-major
/* designated usage as:
 * void
 * low_level_matrix_multiply_and_add(const double * a, bool a_in_col_major,
                                     const double * b, bool b_in_col_major,
                                     double * c, const bool c_in_col_major)  */
template <typename ValueType, unsigned BlockSideLength>
struct low_level_matrix_multiply_and_add
{
    low_level_matrix_multiply_and_add(const ValueType * a, bool a_in_col_major,
                                      const ValueType * b, bool b_in_col_major,
                                      ValueType * c, const bool c_in_col_major)
    {
        if (c_in_col_major)
        {
            std::swap(a,b);
            bool a_cm = ! b_in_col_major;
            b_in_col_major = ! a_in_col_major;
            a_in_col_major = a_cm;
        }
        if (! a_in_col_major)
        {
            if (! b_in_col_major)
            {   // => both row-major
                #if STXXL_PARALLEL
                #pragma omp parallel for
                #endif
                for (int_type i = 0; i < int_type(BlockSideLength); ++i)    //OpenMP does not like unsigned iteration variables
                  for (unsigned_type k = 0; k < BlockSideLength; ++k)
                      for (unsigned_type j = 0; j < BlockSideLength; ++j)
                          c[i * BlockSideLength + j] += a[i * BlockSideLength + k] * b[k * BlockSideLength + j];
            }
            else
            {   // => a row-major, b col-major
                #if STXXL_PARALLEL
                #pragma omp parallel for
                #endif
                for (int_type i = 0; i < int_type(BlockSideLength); ++i)    //OpenMP does not like unsigned iteration variables
                    for (unsigned_type j = 0; j < BlockSideLength; ++j)
                        for (unsigned_type k = 0; k < BlockSideLength; ++k)
                          c[i * BlockSideLength + j] += a[i * BlockSideLength + k] * b[k + j * BlockSideLength];
            }
        }
        else
        {
            if (! b_in_col_major)
            {   // => a col-major, b row-major
                #if STXXL_PARALLEL
                #pragma omp parallel for
                #endif
                for (int_type i = 0; i < int_type(BlockSideLength); ++i)    //OpenMP does not like unsigned iteration variables
                  for (unsigned_type k = 0; k < BlockSideLength; ++k)
                      for (unsigned_type j = 0; j < BlockSideLength; ++j)
                          c[i * BlockSideLength + j] += a[i + k * BlockSideLength] * b[k * BlockSideLength + j];
            }
            else
            {   // => both col-major
                #if STXXL_PARALLEL
                #pragma omp parallel for
                #endif
                for (int_type i = 0; i < int_type(BlockSideLength); ++i)    //OpenMP does not like unsigned iteration variables
                  for (unsigned_type k = 0; k < BlockSideLength; ++k)
                      for (unsigned_type j = 0; j < BlockSideLength; ++j)
                          c[i * BlockSideLength + j] += a[i + k * BlockSideLength] * b[k + j * BlockSideLength];
            }
        }
    }
};

//! \brief multiplies matrices A and B, adds result to C, for double entries
template <unsigned BlockSideLength>
struct low_level_matrix_multiply_and_add<double, BlockSideLength>
{
    low_level_matrix_multiply_and_add(const double * a, bool a_in_col_major,
                                      const double * b, bool b_in_col_major,
                                      double * c, const bool c_in_col_major)
    {
        gemm_wrapper<double>(BlockSideLength, BlockSideLength, BlockSideLength,
                1.0, a_in_col_major, a,
                     b_in_col_major, b,
                1.0, c_in_col_major, c);
    }
};

//! \brief multiplies matrices A and B, adds result to C, for float entries
template <unsigned BlockSideLength>
struct low_level_matrix_multiply_and_add<float, BlockSideLength>
{
    low_level_matrix_multiply_and_add(const float * a, bool a_in_col_major,
                                      const float * b, bool b_in_col_major,
                                      float * c, const bool c_in_col_major)
    {
        gemm_wrapper<float>(BlockSideLength, BlockSideLength, BlockSideLength,
                1.0, a_in_col_major, a,
                     b_in_col_major, b,
                1.0, c_in_col_major, c);
    }
};

//! \brief multiplies matrices A and B, adds result to C, for complex<float> entries
template <unsigned BlockSideLength>
struct low_level_matrix_multiply_and_add<blas_single_complex, BlockSideLength>
{
    low_level_matrix_multiply_and_add(const blas_single_complex * a, bool a_in_col_major,
                                      const blas_single_complex * b, bool b_in_col_major,
                                      blas_single_complex * c, const bool c_in_col_major)
    {
        gemm_wrapper<blas_single_complex>(BlockSideLength, BlockSideLength, BlockSideLength,
                1.0, a_in_col_major, a,
                     b_in_col_major, b,
                1.0, c_in_col_major, c);
    }
};

//! \brief multiplies matrices A and B, adds result to C, for complex<double> entries
template <unsigned BlockSideLength>
struct low_level_matrix_multiply_and_add<blas_double_complex, BlockSideLength>
{
    low_level_matrix_multiply_and_add(const blas_double_complex * a, bool a_in_col_major,
                                      const blas_double_complex * b, bool b_in_col_major,
                                      blas_double_complex * c, const bool c_in_col_major)
    {
        gemm_wrapper<blas_double_complex>(BlockSideLength, BlockSideLength, BlockSideLength,
                1.0, a_in_col_major, a,
                     b_in_col_major, b,
                1.0, c_in_col_major, c);
    }
};
#endif

__STXXL_END_NAMESPACE

#endif /* STXXL_MATRIX_LOW_LEVEL_HEADER */
