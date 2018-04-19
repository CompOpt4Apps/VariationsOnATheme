/*****************************************************************************
 * utilTest.h
 *
 * This file contains validation, memory allocation and initalization
 *  functions:
 *  bool verifyResultJacobi2D(real*,Configuration&);
 *  bool verifyResultJacobi2DCuda(real*,Configuration&);
 *  bool allocateSpace(real**,int);
 *  void initializeSpace(real*,Configuration&);
 * 
 * Example usage of functions:
 *  
 *  verifyResultJacobi2D(pointerToData,configuration);
 *   if verification is successful 
 *     returns true
 *   else
 *     returns false
 * 
 *  verifyResultJacobi2DCuda(pointerToData,configuration);
 *   if	verification is successful
 *     returns true
 *   else
 *     returns false
 *
 *  allocateSpace(&pointerTodata,configuration);
 *   if allocation is successful
 *     returns true
 *   else
 *     returns false 
 *
 *  initializeSpace(pointerTodata,configuration);
 *
 *  checkCommandLineOptions(configuration);
 *  if checking is successful
 *    return true
 *  else
 *    return false
 *
*****************************************************************************/
 
#include <stdio.h>
#include <omp.h>
#include "../common/Configuration.h"

/*****************************************************************************
 * This macro is used for selecting data type in benchmarks
 *
 *
*****************************************************************************/
#ifdef DATATYPEDOUBLE
   typedef double real;
#else
   typedef float real;
#endif

/*****************************************************************************
 * Function headers
 * See the utilTest.cpp for the functions
 *
*****************************************************************************/
bool verifyResultJacobi2D(real** result, Configuration& configuration);
bool verifyResultJacobi2DCuda(real* result, Configuration& configuration);
bool allocateSpace(real*** data, Configuration& configuration);
void initializeSpace(real*** data, Configuration& configuration);
bool checkCommandLineOptions(Configuration& configuration);
/*****************************************************************************
 *
 * Macros are used for calculations
 *
 *
*****************************************************************************/
#define space(t,i,j)  data[( (t) & 1 )][ (i) ][ (j) ]
#define stencil(t,i,j) space(t,i,j) = (  space(t-1, i-1, j  )   + \
                                         space(t-1, i  , j  )   + \
                                         space(t-1, i+1, j  )   + \
                                         space(t-1, i  , j+1)   + \
                                         space(t-1, i  , j-1) ) * 0.2
//#define intDiv_(i,j)  ((((i)%(j))>=0) ? ((i)/(j)) : (((i)/(j)) -1))
//#define intMod_(i,j)  ((((i)%(j))>=0) ? ((i)%(j)) : (((i)%(j)) +j))
//#define ceild(n, d) intDiv_((n), (d)) + ((intMod_((n),(d))>0)?1:0)
//#define floord(n, d)  intDiv_((n), (d))

/*****************************************************************************
 *   The instrumentation macros are for CUDA benchmarks shown
 *   below:
 *
 * COUNT_MACRO_INIT()
 * COUNT_MACRO_KERNEL_CALL()
 * COUNT_MACRO_RESOURCE_ITERS(x)
 * COUNT_MACRO_BYTES_FROM_GLOBAL(x)
 * COUNT_MACRO_BYTES_TO_GLOBAL(x) 
 * COUNT_MACRO_NUMTILES(x)
 * COUNT_MACRO_UNIQUE_WAVEFRONT_SIZE(x) 
 * COUNT_MACRO_PRINT() 
*****************************************************************************/
#ifdef COUNT
#define COUNT_MACRO_INIT() unsigned long long int resourceIters = 0,     \
bytesFromGlobal = 0, bytesToGlobal = 0, countTiles = 0;                  \
int kernelCalls = 0, uniqueWavefronts[10]={0};                                                    
#define COUNT_MACRO_KERNEL_CALL() kernelCalls +=1
#define COUNT_MACRO_RESOURCE_ITERS(x) resourceIters +=                   \
((unsigned long long int)x)
#define COUNT_MACRO_BYTES_FROM_GLOBAL(x) bytesFromGlobal +=              \
((unsigned long long int)x)
#define COUNT_MACRO_BYTES_TO_GLOBAL(x) bytesToGlobal +=                  \
((unsigned long long int)x)
#define COUNT_MACRO_NUMTILES(x) countTiles += ((unsigned long long int)x)
#define COUNT_MACRO_UNIQUE_WAVEFRONT_SIZE(x)                             \
do {char flag = false;                                                   \
 for (int i = 0; i < 10;  i++ ){                                         \
   if(x == uniqueWavefronts[i]){                                         \
     flag = true;                                                        \
   }                                                                     \
 }                                                                       \
  if(flag == false){                                                     \
    for (int i = 0; i < 10;  i++ ){                                      \
      if(uniqueWavefronts[i] == 0){                                      \
        uniqueWavefronts[i] = x;                                         \
        break;                                                           \
      }                                                                  \
    }                                                                    \
  }                                                                      \
}while(0);                                                
#define COUNT_MACRO_PRINT()                                              \
  printf("KernelCalls:%d,ResourceIters:%llu,bytesFromGlobal:%llu,\
bytesToGlobal:%llu,countTiles:%llu,",kernelCalls,resourceIters,          \
bytesFromGlobal,bytesToGlobal,countTiles);                               \
do {                                                                     \
int counter = 0;                                                         \
printf("uniqueWavefronts:[");                                            \
while(uniqueWavefronts[counter]!=0){                                     \
  printf("%d",uniqueWavefronts[counter]);                                \
  if(uniqueWavefronts[counter+1]!=0){                                    \
    printf(",");                                                         \
  }                                                                      \
  counter++;                                                             \
}                                                                        \
printf("]");                                                             \
}while(0)                                                        

#else 
#define COUNT_MACRO_INIT() 
#define COUNT_MACRO_KERNEL_CALL()
#define COUNT_MACRO_RESOURCE_ITERS(x)
#define COUNT_MACRO_NUMTILES(x) 
#define COUNT_MACRO_BYTES_FROM_GLOBAL(x) 
#define COUNT_MACRO_BYTES_TO_GLOBAL(x)
#define COUNT_MACRO_NUMTILES(x)
#define COUNT_MACRO_UNIQUE_WAVEFRONT_SIZE(x)
#define COUNT_MACRO_PRINT() 
#endif

// Important defines (imported from generated_code.cpp).
#define max(x,y)  (((x)>(y))?(x):(y))
#define min(x,y)  (((x)<(y))?(x):(y))
#if ASSUME_POSITIVE_INTMOD
#define intDiv(x,y)	(eassert(((x)%(y)) >= 0), ((x)/(y)))
#define intMod(x,y)	(eassert(((x)%(y)) >= 0), ((x)%(y)))
#else
#define intDiv_(x,y)  ((((x)%(y))>=0) ? ((x)/(y)) : (((x)/(y)) -1))
#define intMod_(x,y)  ((((x)%(y))>=0) ? ((x)%(y)) : (((x)%(y)) +y))
#define checkIntDiv(x,y) (eassert((y) > 0 && intMod_((x),(y)) >= 0 && intMod_((x),(y)) <= (y) && x==((y)*intDiv_((x),(y)) + intMod_((x),(y)))))
#define intDiv(x,y)	(checkIntDiv((x),(y)), intDiv_((x),(y)))
#define intMod(x,y)	(checkIntDiv((x),(y)), intMod_((x),(y)))
#endif
#define floord(n, d)  intDiv_((n), (d))
