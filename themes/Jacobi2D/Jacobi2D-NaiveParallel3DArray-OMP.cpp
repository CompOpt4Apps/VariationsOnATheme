/*****************************************************************************
 * Jacobi2D benchmark
 * Basic parallelisation with OpenMP using a static scheduling for the loop
 * over the spatial dimension.
 *
 * Usage:
 *  make omp
 *  ./Jacobi2D-NaiveParallel-OMP --Nx 5000 --Ny 5000 -T  50 --num_threads 8
 * For a run on 8 threads
 *
 * To see possible options:
 *   Jacobi2D-NaiveParallel-OMP --help 
*****************************************************************************/ 
#include <stdio.h>
#include <iostream>
#include <omp.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include <string>
#include <sstream>

#include "util3DArray.h"
#include "../common/Measurements.h"
#include "../common/Configuration.h"

using namespace std;

// main
// Stages
// 1 - command line parsing
// 2 - data allocation and initialization
// 3 - jacobi 1D timed within an openmp loop
// 4 - output and optional verification

int main( int argc, char* argv[] ){
  
  // rather than calling fflush    
  setbuf(stdout, NULL);

  // 1 - Command line parsing and checking  
  Measurements  measurements;
  Configuration configuration;
  /**************************************************************************
  **  Parameter options. Help and verify are constructor defaults  **********
  **************************************************************************/
  configuration.addParamInt("Nx",'k',100,
                            "--Nx <problem-size> for x-axis in elements (N)");

  configuration.addParamInt("Ny",'l',100,
                            "--Ny <problem-size> for y-axis in elements (N)");

  configuration.addParamInt("T",'T',100,
                            "-T <time-steps>, the number of time steps");

  configuration.addParamInt("num_threads",'p',1,
                            "-p <num_threads>, number of cores");

  configuration.addParamInt("global_seed", 'g', 1524,
                            "--global_seed <global_seed>, seed for rng");

  configuration.addParamBool("n",'n', false, "-n do not print time");

  configuration.parse(argc,argv);

  // make sure that OpenMP is going to use the number of threads 
  // indicated
  if(configuration.getInt("num_threads") > omp_get_max_threads()){
    printf("--num_threads cannot be more than %d\n",omp_get_max_threads());
    return false;
  }else if (configuration.getInt("num_threads") < 1){
    printf("--num_threads cannot be less than %d\n",1);
    return false;
  }else{
    omp_set_num_threads(configuration.getInt("num_threads"));
  }
 
  // Checking command line options 
  /*if( !checkCommandLineOptions(configuration) ){
    return 1;
  }  */

  // End Command line parsing and checking 

  // 2 - Data allocation and initialization
  int i;
  int j;
  int Nx = configuration.getInt("Nx");
  int Ny = configuration.getInt("Ny");
  int upper_bound_T = configuration.getInt("T");  
  int lower_bound_i = 1;
  int lower_bound_j = 1;  
  int upper_bound_i = lower_bound_i + Ny - 1;
  int upper_bound_j = lower_bound_j + Nx - 1; 
  real ** data[2];

  // Allocate time-steps 0 and 1
  if( !allocateSpace(data,configuration) ){
    printf( "Could not allocate space for array\n" );
    return 1;
  }
  // Initialize arrays 
  // First touch for openmp	
  #pragma omp parallel for private(i,j) schedule(runtime)
  for( i = lower_bound_i; i <= upper_bound_i; ++i ){
    for( j = lower_bound_j; j <= upper_bound_j; ++j ){
      data[0][i][j] = 0;
    }
  }
  initializeSpace(data,configuration);
 
  // End data allocation and initialization
  
  // 3 - jacobi 2D timed within an openmp loop
  double start_time = omp_get_wtime();

  for( int t = 1; t <= upper_bound_T; ++t ){ 
    #pragma omp parallel for private(i,j) schedule(runtime)
    for( i = lower_bound_i; i <= upper_bound_i; ++i ){
      for( j = lower_bound_j; j <= upper_bound_j; ++j ){
        stencil(t,i,j);
      }    
    }
  }
  double end_time = omp_get_wtime();
  double time =  (end_time - start_time);

  measurements.setField("elapsedTime",time);

  // End timed test
  
  // 4 - Verification and output (optional)
  // Verification
  if( configuration.getBool("v") ){
    if( verifyResultJacobi2D(data[(upper_bound_T) & 1],configuration) ){
       measurements.setField("verification","SUCCESS");    
    }else{
       measurements.setField("verification","FAILURE");
    }
  }
  // Output
  if( !configuration.getBool("n") ){
    string ldap = configuration.toLDAPString();
    ldap += measurements.toLDAPString();
    cout<<ldap;
    cout<<endl;
  }
  
  // End verification and output
  
  // Free the space in memory
   for( int i = 0; i < configuration.getInt("Ny") + 2; ++i ){
     free( data[0][i] );
     free( data[1][i] );
   } 
   free( data[0] );
   free( data[1] );
 
  return 0;   
}
