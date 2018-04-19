/******************************************************************************
 * util.h
 *
 * Is included in the .test.c drivers.
 * Search for "default" to find where the default values for command-line options
 * are set.
 *
 * Also includes the various max, min, and floord definitions we need for iscc-generated code.
 ******************************************************************************/
// This is for instrumentation of CUDA benchmarks.
#include <string.h>

#ifdef COUNT
#define COUNT_MACRO_INIT() unsigned long long int resourceIters = 0,   \
bytesFromGlobal = 0, bytesToGlobal = 0, countTiles = 0;                \
int kernelCalls = 0, uniqueWavefronts[10]={0};                                                    
#define COUNT_MACRO_KERNEL_CALL() kernelCalls +=1
#define COUNT_MACRO_RESOURCE_ITERS(x) resourceIters += ((unsigned long long int)x)
#define COUNT_MACRO_BYTES_FROM_GLOBAL(x) bytesFromGlobal += ((unsigned long long int)x)
#define COUNT_MACRO_BYTES_TO_GLOBAL(x) bytesToGlobal += ((unsigned long long int)x)
#define COUNT_MACRO_NUMTILES(x) countTiles += ((unsigned long long int)x)
#define COUNT_MACRO_UNIQUE_WAVEFRONT_SIZE(x)                           \
do {char flag = false;                                                 \
 for (int i = 0; i < 10;  i++ ){                                       \
   if(x == uniqueWavefronts[i]){                                       \
     flag = true;                                                      \
   }                                                                   \
 }                                                                     \
  if(flag == false){                                                   \
    for (int i = 0; i < 10;  i++ ){                                    \
      if(uniqueWavefronts[i] == 0){                                    \
        uniqueWavefronts[i] = x;                                       \
        break;                                                         \
      }                                                                \
    }                                                                  \
  }                                                                    \
}while(0);                                                
#define COUNT_MACRO_PRINT()                                            \
  printf(",KernelCalls:%d,ResourceIters:%llu,bytesFromGlobal:%llu,\
bytesToGlobal:%llu,countTiles:%llu,",kernelCalls,resourceIters,        \
bytesFromGlobal,bytesToGlobal,countTiles);                             \
do {                                                                   \
int counter = 0;                                                       \
printf("uniqueWavefronts:[");                                          \
while(uniqueWavefronts[counter]!=0){                                   \
  printf("%d",uniqueWavefronts[counter]);                              \
  if(uniqueWavefronts[counter+1]!=0){                                  \
    printf(",");                                                       \
  }                                                                    \
  counter++;                                                           \
}                                                                      \
printf("]");                                                           \
printf("\n");                                                          \
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

//==== Needed for generated and modified iscc code.
#define min(a,b)	((a)>(b)?(b):(a))
#define max(a,b)	((a)>(b)?(a):(b))

#if defined NDEBUG
#define eassert(X)	0
#else
static inline int eassert_function(int fact, int line, char *fact_text)
{
  if (!fact)
    {
      fprintf(stderr, "assertion failed, line %d: %s\n", line, fact_text);
      exit(1);
    }
  else
    return 0;
}
#define eassert(X)	eassert_function(X, __LINE__, "X")
#endif

// FIXME: Dave W, what is going on with ASSUME_POSITIVE_INTMOD?
#if ! defined ASSUME_POSITIVE_INTMOD
#define ASSUME_POSITIVE_INTMOD 0
#endif

#if ASSUME_POSITIVE_INTMOD
#define intDiv(x,y)	(eassert(((x)%(y)) >= 0), ((x)/(y)))
#define intMod(x,y)	(eassert(((x)%(y)) >= 0), ((x)%(y)))
#else
#define intDiv_(x,y)	((((x)%(y))>=0) ? ((x)/(y)) : (((x)/(y)) -1))
#define intMod_(x,y)	((((x)%(y))>=0) ? ((x)%(y)) : (((x)%(y)) +y))
#define checkIntDiv(x,y) (eassert((y) > 0 && intMod_((x),(y)) >= 0 && intMod_((x),(y)) <= (y) && x==((y)*intDiv_((x),(y)) + intMod_((x),(y)))))
#define intDiv(x,y)	(checkIntDiv((x),(y)), intDiv_((x),(y)))
#define intMod(x,y)	(checkIntDiv((x),(y)), intMod_((x),(y)))
#endif

#if defined CUDA
#undef intDiv
#undef intMod
//#define intDiv(x,y)	((((x)%(y))>=0) ? ((x)/(y)) : (((x)/(y)) -1))
//#define intMod(x,y)	((((x)%(y))>=0) ? ((x)%(y)) : (((x)%(y)) +y))
#define intDiv(x,y)	( ((x)/(y)))
#define intMod(x,y)	( ((x)%(y)))
#endif

#define ceild(n, d)	intDiv_((n), (d)) + ((intMod_((n),(d))>0)?1:0)
#define floord(n, d)	intDiv_((n), (d))



//==== parse int abstraction from strtol
 int parseInt( char* string ){
   return (int) strtol( string, NULL, 10 );
  }

//==== verification
// All that verification does is to run the exact same 
// computation in a serial manner.
// Therefore, all we are checking is that our we did not break it when
// going parallel
 bool verifyResultJacobi1D(double* result,int problemSize,int seed,int steps){

  // run serial Jacobi 1D
  int  lowerBound = 1;
  int  upperBound = lowerBound + problemSize - 1;
  int x;
  int t, read = 0, write = 1;
  double* space[2];
  bool failed = false;

  space[0] = (double*) malloc( (problemSize + 2) * sizeof(double));
  space[1] = (double*) malloc( (problemSize + 2) * sizeof(double));
  if( space[0] == NULL || space[1] == NULL ){
    printf( "Could not allocate space array\n" );
    exit(-1);
  }

  // seed the space. 
  srand(seed);
  for( x = lowerBound; x <= upperBound; ++x ){
    space[0][x] = rand() / (double)rand();
  }

  // set halo values (sanity)
  space[0][0] = 0;
  space[0][upperBound+1] = 0;
  space[1][0] = 0;
  space[1][upperBound+1] = 0;


  for( t = 1; t <= steps; ++t ){
    for( x = lowerBound; x <= upperBound; ++x ){
      space[write][x] = (space[read][x-1] +
                         space[read][x] +
                         space[read][x+1])/3;
    }
    read = write;
    write = 1 - write;
  }


  for( x = lowerBound; x <= upperBound; ++x ){
    if( space[read][x] != result[x] ){
      fprintf(stderr,"Position: %d, values: expected %f, found %f\n",
                      x,space[read][x],result[x]);
      failed = true;
      break;
    }
  }
  return !failed;
}

bool verifyResultJacobi1DCuda(float* result,int problemSize,int seed,int steps){

  // run serial Jacobi 1D
  int  lowerBound = 1;
  int  upperBound = lowerBound + problemSize - 1;
  int x;
  int t, read = 0, write = 1;
  float* space[2];
  bool failed = false;
  float epsilon=0.000000;

  space[0] = (float*) malloc( (problemSize + 2) * sizeof(float));
  space[1] = (float*) malloc( (problemSize + 2) * sizeof(float));
  if( space[0] == NULL || space[1] == NULL ){
    printf( "Could not allocate space array\n" );
    exit(-1);
  }

  // seed the space. 
  srand(seed);
  for( x = lowerBound; x <= upperBound; ++x ){
    space[0][x] = rand() / (float)rand();
  }

  // set halo values (sanity)
  space[0][0] = 0.;
  space[0][upperBound+1] = 0.;
  space[1][0] = 0.;
  space[1][upperBound+1] = 0.;


  for( t = 1; t <= steps; ++t ){
    for( x = lowerBound; x <= upperBound; ++x ){
      space[write][x] = (space[read][x-1] +
                         space[read][x] +
                         space[read][x+1])/3;
    }
    read = write;
    write = 1 - write;
  }


  for( x = 0; x <= upperBound; ++x ){
    if( ! (result[x]-epsilon <= space[read][x] 
        && space[read][x] <= result[x]+epsilon)){
      fprintf(stderr,"Position: %d, values: expected %f, found %f\n",
                      x,space[read][x],result[x]);
      failed = true;
      break;
    }
  }
  return !failed;
}

/******************************************************************************
 * Command Line Parsing
 *
 * Each benchmark has its own command line parsing because the
 * options vary slightly depending on the tiling approach used.
 ******************************************************************************/
typedef struct {
  // common parameters
  int copies;
  int blockSize;
  int T; // time step
  int globalSeed;
  int cores;
  int problemSize;
  bool verify;
  bool printtime;
  //==== needed for tiling (ignored by some)
  int timeBand; // time band
  int width_max;

  // diamond tiling approaches derived from iscc
  // Height (i.e., number of slices) in diamond slab.
  // Equivalent to timeBand, but not using that because error checking
  // is related to tau and different.
  // See Jacobi2D-DiamondSlabISCCParam-OMP.test.c for error checking.
  int subset_s;
  // width between tiling hyperplanes, only use command-line param
  // if tau was not set at compile time
  // See Jacobi2D-DiamondSlabISCCParam-OMP.test.c after call to
  // parseCmdLineArgs to see an example of how this works.
  int tau_runtime;

  // this is being used for the naive space tiling
  int tile_len_x;
  int tile_len_y;

} Params;
int parseCmdLineArgs(Params *cmdLineArgs, int argc, char* argv[]){

  int s;
  // set default values
  cmdLineArgs->copies = 1;
  cmdLineArgs->blockSize = 0;
  cmdLineArgs->T = 100;
  cmdLineArgs->globalSeed = 1524; 
  //cmdLineArgs->cores = omp_thread_count();
#ifdef OPENMP
  cmdLineArgs->cores = omp_get_max_threads();
#else
  cmdLineArgs->cores = 1;
#endif
  cmdLineArgs->problemSize = 100;
  cmdLineArgs->verify = false;
  cmdLineArgs->printtime = true;
  cmdLineArgs->timeBand = 10;
  cmdLineArgs->width_max = 54701;
  // diamond tile size, can be any multiple of 3 >=15
  cmdLineArgs->tau_runtime = 30;    
  cmdLineArgs->subset_s = -1;
  // this is a random number, so don't ask me why I chose it
  cmdLineArgs->tile_len_x = 128; 
  cmdLineArgs->tile_len_y = 128; 

  // process incoming
  char c;
  while ((c = getopt (argc, argv, "C:b:nc:s:p:T:x:y:a:w:t:hv")) != -1){
    switch( c ) {
    case 'C': // The number of copy
      cmdLineArgs->copies = parseInt( optarg );
      if(cmdLineArgs->copies <= 0){
        fprintf(stderr, "The number of copies from the global to shared and shared to global"
            "must be greater than 0: %d\n",
                         cmdLineArgs->copies);
          exit( -1 );
      }
      break;

      case 'b': // block size
        cmdLineArgs->blockSize = parseInt( optarg );
        if(cmdLineArgs->blockSize <= 0){
          fprintf(stderr, "blockSize must be greater than 0: %d\n",
                           cmdLineArgs->blockSize);
            exit( -1 );
        }
        break;

      case 'n': // print time
        cmdLineArgs->printtime = false;
        break;

      case 'c': // cores
        cmdLineArgs->cores = parseInt( optarg );
        if(cmdLineArgs->cores <= 0){
          fprintf(stderr, "cores must be greater than 0: %d\n", 
                           cmdLineArgs->cores);
           exit( -1 );
        }
        break;
                
      case 'p': // problem size
        cmdLineArgs->problemSize = parseInt( optarg );
        if( cmdLineArgs->problemSize <= 0 ){
          fprintf(stderr, "problemSize must be greater than 0: %d\n",
                           cmdLineArgs->problemSize);
          exit( -1 );
        }
        break;
                
      case 'T': // T (time steps)
        cmdLineArgs->T = parseInt( optarg );
        if( cmdLineArgs->T <= 0 ){
          fprintf(stderr, "T must be greater than 0: %d\n", cmdLineArgs->T);
          exit( -1 );
        }
        break;

      case 't': // timeBand
         cmdLineArgs->timeBand = parseInt( optarg );
          if( cmdLineArgs->timeBand <= 0 ){
            fprintf(stderr, "t must be greater than 0: %d\n",
                         cmdLineArgs->timeBand);
            exit( -1 );
          }
          break;

      case 'w': // width
        cmdLineArgs->width_max = parseInt( optarg );
        if( cmdLineArgs->width_max <= 0 ){
          fprintf(stderr, "w must be greater than 0: %d\n",
                               cmdLineArgs->width_max);
           exit( -1 );
        }
        break;

      case 'a': // tau_runtime
        cmdLineArgs->tau_runtime = parseInt( optarg );
        if( cmdLineArgs->tau_runtime <= 1 ){
          fprintf(stderr,
                       "-a tau must be greater than 1:"
                       " current value is %d\n",
                       cmdLineArgs->tau_runtime);
          exit( -1 );
         }
         break;

      case 'x': // tile_len
        cmdLineArgs->tile_len_x = parseInt( optarg );
        if( cmdLineArgs->tile_len_x < 1){
          fprintf(stderr, "-x: must be >= 1\n");
              exit(-1);
         }
         break;

      case 'y': // tile_len
        cmdLineArgs->tile_len_y = parseInt( optarg );
        if( cmdLineArgs->tile_len_y < 1){
          fprintf(stderr, "-y: must be >= 1\n");
              exit(-1);
        }
        break;

      // Have to check in driver as well.
      // See Jacobi2D-DiamondSlabISCCParam-OMP.test.c for same check.
      case 's': // subset_s
        cmdLineArgs->subset_s = parseInt( optarg );
        // Note: you can't do the error checking here.
        // the value of tau may come after s --> in that case
        // this is not valid
        // s is the number of non-pointy bit 2D slices of diamond tiling
        // that is available for the current tile size.
        // subset_s is an input parameter indicating how many of those
        // slices we want to use in the repeated tiling pattern.
        // subset_s should be less than s and greater than or equal to 2.
        //s = (cmdLineArgs->tau_runtime/3) - 2;
          //if (cmdLineArgs->subset_s > s  || cmdLineArgs->subset_s<2) {
            //fprintf(stderr, "-s: need 2<=subset_s<=%d\n",s);
              //exit(-1);
          //}
          break;

      case 'h': // help
        printf("usage: %s\n"
                  "-n dont print time \n"
                  "-p <problem size>, problem size in elements (N) \n"
                  "-T <time steps>, number of time steps\n"
                  "-c <cores>, number of threads\n"
                  "-w <max tile width>, max tile width for Jacobi1D-DiamondSlabByHand\n"
                  "-a <tau>, distance between tiling hyperplanes (all diamond(slab) but ByHand)\n"
                  "-s <subset_s>, number of slices in slab (all diamondslab but ByHand)\n"
                  "-h usage help, this dialogue\n"
                  "-v verify output\n", argv[0]);
           exit(0);
            
      case 'v': // verify;
         cmdLineArgs->verify = true;
         break;
            
      case '?':
         if (optopt == 'p'){
            fprintf (stderr,
                   "Option -%c requires positive int argument: problem size.\n",
                   optopt);
          }else if (optopt == 'T'){
            fprintf (stderr,
                     "Option -%c requires positive int argument: T.\n",
                      optopt);
          }else if (optopt == 's'){
            fprintf (stderr,
                     "Option -%c requires int argument: subset_s.\n",
                      optopt);
          }else if (optopt == 'c'){
            fprintf (stderr,
                "Option -%c requires int argument: number of cores.\n",
                 optopt);
          //}else if (isprint (optopt)){
                    //fprintf (stderr, "Unknown option `-%c'.\n", optopt);
          }else{
            fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
          }
          exit(-1);
               
      default:
         exit(0);
    }
  } 
  if( cmdLineArgs->blockSize > 0){
    cmdLineArgs->tau_runtime = cmdLineArgs->blockSize-2;
  }
  if(cmdLineArgs->subset_s > 0){
    int s2;
    const char* Jacobi1D = "Jacobi1D";
    const char* Jacobi2D = "Jacobi2D";
    if(strstr(argv[0],Jacobi1D) != NULL){
      s = (cmdLineArgs->tau_runtime/2) - 2;
      s2 = 2;
    }else if(strstr(argv[0],Jacobi2D) != NULL){
      s = (cmdLineArgs->tau_runtime/3) - 2;
      s2 = 2;
    }
    const char* Over = "Overlapped";
    if(strstr(argv[0],Over) != NULL){
      s = cmdLineArgs->T+1;
      s2 = 0;
    }

    if(cmdLineArgs->subset_s > s || cmdLineArgs->subset_s < s2){
      fprintf(stderr, "-s: need %d<=subset_s<=%d\n",s2,s);
        exit(-1);    
    }
  }

#ifdef OPENMP
  omp_set_num_threads( cmdLineArgs->cores );
#endif

  return 0;
}

#define space_h(ptr,x,y) (ptr)[(x)*(problemSize+2)+(y)]
bool verifyResultJacobi2DCuda( float* result,int problemSize,
                               int seed,int steps){

  int i;
  int lowerBound = 1;
  int upperBound = lowerBound + problemSize - 1;

  // allocate and initialize the space
  float** space[2];
  // allocate x axis
  space[0] = (float**) malloc( (problemSize + 2) * sizeof(float*));
  space[1] = (float**) malloc( (problemSize + 2) * sizeof(float*));
  if( space[0] == NULL || space[1] == NULL ){
    printf( "Could not allocate x axis of space array\n" );
    exit(0);
  }

  // allocate y axis
  for( i = 0; i < problemSize + 2; ++i ){
    space[0][i] = (float*) malloc( (problemSize + 2) * sizeof(float));
    space[1][i] = (float*) malloc( (problemSize + 2) * sizeof(float));
    if( space[0][i] == NULL || space[1][i] == NULL ){
      printf( "Could not allocate y axis of space array\n" );
      exit(0);
    }
  }

  // use global seed to seed the random number gen (will be constant)
  srand(seed);
  // seed the space.
  int x, y;
  for( x = lowerBound; x <= upperBound; ++x ){
    for( y = lowerBound; y <= upperBound; ++y ){
      space[0][x][y] = rand() / (float)rand();
    }
  }

  // set halo values (sanity)
  for( i = 0; i < problemSize + 2; ++i){
    space[0][i][0] = 0;
    space[1][i][0] = 0;
    space[0][i][problemSize + 1] = 0;
    space[1][i][problemSize + 1] = 0;

    space[0][0][i] = 0;
    space[1][0][i] = 0;
    space[0][problemSize + 1][i] = 0;
    space[1][problemSize + 1][i] = 0;
  }


  int t,read = 0, write = 1;
  for( t = 1; t <= steps; ++t ){
    for( x = lowerBound; x <= upperBound; ++x ){
      for( y = lowerBound; y <= upperBound; ++y ){
        space[write][x][y] = ( space[read][x-1][y] +
                               space[read][x][y] +
                               space[read][x+1][y] +
                               space[read][x][y+1] +
                               space[read][x][y-1] )*0.2;
      }
    }
    read = write;
    write = 1 - write;
  }
 printf("\n");
  bool failed = false;
  for( x = lowerBound; x <= upperBound; ++x ){
    for( y = lowerBound; y <= upperBound; ++y ){
      if( space_h(result,x,y) != space[ steps & 1 ][x][y] ){
      fprintf(stderr,"Position: %d,%d, values:(expected)%a, %a\n",
              x,y,space[steps & 1][x][y],space_h(result,x,y));
        failed = true;
        break;
      }
    }
    if( failed ) break;
  }

  return !failed;

}

int verifyResultJacobi2DTiled(double** result,int problemSize, int seed,int steps,
                          int x_tile_count, int y_tile_count){

  int i;
  int lowerBound = 0;
  int upperBound = lowerBound + problemSize - 1;

  // allocate and initialize the space
  double** space[2]; 
  // allocate y axis
  space[0] = (double**) malloc( (problemSize + 2) * sizeof(double*));
  space[1] = (double**) malloc( (problemSize + 2) * sizeof(double*));
  if( space[0] == NULL || space[1] == NULL ){
    printf( "Could not allocate y axis of space array\n" );
    exit(0);
  }

  // allocate x axis
  for( i = 0; i < problemSize + 2; ++i ){
    space[0][i] = (double*) malloc( (problemSize + 2) * sizeof(double));
    space[1][i] = (double*) malloc( (problemSize + 2) * sizeof(double));
    if( space[0][i] == NULL || space[1][i] == NULL ){
      printf( "Could not allocate x axis of space array\n" );
      exit(0);
    }
  }

  // need to seed the space one tile at a time in order to 
  // duplicate the MPI version
  int tile_count = x_tile_count * y_tile_count;
  int tile_len_x = ceild(problemSize,x_tile_count);
  int tile_len_y = ceild(problemSize,y_tile_count);
  int t_idx;
  for(t_idx=0;t_idx<tile_count;t_idx++){
    int t_x = t_idx % x_tile_count;
    int t_y = t_idx/x_tile_count;
    // use global seed to seed the random number gen 
    srand(seed+t_idx);
    int lowerBound_y = t_y*tile_len_y+1;
    int lowerBound_x = t_x*tile_len_x+1;
    int upperBound_y = lowerBound_y + tile_len_y;
    int upperBound_x = lowerBound_x + tile_len_x;
    if(upperBound_x > problemSize+1){
      upperBound_x = problemSize+1;
    }
    if(upperBound_y > problemSize+1){
      upperBound_y = problemSize+1;
    }
    // seed the space.
    int x, y;
      //fprintf(stderr,"v:y(%d,%d) x(%d,%d)\n",lowerBound_y,upperBound_y,
                                             //lowerBound_x,upperBound_x);
    for( y = lowerBound_y; y < upperBound_y; ++y ){
      for( x = lowerBound_x; x < upperBound_x; ++x ){
        space[0][y][x] = rand() / (double)rand();
        //fprintf(stderr,"v:(%d,%d) = %f",y,x,space[0][y][x]);
      }
    }
  }
  // set halo values (sanity)
  for( i = 0; i < problemSize + 2; ++i){
    space[0][i][0] = 0;
    space[1][i][0] = 0;
    space[0][i][problemSize+1 ] = 0;
    space[1][i][problemSize+1 ] = 0;

    space[0][0][i] = 0;
    space[1][0][i] = 0;
    space[0][problemSize+1 ][i] = 0;
    space[1][problemSize+1 ][i] = 0;
  }

  
  int t,read = 0, write = 1;
  int x,y;
  lowerBound = 1;
  upperBound = problemSize;
  for( t = 1; t <= steps; ++t ){
    for( x = lowerBound; x <= upperBound; ++x ){
      for( y = lowerBound; y <= upperBound; ++y ){
        space[write][x][y] = ( space[read][x-1][y] +
                               space[read][x][y] +
                               space[read][x+1][y] +
                               space[read][x][y+1] +
                               space[read][x][y-1] )/5;
        /*fprintf(stderr,"vcalc (%d,%d):%f (%f,%f,%f,%f,%f)\n",x,y,
                               space[write][x][y],
                               space[read][x][y],
                               space[read][x][y-1],
                               space[read][x][y+1],
                               space[read][x-1][y],
                               space[read][x+1][y]);*/
      }    
    }
    read = write;
    write = 1 - write;
  }
  
  bool failed = false;
  for( x = lowerBound; x <= upperBound; ++x ){
    for( y = lowerBound; y <= upperBound; ++y ){
      if( result[x][y] != space[ steps & 1 ][x][y] ){
        failed = true;
        fprintf(stderr,"Position %d,%d expected: %f found:%f\n",
                x,y,space[ steps & 1 ][x][y],result[x][y]);
        break;
      }
    }
    if( failed ) break;
  }
  
  return !failed;

}
bool verifyResultJacobi2D1malloc( double* result,int problemSize,
                                  int seed,int steps){

  int i;
  int lowerBound = 1;
  int upperBound = lowerBound + problemSize - 1;

  // allocate and initialize the space
  double** space[2]; 
  // allocate x axis
  space[0] = (double**) malloc( (problemSize + 2) * sizeof(double*));
  space[1] = (double**) malloc( (problemSize + 2) * sizeof(double*));
  if( space[0] == NULL || space[1] == NULL ){
    printf( "Could not allocate x axis of space array\n" );
    exit(0);
  }

  // allocate y axis
  for( i = 0; i < problemSize + 2; ++i ){
    space[0][i] = (double*) malloc( (problemSize + 2) * sizeof(double));
    space[1][i] = (double*) malloc( (problemSize + 2) * sizeof(double));
    if( space[0][i] == NULL || space[1][i] == NULL ){
      printf( "Could not allocate y axis of space array\n" );
      exit(0);
    }
  }

  // use global seed to seed the random number gen (will be constant)
  srand(seed);
  // seed the space.
  int x, y;
  for( x = lowerBound; x <= upperBound; ++x ){
    for( y = lowerBound; y <= upperBound; ++y ){
      space[0][x][y] = rand() / (double)rand();
    }
  }

  // set halo values (sanity)
  for( i = 0; i < problemSize + 2; ++i){
    space[0][i][0] = 0;
    space[1][i][0] = 0;
    space[0][i][problemSize + 1] = 0;
    space[1][i][problemSize + 1] = 0;

    space[0][0][i] = 0;
    space[1][0][i] = 0;
    space[0][problemSize + 1][i] = 0;
    space[1][problemSize + 1][i] = 0;
  }

  
  int t,read = 0, write = 1;
  for( t = 1; t <= steps; ++t ){
    for( x = lowerBound; x <= upperBound; ++x ){
      for( y = lowerBound; y <= upperBound; ++y ){
        //fprintf(stderr,"%f\n",space[read][x][y]);
        space[write][x][y] = ( space[read][x-1][y] +
                               space[read][x][y] +
                               space[read][x+1][y] +
                               space[read][x][y+1] +
                               space[read][x][y-1] )*0.2;
      }    
    }
    read = write;
    write = 1 - write;
  }
  
  bool failed = false;
  for( x = lowerBound; x <= upperBound; ++x ){
    for( y = lowerBound; y <= upperBound; ++y ){
      if( result[x*(problemSize+2)+y] != space[ steps & 1 ][x][y] ){
        failed = true;
        fprintf(stderr,"Position: (%d,%d) Expected: %f, Found: %f\n",x,y,
            result[x*(problemSize+2)+y], space[ steps & 1 ][x][y]);
        break;
      }
    }
    if( failed ) break;
  }
  
  return !failed;

}
// returns true if valid result
bool verifyResultJacobi2D( double** result,int problemSize,int seed,int steps){

  int i;
  int lowerBound = 1;
  int upperBound = lowerBound + problemSize - 1;

  // allocate and initialize the space
  double** space[2]; 
  // allocate x axis
  space[0] = (double**) malloc( (problemSize + 2) * sizeof(double*));
  space[1] = (double**) malloc( (problemSize + 2) * sizeof(double*));
  if( space[0] == NULL || space[1] == NULL ){
    printf( "Could not allocate x axis of space array\n" );
    exit(0);
  }

  // allocate y axis
  for( i = 0; i < problemSize + 2; ++i ){
    space[0][i] = (double*) malloc( (problemSize + 2) * sizeof(double));
    space[1][i] = (double*) malloc( (problemSize + 2) * sizeof(double));
    if( space[0][i] == NULL || space[1][i] == NULL ){
      printf( "Could not allocate y axis of space array\n" );
      exit(0);
    }
  }

  // use global seed to seed the random number gen (will be constant)
  srand(seed);
  // seed the space.
  int x, y;
  for( x = lowerBound; x <= upperBound; ++x ){
    for( y = lowerBound; y <= upperBound; ++y ){
      space[0][x][y] = rand() / (double)rand();
    }
  }

  // set halo values (sanity)
  for( i = 0; i < problemSize + 2; ++i){
    space[0][i][0] = 0;
    space[1][i][0] = 0;
    space[0][i][problemSize + 1] = 0;
    space[1][i][problemSize + 1] = 0;

    space[0][0][i] = 0;
    space[1][0][i] = 0;
    space[0][problemSize + 1][i] = 0;
    space[1][problemSize + 1][i] = 0;
  }

  
  int t,read = 0, write = 1;
  for( t = 1; t <= steps; ++t ){
    for( x = lowerBound; x <= upperBound; ++x ){
      for( y = lowerBound; y <= upperBound; ++y ){
        space[write][x][y] = ( space[read][x-1][y] +
                               space[read][x][y] +
                               space[read][x+1][y] +
                               space[read][x][y+1] +
                               space[read][x][y-1] )/5;
      }    
    }
    read = write;
    write = 1 - write;
  }
  
  bool failed = false;
  for( x = lowerBound; x <= upperBound; ++x ){
    for( y = lowerBound; y <= upperBound; ++y ){
      if( result[x][y] != space[ steps & 1 ][x][y] ){
        failed = true;
        break;
      }
    }
    if( failed ) break;
  }
  
  return !failed;

}

void printResultsCUDA(char * executableName, int N, int T, int blockSize, int tau, float transferTo, float transferFrom, float total){
    
  printf( "Executable:%s,N:%d,T:%d,BlockSize:%d,tau:%d,DataTransferToDevice:%f,Processing:%f,DataTransferFromDevice:%f,TotalTime:%f\n",
          executableName,
          N,
          T,
          blockSize,
          tau,
          transferTo*.001,
          (total - transferTo -transferFrom)*.001,
          transferFrom*.001,
          total*.001
      );

}


