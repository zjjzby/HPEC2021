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
#include <thread>
#include <math.h> 

#include "./utility.hpp"
#include "./types.hpp"
#include "./sampleromp.hpp"
#include "./minibatch.hpp"



using namespace std;

// void sampleminibatchout(neighborhoodsampler & sampler, minibatch & batch, int id ){

//   int result = sampler.SampleAminibatch(batch, id);
// }




void PerformanceModeling(minibatch & itminibatch, int Nlayers, std::vector<int>  flenArrayh, timeProfiler & trecorder);

int shardpartition(
  std::vector<edge_type> & edgeList, 
  std::vector<std::vector<edge_type >> & shardlist, 
  std::vector<int> & nnzarray, 
  const int npartition, 
  const int nnz, 
  const int ksize);







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

  
  int numberOfLayers = 2;
  std::vector<int>  flenArray;
  flenArray.push_back(500);
  flenArray.push_back(128);
  flenArray.push_back(7);

  int Nnodes = 64;
  

  start = std::clock();
  neighborhoodsampler mysampler(GraphFile);
  std::cout << "Time for reading the graph from the files " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;


  printf("the number of vertices is %d\n", mysampler.mygraph.Nofvertices);
  printf("the number of nodes is %d\n", mysampler.mygraph.Nofedges);

  start = std::clock();
  mysampler.Generate_adjtable();
  std::cout << "Time for constructing the adjacency table " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;



  start = std::clock();
  mysampler.InitializeEpoch(Nnodes);
  std::cout << "Time for training epoch initialization " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;


  std::clock_t epochstart = std::clock();



  std::vector<timeProfiler> timeTreacorder;

  int Nthreads =1;


  for(int epoch = 0; epoch < mysampler.mygraph.Nofvertices/(Nnodes*Nthreads);epoch++){
  // for(int epoch = 0; epoch < 1;epoch++){
    std::vector<minibatch> batches;
    // batches.resize(Nthreads);
    for(int i =0 ;i < Nthreads; i++){
      batches.push_back(minibatch());
    }

    for(int i =0 ;i < Nthreads; i++){
      batches[i].setnumberoflayers(numberOfLayers);
    }



    // printf("test::number of layers is %d\n", batches[0].numberOfLayers);


    // set layer budegts
    std::vector<int>  layerbudgets;
    layerbudgets.push_back(10);
    layerbudgets.push_back(25);
    mysampler.setsamplingbudgets(numberOfLayers, layerbudgets);
    
    std::thread threads[Nthreads];

    auto start = chrono::steady_clock::now();

    for(int i =0 ;i < Nthreads; i++){

      threads[i] = std::thread(&neighborhoodsampler::SampleAminibatch, &mysampler, std::ref(batches[i]), i, epoch*Nnodes*Nthreads + i*Nnodes,  epoch*Nnodes*Nthreads + (i + 1)*Nnodes - 1);
    }

    for(int i =0 ;i < Nthreads; i++){
      threads[i].join();
    }

    // for (auto& th : threads) th.join();

    float samplingtime = 0;

    auto end = chrono::steady_clock::now();

    auto diff = end - start;
 

    samplingtime = chrono::duration <double, milli> (diff).count();
    
    // std::cout << "Time for sampling " << samplingtime << " ms" << std::endl;


    for(int i = 0; i < Nthreads; i++){
      timeProfiler itime;
      itime.samplingTime = samplingtime/Nthreads;

      PerformanceModeling(batches[i],  numberOfLayers, flenArray, itime);

      printf("The sampling time is %0.9f \n", itime.samplingTime);
      printf("The data transmission time is %0.9f \n", itime.dataTransmissionTime);
      printf("The GNN operation time is %0.9f \n", itime.GNNOperationTime);
      printf("\n");
    }
    // for(int i =0 ;i < 16; i++){
    //   printf("The start of the minibatch %d\n", batches[i].start);
    //   printf("The end of the minibatch %d\n", batches[i].end);
    // }

    // for(int i =0 ;i < 16; i++){
    //   printf("The number of 1-hop neighbors is %d\n", batches[i].VertexLists[0].size());
    //   printf("The number of 2-hop neighbors is %d\n", batches[i].VertexLists[1].size());
    // }

    // for(int i = 0; i < batches[0].VertexLists[0].size();i++){
    //   printf("vertex layer 0: %d\n", batches[0].VertexLists[0][i]);
    // }

    // for(int i = 0; i < batches[0].VertexLists[1].size();i++){
    //   printf("vertex layer 1: %d\n", batches[0].VertexLists[1][i]);
    // }

    // for(int i = 0; i < batches[0].VertexLists[2].size();i++){
    //   printf("vertex layer 2: %d\n", batches[0].VertexLists[2][i]);
    // }

    // for(int i = 0; i < batches[0].EdgeLists[0].size();i++){
    //   printf("Edge layer 0: (%d, %d)\n", batches[0].EdgeLists[0][i].src, batches[0].EdgeLists[0][i].dst);
    // }

    // for(int i = 0; i < batches[0].EdgeLists[1].size();i++){
    //   printf("Edge layer 1: (%d, %d)\n", batches[0].EdgeLists[1][i].src, batches[0].EdgeLists[1][i].dst);
    // }

    
  }
   std::cout << "Time for an epoch " << (std::clock() - epochstart) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;

  return 0;

}






void PerformanceModeling(minibatch & itminibatch, int Nlayers, std::vector<int>  flenArrayh, timeProfiler & trecorder){
    std::vector<int> NvertexArray;
    std::vector<int> NedgeArray;
    NvertexArray.resize(Nlayers + 1);
    NedgeArray.resize(Nlayers);

    int totaledges = 0;

    for(int i = 0; i < Nlayers + 1; i++){
      NvertexArray[i] = itminibatch.VertexLists[i].size();
    }

    
    for(int i = 0; i < Nlayers; i++){
      NedgeArray[i] = itminibatch.EdgeLists[i].size();
      totaledges +=  NedgeArray[i];
    }

    // Calculate the time for mini-batch transmit though PCIE
    float timeOfPCIE;

    timeOfPCIE = totaledges*8.0/(10.3*( pow(10.0, 9.0 ))) + 300.0/pow(10.0, 9.0 );

    trecorder.dataTransmissionTime = timeOfPCIE*1000;
    // printf("Total number of edgees is %d, The PCIE time is %.9f ms\n",  totaledges, timeOfPCIE * 1000);

    int sizeIfDstBuffer = 1024 * 1024 * 32;
    int NPE = 4;
    int PEparallelism = 16 * 4 * 4;
    int TPEparallelism = 16 * 16 * 2;

    int PEweighUpdate = 8*8*2;


    std::vector<float> timePerLayer;
    std::vector<float> timeForWeightUpdate;

    
    for(int i = 0; i < Nlayers; i++){

      float FeatureLoadingTime = 0;
      float AggregationComputeTIme = 0;
      float FeatureTransformTIme = 0;

      int Npartition = 0;
      int ksize = 0;

      // calculate the number of partitions is needed
      
      // TimeForFowardProgagation
      // ForwardPropogation


      if(NvertexArray[i + 1] * flenArrayh[i] <= sizeIfDstBuffer * NPE){
        Npartition = NPE;
        ksize = NvertexArray[i + 1] / 4;
        if(NvertexArray[i + 1] % 4 != 0){
          ksize = ksize + 1;
        }
      }
      else{
        Npartition = (NvertexArray[i + 1] * flenArrayh[i]) / sizeIfDstBuffer;

        if((NvertexArray[i + 1] * flenArrayh[i]) % sizeIfDstBuffer != 0){
          Npartition = Npartition + 1;
        }

        ksize = sizeIfDstBuffer/ flenArrayh[i];
      }

      std::vector<std::vector<edge_type >>  shardlist;
      shardlist.resize(Npartition);
      std::vector<int>  nnzarray;
      nnzarray.resize(Npartition);

      shardpartition(itminibatch.EdgeLists[i], shardlist, nnzarray, Npartition, NedgeArray[i], ksize);

      int totalFeatureTraffic = 0;

      for(int k = 0; k <= Npartition; k++){
        totalFeatureTraffic += nnzarray[i];
      }


      if(i == 0){
        FeatureLoadingTime = totalFeatureTraffic * flenArrayh[i] * 4.0/ (77.0 * 0.2 * pow(10, 9.0))  +  NvertexArray[i + 1] * flenArrayh[i] * 4.0 / (67.0 * pow(10, 9.0))  ;
        //printf("Total number of Traffic is %d, The feature_loading time is %.9f s\n",  totalFeatureTraffic, totalFeatureTraffic * flenArrayh[i]* 4/(77.0*0.2*pow(10, 8.0)));
      }
      else{
        FeatureLoadingTime = totalFeatureTraffic * flenArrayh[i] * 4.0/ (55.0 * pow(10, 9.0))  +  NvertexArray[i + 1] * flenArrayh[i] * 4 / (67.0 * pow(10, 9.0)) ;
      }

      AggregationComputeTIme = NedgeArray[i] * flenArrayh[i]/ (PEparallelism * 0.7 * 0.27 * pow(10, 9.0));

      FeatureTransformTIme = NvertexArray[i + 1] * flenArrayh[i] * flenArrayh[i + 1] / (TPEparallelism * 0.7 * 0.27 * pow(10, 9.0)) ;

      // timeForWeightUpdate.push_back(NvertexArray[i + 1] * flenArrayh[i] * flenArrayh[i + 1] / (PEweighUpdate * 0.7 * 0.27 * pow(10, 8.0) )

      // printf("The feature loading time of %d layer is %0.9f\n", i, FeatureLoadingTime );
      // printf("The Aggregation  time of %d layer is %0.9f\n", i,  AggregationComputeTIme );
      // printf("The Transformation time of %d layer is %0.9f\n", i,  FeatureTransformTIme );

      timePerLayer.push_back(std::max(FeatureLoadingTime, AggregationComputeTIme) + FeatureTransformTIme );

      // printf("the number of NvertexArray[i + 1] of the last layer is %d \n", NvertexArray[i + 1]);
      // printf("the number of flenArrayh[i] of the last layer is %d \n", flenArrayh[i]);
      // printf("the number of flenArrayh[i + 1] of the last layer is %d \n", flenArrayh[i + 1]);

      if(i != 0){
          timePerLayer.push_back(std::max(FeatureLoadingTime, AggregationComputeTIme) + FeatureTransformTIme * 2);
      }
      else{
          timePerLayer.push_back(FeatureTransformTIme * 2);
      }
      // Time for backpropagation
      // if(i != 0){
      
      //   for(int iedge = 0; iedge < itminibatch.EdgeLists[i].size(); iedge++){
      //     edge_type tmpEdge;
      //     tmpEdge = itminibatch.EdgeLists[i][iedge];
      //     itminibatch.EdgeLists[i][iedge].src = itminibatch.EdgeLists[i][iedge].dst;
      //     itminibatch.EdgeLists[i][iedge].dst = tmpEdge.src;
      //   }
      //   std::sort( itminibatch.EdgeLists[i].begin(), itminibatch.EdgeLists[i].end(), compareedge);



      // }
      // else{
      //   FeatureLoadingTime = 0;
      //   AggregationComputeTIme = 0;
      //   FeatureTransformTIme = 0;
      // }

    }

    float timeOfLayer = 0;
    for(int i = 0; i < timePerLayer.size(); i++){
      timeOfLayer += timePerLayer[i];
    }

    // printf("The time of layer operations is %.9f ms\n",  timeOfLayer * 1000);


    trecorder.GNNOperationTime = timeOfLayer * 1000;
    

    // timeWeightUpdate = timeWeightUpdate + 

    // Calculate the time for forward propogate





}





int shardpartition(
  std::vector<edge_type> & edgeList, 
  std::vector<std::vector<edge_type >> & shardlist, 
  std::vector<int> & nnzarray, 
  const int npartition, 
  const int nnz, 
  const int ksize)
{
  edge_type tmpedge[npartition];


  int ii[npartition];
  int jj[npartition];

  for (int i = 0 ; i< npartition; i++){
    ii[i] = 0;
    jj[i] = 0;
    nnzarray[i] = 0;
  }


  edge_type ipedge;
  v_edges ivedge;

  int innz = 0;

  // printf("the npartition is %d\n", npartition);
  // printf("the size of edgeList is %d\n", edgeList.size());

  for (int i = 0; i < edgeList.size(); i++){
    ipedge = edgeList[i];
    // calculate which npartition the edge belongs to

    int ipar = (ipedge.dst)/(ksize);
    // printf("the ksize is %d, the nnz  is %d, npartition is %d, the partition is %d\n", ksize, nnz, npartition, ipar);
    

    tmpedge[ipar] =  ipedge;

    jj[ipar]++;

    shardlist[ipar].push_back(tmpedge[ipar]);

    nnzarray[ipar]++;
  }


  


  for (int i = 0 ; i< npartition; i++){
    shardlist[i].erase( 
      std::unique( 
          shardlist[i].begin(), 
          shardlist[i].end(),
          equalsrc), 
      shardlist[i].end() 
    );
    nnzarray[i] = shardlist[i].size();
    // printf("the size of %dth partition is %d\n", i, shardlist[i].size());
  }
  return 0;
   


}


