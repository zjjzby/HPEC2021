#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
// #define DEBUG
//#define PRINT_MATRIX
#include <vector>
#include <CL/cl2.hpp>


#include <iostream>
#include <fstream>
#include <CL/cl_ext_xilinx.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ap_int.h>
#include <cstdlib>
#include <ctime>
#include <iterator>
#include <string>
#include <cfloat>
#include <CL/cl_ext.h>


#include "./utility.hpp"
#include "./types.hpp"


using namespace std;


int main(int argc, char ** argv){

    printf("program starts\n");
    std::string GraphFile;

    int c;


  // parse the command line argumentrere
  // -x the name of edge file

  while ((c = getopt (argc, argv, "x:")) != -1){
    
    switch (c){
      case 'x':
        GraphFile = optarg;
        break;

      default:
        abort();
    }
  }
  
  std::clock_t start;

  start = std::clock();
  GraphObj mygraph(GraphFile);

  std::cout << "Time for reading the graph from the files " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;

  printf("the number of vertices is %d\n", mygraph.Nofvertices);
  printf("the number of nodes is %d\n", mygraph.Nofedges);


  return 0;

}