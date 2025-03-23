/* types.h
 *
 * Author: Khalid Al-Hawaj
 * Date  : 13 Nov. 2023
 *
 * This file contains all required types decalartions.
 */

 #ifndef __INCLUDE_TYPES_H_
 #define __INCLUDE_TYPES_H_
 
 typedef struct {
   byte*   input; // A followed by B
   byte*   output; // R
 
   size_t size_m; // M number of rows in Matrix A and R
   size_t size_n; // N number of rows in Matrix B and N columns in matrix A
   size_t size_p; // P number of columns in Matrix B and R
   int     cpu;
   int     nthreads;
 } args_t;
 #endif //__INCLUDE_TYPES_H_