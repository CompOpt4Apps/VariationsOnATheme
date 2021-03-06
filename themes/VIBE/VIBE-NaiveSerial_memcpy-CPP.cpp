#include<iostream>
#include<stdio.h>
#include<string>
#include<cmath>
#include<stdlib.h>
#include<time.h>

#include "opencv2/opencv.hpp"
#include "verification.h"
#include "../common/Configuration.h"
#include "../common/Measurements.h"
#include "util.h"


using namespace std;
using namespace cv;

#define FRAME

int iter = 0;          

int main(int argc,char* argv[])
{   


  string filename;
  int N;
  int R;
  int time_sample;
  int match_count;


  clock_t t1_start,t1_end,t2_start,t2_end,t3_start,t3_end;
  clock_t start_time,end_time;
  float initialize_time,segment_time,update_time,memory_transfer_time;

//Constructor for parsing command line arguments
   
   Configuration config;
  
   /**************************************************************************
    ** Parameter options. Help and verify are constructor defaults.          *
    **************************************************************************/ 

    config.addParamString("filename", 'f', argv[1], "--The filename of the video");
    // changed the 'o' to 'n' (conflict with "output")
    config.addParamInt("numberofSamples", 's' , 20 , "--The number of samples in model for each pixel");
    config.addParamInt("radiusofSphere", 'r' , 16 , "--The radius of euclidean sphere");
    config.addParamInt("timeSampling",'i', 16 , "--Sampling in time factor");
    config.addParamInt("matchingSamples", 'c' , 2 , "--Number of samples within the radius");
    config.addParamString("output", 'o',"NULL", " --The filename of the result");
    // added the following addParam's from vibe_naive: common/Configuration.cpp 
    config.addParamInt("Nx",'k',100,"--Nx <problem-size> for x-axis in elements (N)");
    config.addParamInt("Ny",'l',100,"--Ny <problem-size> for y-axis in elements (N)");
    config.addParamInt("Nz",'m',100,"--Nz <problem-size> for z-axis in elements (N)");
    config.addParamInt("bx",'x',1,"--bx <block-size> for x-axis in elements");
    config.addParamInt("by",'y',1,"--by <block-size> for y-axis in elements");
    config.addParamInt("bz",'z',1,"--bz <block-size> for z-axis in elements");
    config.addParamInt("tilex",'a',128,"--tilex <tile-size> for x-axis in elements");
    config.addParamInt("tiley",'b',128,"--tiley <tile-size> for y-axis in elements");
    // changed c to q (conflict with "matchingSamples" 
    config.addParamInt("tilez",'q',128,"--tilez <tile-size> for z-axis in elements");
    config.addParamInt("T",'T',100,"-T <time-steps>, the number of time steps");
    // changed h to q (conflict with "help")
    config.addParamInt("height",'j',-1,"height <#>, number of time steps in tile");
    config.addParamInt("tau",'t',30,"--tau <tau>, distance between tiling"
                                "hyperplanes (all diamond(slab))");
    config.addParamInt("num_threads",'p',1,"-p <num_threads>, number of cores");
    config.addParamInt("global_seed", 'g', 1524, "--global_seed " 
                                           "<global_seed>, seed for rng"); 
    config.addParamBool("n",'n', false, "-n do not print time");

    config.parse(argc, argv);

  filename = config.getString("filename");
 
  if(filename == "NULL")
  {
    cout << " unable to find file" << endl;
    exit(-1);
  }
  

  N = config.getInt("numberofSamples");
  R = config.getInt("radiusofSphere");
  time_sample = config.getInt("timeSampling");
  match_count = config.getInt("matchingSamples");

//checking for command line arguments
  if(N < 0)
 {
   fprintf(stderr,"The value of N has to be a positive number %d\n",N);
   exit(-1);
 }

 if(R < 0)
 {
   fprintf(stderr,"The value of R has to be a positive number %d\n",R);
    exit(-1);
 } 

  if(time_sample < 0)
  {
   fprintf(stderr,"The value of time_sample has to be a positive number %d\n",time_sample);
   exit(-1);
  }

  if(match_count < 0)
  {
   fprintf(stderr,"The value of match_count has to be a positive number %d\n",match_count);
   exit(-1);
  }

//2 Initializiing pointers

  pixel* image = NULL;
  pixel* start_image = NULL;
  unsigned char* segmap = NULL;
  pixel* model = NULL;

// Parameters of the image
 
  int height;
  int width;
  int numofFrames;

// Initializing variables

  int frameCount = 0;
  
// Getting frame info
  string& fp = filename;
  VideoCapture capture(fp);

//get frame info

 #ifdef VIDEO
  numofFrames = (int)capture.get(CV_CAP_PROP_FRAME_COUNT);
  width  = (int)capture.get(CV_CAP_PROP_FRAME_WIDTH);
  height = (int)capture.get(CV_CAP_PROP_FRAME_HEIGHT);
 #endif
 
//Allocating memory to pixel
 #ifdef VIDEO
  model = new pixel[width*height*N];
  image = new pixel[width*height];
  start_image = image;
 #endif

// to create different random numbers every time
  srand(time(NULL));
   
// Reading all the frames and initializing the model 
  Mat frame;
  capture >> frame;

  start_time = clock();
  while(!frame.empty())
  {
   #ifdef FRAME
    height = frame.rows;
    width = frame.cols;
   #endif
 
// allocate memory for segmented map for each frame
    #ifdef FRAME
    image = new pixel[width*height];
    #endif

    segmap = new unsigned char[width*height];
   
    #ifdef VIDEO
     image = start_image;
    #endif
   
// assign image
     clock_t t = clock();
     pixel* frame_pixel = (pixel*)frame.data;
     memcpy(image,frame_pixel,height*width*sizeof(pixel));
     clock_t t_end = clock();
     float difference(float(t_end) - float(t));
     difference = difference/CLOCKS_PER_SEC;
     memory_transfer_time += difference;
// initialize pixel model and time 
    
    if(frameCount ==0){ 
      t1_start = clock();
      #ifdef FRAME
      model = new pixel[width*height*N];
      #endif
      initialize_model(image,model,height,width,N); 
      t1_end = clock();
      float diff(float(t1_end) - float(t1_start));
      diff = diff/CLOCKS_PER_SEC;
      initialize_time += diff; 
    
    }
  
// segment the frame and time

    t2_start = clock();
    segment_frame(segmap,image,model,height,width,N,R,match_count,frameCount);
    t2_end = clock();
    float diff(float(t2_end) - float(t2_start));
    diff = diff/CLOCKS_PER_SEC;
    segment_time += diff;
    

// update the model and its neighbour and time
    if(frameCount > 0)
    {
      t3_start = clock();
      update_model(segmap,image,model,height,width,
                 time_sample,N,frameCount);
      t3_end = clock();
      float diff(float(t3_end) -float(t3_start));
      diff = diff/CLOCKS_PER_SEC;
      update_time += diff;
      
    }
    
    frameCount++;
    
    delete[] segmap;
    #ifdef FRAME
    delete[] image;
    #endif

    capture >> frame;
   
  }
  end_time = clock();
 
//total time for all frames 
  float total_time(float(end_time) - float(start_time));
  total_time = total_time/CLOCKS_PER_SEC;
  

//delete the allocated arrays
#ifdef VIDEO
  delete[] start_image;
#endif
  delete[] model;

//release the cpture instance
  capture.release();

//print time measurements
  Measurements measure;
  measure.setField("Initialize_model",initialize_time);
  measure.setField("Segment_frames",segment_time);
  measure.setField("Update_frames",update_time);
  measure.setField("Total_time",total_time);
  measure.setField("Measure_transfer_memcpy",memory_transfer_time);

  measure.getFieldFloat("Initialize_model");
  measure.getFieldFloat("Segment_frames");
  measure.getFieldFloat("Update_frames");
  measure.getFieldFloat("Total_time");
  measure.getFieldFloat("Measure_transfer_memcpy");
  string result = measure.toLDAPString();
  cout << result << endl;

//print the string configuration
  string lda = config.toLDAPString();
  cout << lda << endl;

  return 0;
}
