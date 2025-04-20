/* naive.c
 *
 * Author:
 * Date  :
 *
 *  Description
 */

/* Standard C includes */
#include <stdlib.h>
#include <stdio.h>
/* Include common headers */
#include "common/macros.h"
#include "common/types.h"

/* Include application-specific headers */
#include "include/types.h"

/* Naive Implementation */
#pragma GCC push_options
#pragma GCC optimize("O1")
void *impl_scalar_naive(void *args)
{
  /* Get the argument struct */
  args_t *parsed_args = (args_t *)args;

  /* Get all the arguments */
  register size_t M = parsed_args->size_m;
  register size_t N = parsed_args->size_n;
  register size_t P = parsed_args->size_p;

  // Cast buffers to 2D float arrays
  register const float *matrix_A = (const float *)parsed_args->matrix_A;
  register const float *matrix_B = (const float *)parsed_args->matrix_B;
  register float *matrix_R = (float *)parsed_args->matrix_R;

  // Compute R = A × B
  for (register size_t i = 0; i < M; i++)
  {
    for (register size_t j = 0; j < P; j++)
    {
      register float sum = 0.0f;
      for (register size_t k = 0; k < N; k++)
      {
        sum += matrix_A[i * N + k] * matrix_B[k * P + j];
      }
      matrix_R[i * P + j] = sum;
    }
  }
  // printf("Multiplication Finished\n");
  return NULL;
}
#pragma GCC pop_options
