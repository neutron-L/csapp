/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
static void trans_32x32(int M, int N, int A[N][M], int B[M][N])
{
    int a0, a1, a2, a3, a4, a5, a6, a7;
    for (int i = 0; i < N; i += 8)
        for (int j = 0; j < M; j += 8)
            for (int k = 0; k < 8; ++k)
        {
            a0 = A[i + k][j + 0]; 
            a1 = A[i + k][j + 1]; 
            a2 = A[i + k][j + 2]; 
            a3 = A[i + k][j + 3]; 
            a4 = A[i + k][j + 4]; 
            a5 = A[i + k][j + 5]; 
            a6 = A[i + k][j + 6]; 
            a7 = A[i + k][j + 7]; 

            B[j + 0][i + k] = a0;
            B[j + 1][i + k] = a1;
            B[j + 2][i + k] = a2;
            B[j + 3][i + k] = a3;
            B[j + 4][i + k] = a4;
            B[j + 5][i + k] = a5;
            B[j + 6][i + k] = a6;
            B[j + 7][i + k] = a7;
        }
}


static void trans_64x64(int M, int N, int A[N][M], int B[M][N])
{
    int a0, a1, a2, a3, a4, a5, a6, a7;
    int tmp;
    int i, j, k;

    for (i = 0; i < N; i += 8)
        for (j = 0; j < M; j += 8)
        {
            for (k = 0; k < 4; ++k)
            {
                a0 = A[i + k][j + 0];
                a1 = A[i + k][j + 1];
                a2 = A[i + k][j + 2];
                a3 = A[i + k][j + 3];

                a4 = A[i + k][j + 4];
                a5 = A[i + k][j + 5];
                a6 = A[i + k][j + 6];
                a7 = A[i + k][j + 7];

                B[j + 0][i + k] = a0;
                B[j + 1][i + k] = a1;
                B[j + 2][i + k] = a2;
                B[j + 3][i + k] = a3;

                B[j + 0][i + k + 4] = a4;
                B[j + 1][i + k + 4] = a5;
                B[j + 2][i + k + 4] = a6;
                B[j + 3][i + k + 4] = a7;
            }

            for (k = 0; k < 4; ++k)
            {
                a0 = A[i + 4][j + k];
                a1 = A[i + 5][j + k];
                a2 = A[i + 6][j + k];
                a3 = A[i + 7][j + k];

                a4 = A[i + 4][j + k + 4];
                a5 = A[i + 5][j + k + 4];
                a6 = A[i + 6][j + k + 4];
                a7 = A[i + 7][j + k + 4];

                tmp = B[j + k][i + 4], B[j + k][i +4] = a0, a0 = tmp; 
                tmp = B[j + k][i + 5], B[j + k][i +5] = a1, a1 = tmp; 
                tmp = B[j + k][i + 6], B[j + k][i +6] = a2, a2 = tmp; 
                tmp = B[j + k][i + 7], B[j + k][i +7] = a3, a3 = tmp; 

                B[j + k + 4][i + 0] = a0;
                B[j + k + 4][i + 1] = a1;
                B[j + k + 4][i + 2] = a2;
                B[j + k + 4][i + 3] = a3;
                B[j + k + 4][i + 4] = a4;
                B[j + k + 4][i + 5] = a5;
                B[j + k + 4][i + 6] = a6;
                B[j + k + 4][i + 7] = a7;
            }
        }   
}

static void trans_61x67(int M, int N, int A[N][M], int B[M][N])
{
    int a0, a1, a2, a3, a4, a5, a6, a7;
    int tmp;
    int i, j, k;

    for (i = 0; i < N / 8 * 8; i += 8)
        for (j = 0; j < M / 8 * 8; j += 8)
        {
            for (k = 0; k < 4; ++k)
            {
                a0 = A[i + k][j + 0];
                a1 = A[i + k][j + 1];
                a2 = A[i + k][j + 2];
                a3 = A[i + k][j + 3];

                a4 = A[i + k][j + 4];
                a5 = A[i + k][j + 5];
                a6 = A[i + k][j + 6];
                a7 = A[i + k][j + 7];

                B[j + 0][i + k] = a0;
                B[j + 1][i + k] = a1;
                B[j + 2][i + k] = a2;
                B[j + 3][i + k] = a3;

                B[j + 0][i + k + 4] = a4;
                B[j + 1][i + k + 4] = a5;
                B[j + 2][i + k + 4] = a6;
                B[j + 3][i + k + 4] = a7;
            }

            for (k = 0; k < 4; ++k)
            {
                a0 = A[i + 4][j + k];
                a1 = A[i + 5][j + k];
                a2 = A[i + 6][j + k];
                a3 = A[i + 7][j + k];

                a4 = A[i + 4][j + k + 4];
                a5 = A[i + 5][j + k + 4];
                a6 = A[i + 6][j + k + 4];
                a7 = A[i + 7][j + k + 4];

                tmp = B[j + k][i + 4], B[j + k][i +4] = a0, a0 = tmp; 
                tmp = B[j + k][i + 5], B[j + k][i +5] = a1, a1 = tmp; 
                tmp = B[j + k][i + 6], B[j + k][i +6] = a2, a2 = tmp; 
                tmp = B[j + k][i + 7], B[j + k][i +7] = a3, a3 = tmp; 

                B[j + k + 4][i + 0] = a0;
                B[j + k + 4][i + 1] = a1;
                B[j + k + 4][i + 2] = a2;
                B[j + k + 4][i + 3] = a3;
                B[j + k + 4][i + 4] = a4;
                B[j + k + 4][i + 5] = a5;
                B[j + k + 4][i + 6] = a6;
                B[j + k + 4][i + 7] = a7;
            }
        }   

    // transpose last 3 columns and last 5 rows
    for (i = N / 8 * 8; i < N; i += 3)
        for (j = 0; j < M; j += 8)
        {
            for (a0 = 0; a0 < 3; ++a0)
            {
                if (i + a0 >= N)
                    break;
                for (a1 = 0; a1 < 8; ++a1)
                {
                    if (j + a1 >= M)
                        break;
                    B[j + a1][i + a0] = A[i + a0][j + a1]; 
                }
            }
        }

    for (i = 0; i < N; ++i)
        for (j = M / 8 * 8; j < M; ++j)
            B[j][i] = A[i][j];
}

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (N == 32) // 32 x 32
        trans_32x32(M, N, A, B);
    else if (N == 64) // 64 x 64
        trans_64x64(M, N, A, B);
    else // 61 x 67
        trans_61x67(M, N, A, B);
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

