/* main.c
 *
 * Author: Khalid Al-Hawaj
 * Date  : 12 Nov. 2023
 *
 * This file is structured to call different implementation of the same
 * algorithm/microbenchmark. The file will allocate 3 output arrays one
 * for: scalar naive impl, scalar opt impl, vectorized impl. As it stands
 * the file will allocate and initialize with random data one input array
 * of type 'byte'. To check correctness, the file allocate a 'ref' array;
 * to calculate this 'ref' array, the file will invoke a ref_impl, which
 * is supposed to be functionally correct and act as a reference for
 * the functionality. The file also adds a guard word at the end of the
 * output arrays to check for buffer overruns.
 *
 * The file will invoke each implementation n number of times. It will
 * record the runtime of _each_ invocation through the following Linux
 * API:
 *    clock_gettime(), with the clk_id set to CLOCK_MONOTONIC
 * Then, the file will calculate the standard deviation and calculate
 * an outlier-free average by excluding runtimes that are larger than
 * 2 standard deviation of the original average.
 */

/* Set features         */
#define _GNU_SOURCE

/* Standard C includes  */
/*  -> Standard Library */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
/*  -> Scheduling       */
#include <sched.h>
/*  -> Types            */
#include <stdbool.h>
#include <inttypes.h>
/*  -> Runtimes         */
#include <time.h>
#include <unistd.h>
#include <errno.h>

/* Include all implementations declarations */
#include "impl/ref.h"
#include "impl/naive.h"
#include "impl/opt.h"
#include "impl/vec.h"
#include "impl/para.h"

/* Include common headers */
#include "common/types.h"
#include "common/macros.h"

/* Include application-specific headers */
#include "include/types.h"

/* Max dataset dimensions for static allocation */
#define MAX_M 2500 // Maximum value for M which is for native dataset
#define MAX_N 3000 // Maximum value for N which is for native dataset
#define MAX_P 2100 // Maximum value for P which is for native dataset

/* Get dataset dimensions based on selection*/
void get_dataset_dims(const char *name, size_t *M, size_t *N, size_t *P)
{
  if (strcmp(name, "testing") == 0)
  {
    *M = 16;
    *N = 12;
    *P = 8;
  }
  else if (strcmp(name, "small") == 0)
  {
    *M = 121;
    *N = 180;
    *P = 115;
  }
  else if (strcmp(name, "medium") == 0)
  {
    *M = 550;
    *N = 620;
    *P = 480;
  }
  else if (strcmp(name, "large") == 0)
  {
    *M = 962;
    *N = 1012;
    *P = 1221;
  }
  else if (strcmp(name, "native") == 0)
  {
    *M = 2500;
    *N = 3000;
    *P = 2100;
  }
  else
  {
    printf("Unknown dataset name: %s\n", name);
    exit(1);
  }
}

/* Load binary matrix into 2D array */
void load_matrix_2D(const char* filename, float* matrix, size_t rows, size_t cols) {
  FILE* f = fopen(filename, "rb");
  if (!f) {
      printf("Error opening %s\n", filename);
      exit(1);
  }
  fread(matrix, sizeof(float), rows * cols, f);
  fclose(f);
}

// Compares two 2D matrices element-by-element with a tolerance threshold.
bool compare_2D(float A[MAX_M][MAX_P], float B[MAX_M][MAX_P], size_t M, size_t P, float delta)
{
  for (size_t i = 0; i < M; i++)
  {
    for (size_t j = 0; j < P; j++)
    {
      if (fabs(A[i][j] - B[i][j]) > delta)
      {
        printf("Mismatch at [%zu][%zu]: %f != %f\n", i, j, A[i][j], B[i][j]);
        return false;
      }
    }
  }
  return true;
}

//const int SIZE_DATA = 4 * 1024 * 1024;

int main(int argc, char **argv)
{
  /* Set the buffer for printf to NULL */
  setbuf(stdout, NULL);

  /* Arguments */
  int nthreads = 1;
  int cpu = 0;

  int nruns = 10000;
  int nstdevs = 3;

  /* Data */
  //int data_size = SIZE_DATA;

  /* Dataset */
  const char *dataset_str = "testing";

  /* Parse arguments */
  /* Function pointers */
  void *(*impl_scalar_naive_ptr)(void *args) = impl_scalar_naive;
  void *(*impl_scalar_opt_ptr)(void *args) = impl_scalar_opt;
  void *(*impl_vector_ptr)(void *args) = impl_vector;
  void *(*impl_parallel_ptr)(void *args) = impl_parallel;

  /* Chosen */
  void *(*impl)(void *args) = NULL;
  const char *impl_str = NULL;

  bool help = false;
  for (int i = 1; i < argc; i++)
  {
    /* Implementations */
    if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--impl") == 0)
    {
      assert(++i < argc);
      if (strcmp(argv[i], "naive") == 0)
      {
        impl = impl_scalar_naive_ptr;
        impl_str = "scalar_naive";
      }
      else if (strcmp(argv[i], "opt") == 0)
      {
        impl = impl_scalar_opt_ptr;
        impl_str = "scalar_opt";
      }
      else if (strcmp(argv[i], "vec") == 0)
      {
        impl = impl_vector_ptr;
        impl_str = "vectorized";
      }
      else if (strcmp(argv[i], "para") == 0)
      {
        impl = impl_parallel_ptr;
        impl_str = "parallelized";
      }
      else
      {
        impl = NULL;
        impl_str = "unknown";
      }

      continue;
    }

    /* Input/output data size */
    if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--size") == 0)
    {
      assert(++i < argc);
      //data_size = atoi(argv[i]);

      continue;
    }

    /* Run parameterization */
    if (strcmp(argv[i], "--nruns") == 0)
    {
      assert(++i < argc);
      nruns = atoi(argv[i]);

      continue;
    }

    if (strcmp(argv[i], "--nstdevs") == 0)
    {
      assert(++i < argc);
      nstdevs = atoi(argv[i]);

      continue;
    }

    /* Parallelization */
    if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--nthreads") == 0)
    {
      assert(++i < argc);
      nthreads = atoi(argv[i]);

      continue;
    }

    if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cpu") == 0)
    {
      assert(++i < argc);
      cpu = atoi(argv[i]);

      continue;
    }

    /* Help */
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
    {
      help = true;

      continue;
    }

    if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dataset") == 0)
    {
      dataset_str = argv[++i];
    }
  }

  if (help || impl == NULL)
  {
    if (!help)
    {
      if (impl_str != NULL)
      {
        printf("\n");
        printf("ERROR: Unknown \"%s\" implementation.\n", impl_str);
      }
      else
      {
        printf("\n");
        printf("ERROR: No implementation was chosen.\n");
      }
    }
    printf("\n");
    printf("Usage:\n");
    printf("  %s {-i | --impl} impl_str [Options]\n", argv[0]);
    printf("  \n");
    printf("  Required:\n");
    printf("    -i | --impl      Available implementations = {naive, opt, vec, para}\n");
    printf("    \n");
    printf("  Options:\n");
    printf("    -h | --help      Print this message\n");
    printf("    -d | --dataset   Select dataset from: testing, small, medium, large, native\n (default = %s)\n", dataset_str);
    printf("    -n | --nthreads  Set number of threads available (default = %d)\n", nthreads);
    printf("    -c | --cpu       Set the main CPU for the program (default = %d)\n", cpu);
    //printf("    -s | --size      Size of input and output data (default = %d)\n", data_size);
    printf("         --nruns     Number of runs to the implementation (default = %d)\n", nruns);
    printf("         --stdevs    Number of standard deviation to exclude outliers (default = %d)\n", nstdevs);
    printf("\n");

    exit(help ? 0 : 1);
  }

  /* Set our priority the highest */
  int nice_level = -20;

  printf("Setting up schedulers and affinity:\n");
  printf("  * Setting the niceness level:\n");
  do
  {
    errno = 0;
    printf("      -> trying niceness level = %d\n", nice_level);
    int __attribute__((unused)) ret = nice(nice_level);
  } while (errno != 0 && nice_level++);

  printf("    + Process has niceness level = %d\n", nice_level);

  /* If we are on an apple operating system, skip the scheduling  *
   * routine; Darwin does not support sched_set* system calls ... *
   *                                                              *
   * hawajkm: and here I was--thinking that MacOS is POSIX ...    *
   *          Silly me!                                           */
#if !defined(__APPLE__)
  /* Set scheduling to reduce context switching */
  /*    -> Set scheduling scheme                */
  printf("  * Setting up FIFO scheduling scheme and high priority ... ");
  pid_t pid = 0;
  int policy = SCHED_FIFO;
  struct sched_param param;

  param.sched_priority = sched_get_priority_max(policy);
  int res = sched_setscheduler(pid, policy, &param);
  if (res != 0)
  {
    printf("Failed\n");
  }
  else
  {
    printf("Succeeded\n");
  }

  /*    -> Set affinity                         */
  printf("  * Setting up scheduling affinity ... ");
  cpu_set_t cpumask;

  CPU_ZERO(&cpumask);
  for (int i = 0; i < nthreads; i++)
  {
    CPU_SET(cpu + i, &cpumask);
  }

  res = sched_setaffinity(pid, sizeof(cpumask), &cpumask);

  if (res != 0)
  {
    printf("Failed\n");
  }
  else
  {
    printf("Succeeded\n");
  }
#endif
  printf("\n");

  /* Statistics */
  __DECLARE_STATS(nruns, nstdevs);

  /* Initialize Rand */
  srand(0xdeadbeef);

  // Matrix rows and columns
  size_t M, N, P;

  // Statically allocated matrices
  static float matrix_A[MAX_M][MAX_N];
  static float matrix_B[MAX_N][MAX_P];
  static float matrix_R[MAX_M][MAX_P];
  static float matrix_R_ref[MAX_M][MAX_P];

  /* Datasets */
  /* Allocation and initialization */

  // Load data
  char fname[64];
  snprintf(fname, sizeof(fname), "src/dataset/A_%s.bin", dataset_str);
  load_matrix_2D(fname, matrix_A, M, N);

  snprintf(fname, sizeof(fname), "src/dataset/B_%s.bin", dataset_str);
  load_matrix_2D(fname, (float(*)[MAX_N])matrix_B, N, P); // cast to match expected type

  snprintf(fname, sizeof(fname), "src/dataset/R_%s_gold.bin", dataset_str);
  load_matrix_2D(fname, matrix_R_ref, M, P);

  /* Setting a guards, which is 0xdeadcafe.
     The guard should not change or be touched. */
  // __SET_GUARD(ref, data_size);
  // __SET_GUARD(dest, data_size);

  // Set guard on output buffer
  __SET_GUARD(matrix_R, M * P * sizeof(float));

  /* Generate ref data */
  /* Arguments for the functions */
  args_t args_ref = {
      .input = (byte *)matrix_A,
      .output = (byte *)matrix_R,
      .size_m = M,
      .size_n = N,
      .size_p = P,
      .cpu = cpu,
      .nthreads = nthreads};


  /* Start execution */
  printf("Running '%s' on dataset '%s'...\n", impl_str, dataset_str);

  printf("  * Invoking the implementation %d times .... ", num_runs);
  for (int i = 0; i < num_runs; i++)
  {
    __SET_START_TIME();
    for (int j = 0; j < 16; j++)
    {
      (*impl)(&args_ref);
    }
    __SET_END_TIME();
    runtimes[i] = __CALC_RUNTIME() / 16;
  }
  printf("Finished\n");

  /* Verfication */
  printf("  * Verifying results .... ");
  // bool match = __CHECK_MATCH(ref, dest, data_size);
  // bool guard = __CHECK_GUARD(dest, data_size);

  bool match = compare_2D(matrix_R, matrix_R_ref, M, P, 1e-3);
  bool guard = __CHECK_GUARD(matrix_R, M * P * sizeof(float));
  if (match && guard)
  {
    printf("Success\n");
  }
  else if (!match && guard)
  {
    printf("Fail, but no buffer overruns\n");
  }
  else if (match && !guard)
  {
    printf("Success, but failed buffer overruns check\n");
  }
  else if (!match && !guard)
  {
    printf("Failed, and failed buffer overruns check\n");
  }

  /* Running analytics */
  uint64_t min = -1;
  uint64_t max = 0;

  uint64_t avg = 0;
  uint64_t avg_n = 0;

  uint64_t std = 0;
  uint64_t std_n = 0;

  int n_msked = 0;
  int n_stats = 0;

  for (int i = 0; i < num_runs; i++)
    runtimes_mask[i] = true;

  printf("  * Running statistics:\n");
  do
  {
    n_stats++;
    printf("    + Starting statistics run number #%d:\n", n_stats);
    avg_n = 0;
    avg = 0;

    /*   -> Calculate min, max, and avg */
    for (int i = 0; i < num_runs; i++)
    {
      if (runtimes_mask[i])
      {
        if (runtimes[i] < min)
        {
          min = runtimes[i];
        }
        if (runtimes[i] > max)
        {
          max = runtimes[i];
        }
        avg += runtimes[i];
        avg_n += 1;
      }
    }
    avg = avg / avg_n;

    /*   -> Calculate standard deviation */
    std = 0;
    std_n = 0;

    for (int i = 0; i < num_runs; i++)
    {
      if (runtimes_mask[i])
      {
        std += ((runtimes[i] - avg) *
                (runtimes[i] - avg));
        std_n += 1;
      }
    }
    std = sqrt(std / std_n);

    /*   -> Calculate outlier-free average (mean) */
    n_msked = 0;
    for (int i = 0; i < num_runs; i++)
    {
      if (runtimes_mask[i])
      {
        if (runtimes[i] > avg)
        {
          if ((runtimes[i] - avg) > (nstd * std))
          {
            runtimes_mask[i] = false;
            n_msked += 1;
          }
        }
        else
        {
          if ((avg - runtimes[i]) > (nstd * std))
          {
            runtimes_mask[i] = false;
            n_msked += 1;
          }
        }
      }
    }

    printf("      - Standard deviation = %" PRIu64 "\n", std);
    printf("      - Average = %" PRIu64 "\n", avg);
    printf("      - Number of active elements = %" PRIu64 "\n", avg_n);
    printf("      - Number of masked-off = %d\n", n_msked);
  } while (n_msked > 0);
  /* Display information */
  printf("  * Runtimes (%s): ", __PRINT_MATCH(match));
  printf(" %" PRIu64 " ns\n", avg);

  /* Dump */
  printf("  * Dumping runtime informations:\n");
  FILE *fp;
  char filename[256];
  strcpy(filename, impl_str);
  strcat(filename, "_runtimes.csv");
  printf("    - Filename: %s\n", filename);
  printf("    - Opening file .... ");
  fp = fopen(filename, "w");

  if (fp != NULL)
  {
    printf("Succeeded\n");
    printf("    - Writing runtimes ... ");
    fprintf(fp, "impl,%s", impl_str);

    fprintf(fp, "\n");
    fprintf(fp, "num_of_runs,%d", num_runs);

    fprintf(fp, "\n");
    fprintf(fp, "runtimes");
    for (int i = 0; i < num_runs; i++)
    {
      fprintf(fp, ", ");
      fprintf(fp, "%" PRIu64 "", runtimes[i]);
    }

    fprintf(fp, "\n");
    fprintf(fp, "avg,%" PRIu64 "", avg);
    printf("Finished\n");
    printf("    - Closing file handle .... ");
    fclose(fp);
    printf("Finished\n");
  }
  else
  {
    printf("Failed\n");
  }
  printf("\n");


  /* Finished with statistics */
  __DESTROY_STATS();

  /* Done */
  return 0;
}
