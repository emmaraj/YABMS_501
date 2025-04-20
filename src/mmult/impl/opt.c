/* opt.c
 *
 * Author:
 * Date  :
 *
 *  Description
 */

/* Standard C includes */
#include <stdlib.h>

/* Include common headers */
#include "common/macros.h"
#include "common/types.h"

/* Include application-specific headers */
#include "include/types.h"

/* Alternative Implementation */
#pragma GCC push_options
#pragma GCC optimize("O1")
void *impl_scalar_opt(void *args)
{
  /* Get the argument struct */
  args_t *parsed_args = (args_t *)args;

  /* Get all the arguments */
  register size_t M = parsed_args->size_m;
  register size_t N = parsed_args->size_n;
  register size_t P = parsed_args->size_p;
  int b = parsed_args->batch_size;

  // Cast buffers to 2D float arrays
  register const float *matrix_A = (const float *)parsed_args->matrix_A;
  register const float *matrix_B = (const float *)parsed_args->matrix_B;
  register float *matrix_R = (float *)parsed_args->matrix_R;

  // Initialize output matrix R to 0
  for (int i = 0; i < M * P; i++)
  {
    matrix_R[i] = 0.0f;
  }

  // Blocked matrix multiplication
  for (int ii = 0; ii < M; ii += b)
  {
    for (int jj = 0; jj < P; jj += b)
    {
      for (int kk = 0; kk < N; kk += b)
      {
        // Compute sub-matrix multiplication
        for (int i = ii; i < ((ii + b > M) ? M : ii + b); i++)
        {
          for (int j = jj; j < ((jj + b > P) ? P : jj + b); j++)
          {
            float sum = matrix_R[i * P + j];
            for (int k = kk; k < ((kk + b > N) ? N : kk + b); k++)
            {
              sum += matrix_A[i * N + k] * matrix_B[k * P + j];
            }
            matrix_R[i * P + j] = sum;
          }
        }
      }
    }
  }

  return NULL;
}
#pragma GCC pop_options
