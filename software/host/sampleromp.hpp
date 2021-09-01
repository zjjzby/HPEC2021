
#ifndef SAMPLER_DEF
#define SAMPLER_DEF


#include <stdio.h>
#include <string.h>
#include <random>
#include <ctime>
#include <chrono>
#include <omp.h>
#include <pthread.h>
#include <mutex>
#include <random>
#include <thread>
#include <cstdlib>
#include <map>

#include "./minibatch.hpp"
#include "./types.hpp"
#include "./utility.hpp"

using namespace std;


class basicsampler {
    public:
        GraphObj mygraph;

        basicsampler(std::string &edge_file_name){
            this->mygraph = GraphObj(edge_file_name);
        }

};

bool compareedge(edge_type i,edge_type j) { 
    
    if(i.src < j.src){
        return (i.src < j.src);
    }
    else if(i.src == j.src){
        return (i.dst < j.dst);
    }
    return 0; 
}

bool equaledge(edge_type i,edge_type j) { 
    

    return (i.src == j.src)&&(i.dst == j.dst); 
}


bool equalsrc(edge_type i,edge_type j) { 
    

    return (i.src == j.src); 
}




class neighborhoodsampler: public basicsampler {
    public:
        std::vector<adj_table_entry> adj_table;
        std::vector<feature_table_entry> feature_table;

        std::vector<int> budgets;
        std::vector<index_type> minibatchIndices;

        int minibatchIndicesInitflag;
        int minibatchsize;
        int numberofiteration;

        int minibatchIterationPoint;

        std::mutex mtx; // lock for parallel sampler

        neighborhoodsampler(std::string &edge_file_name): basicsampler(edge_file_name){
            this->minibatchIndicesInitflag = 0;
            this->minibatchIterationPoint = 0;
        }

        void setsamplingbudgets(int Nlayers, std::vector<int> & layerbudgets){
            this->budgets.resize(Nlayers);
            std::copy(layerbudgets.begin(), layerbudgets.end(), this->budgets.begin());
        }

        void Generate_adjtable(){
            // printf("step 1\n");
            this->adj_table.resize(this->mygraph.Nofvertices);
            // printf("step 2\n");
            
            // printf("step 4\n");
            
            // printf("step 5\n");
            // #pragma omp parallel for num_threads(16) schedule(static, 10240) 
            for(int i = 0; i< (this->mygraph.Nofvertices); i++){
                
                unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                // printf("step 3\n");
                std::srand ( unsigned ( std::time(0) ) );
                // printf("proceed in vertices %d\n", i);
                int number_of_neighbors = this->mygraph.CSR_indptr[i+1] - this->mygraph.CSR_indptr[i];
                adj_table[i].src_id = i;
                adj_table[i].neighbors.resize(MAXDEGREE);
                std::default_random_engine generator (seed);
                // printf("step 6\n");
                if (number_of_neighbors > MAXDEGREE){
                    // printf("step 7\n");
                    std::random_shuffle(
                        this->mygraph.CSR_indices.begin() + this->mygraph.CSR_indptr[i], 
                        this->mygraph.CSR_indices.begin() + this->mygraph.CSR_indptr[i+1]
                    );
                    std::copy(
                        this->mygraph.CSR_indices.begin() + this->mygraph.CSR_indptr[i], 
                        this->mygraph.CSR_indices.begin() + this->mygraph.CSR_indptr[i] + MAXDEGREE,
                        adj_table[i].neighbors.begin()
                    );
                }
                else{
                    // printf("step 8 (%d, %d)\n", this->mygraph.CSR_indptr[i], this->mygraph.CSR_indptr[i+1]);
                    std::random_shuffle(
                        this->mygraph.CSR_indices.begin() + this->mygraph.CSR_indptr[i], 
                        this->mygraph.CSR_indices.begin() + this->mygraph.CSR_indptr[i+1]
                    );
                    // printf("step 9\n");
                    std::copy(
                        this->mygraph.CSR_indices.begin() + this->mygraph.CSR_indptr[i], 
                        this->mygraph.CSR_indices.begin() + this->mygraph.CSR_indptr[i] + number_of_neighbors,
                        adj_table[i].neighbors.begin()
                    );
                    // printf("step 10\n");
                    if(this->mygraph.CSR_indptr[i+1]  > this->mygraph.CSR_indptr[i]){
                        std::uniform_int_distribution<int> distribution(int(this->mygraph.CSR_indptr[i]), int(this->mygraph.CSR_indptr[i+1] -1));
                        for(int j = number_of_neighbors; j <MAXDEGREE; j++){
                            adj_table[i].neighbors[j] = this->mygraph.CSR_indices[distribution(generator)];
                        }
                    }
                    else{
                        std::uniform_int_distribution<int> distribution(int(this->mygraph.CSR_indptr[i]), int(this->mygraph.CSR_indptr[i]));
                        for(int j = number_of_neighbors; j <MAXDEGREE; j++){
                            adj_table[i].neighbors[j] = this->mygraph.CSR_indices[distribution(generator)];
                        }
                    }
                    
                    
                }
                // if (i < 4){
                //     printf("the vertex %d is (", i);
                //     for (int kkk = 0; kkk < MAXDEGREE; kkk++){
                //         printf("%d ", adj_table[i].neighbors[kkk]);
                //     }
                //     printf("\n");
                // }
            }
        }

        int InitializeEpoch(int batchsizeinput){
            if(minibatchIndicesInitflag == 0){ 
                this->minibatchIndices.resize(this->mygraph.Nofvertices);
                for(int i =0; i < this->mygraph.Nofvertices; i++){
                    this->minibatchIndices[i] = i;
                }
                std::random_shuffle(this->minibatchIndices.begin(), this->minibatchIndices.end());
                minibatchIndicesInitflag = 1;
            }
            else{
                std::random_shuffle(this->minibatchIndices.begin(), this->minibatchIndices.end());
            }
            this->minibatchsize = batchsizeinput;

            this->minibatchIterationPoint = 0;
        }


        int SampleAminibatch(minibatch & itminibatch, int threadid, int currentIterationStart, int currentInterationEnd){
            // int currentIterationStart;
            // int currentInterationEnd;

            // printf("stage1\n");
            // mtx.lock();
            // currentIterationStart = this->minibatchIterationPoint;
            // if(this->minibatchIterationPoint >= this->mygraph.Nofvertices){
            //     return 0;
            // }
            
            // if(this->minibatchIterationPoint + this->minibatchsize >= this->mygraph.Nofvertices){
            //     currentInterationEnd = this->mygraph.Nofvertices;
            //     this->minibatchIterationPoint = this->mygraph.Nofvertices;
            // }
            // else{
            //     currentInterationEnd = this->minibatchIterationPoint + this->minibatchsize;
            //     this->minibatchIterationPoint = this->minibatchIterationPoint + this->minibatchsize;
            // }

            // mtx.unlock();
            
            // //  printf("stage2\n");
            int Nlayers = itminibatch.numberOfLayers;

            // int insize = currentInterationEnd - currentIterationStart;

            // copy target vertices
            // printf("The thread ID is %d, the number of layers is %d\n", threadid, Nlayers);

            itminibatch.VertexLists[Nlayers].resize(currentInterationEnd- currentIterationStart);
            // printf("%d::the start index is %d\n", threadid, currentIterationStart);
            // printf("%d::the end index is %d\n", threadid, currentInterationEnd);

            // for(int kk = currentIterationStart; kk < currentInterationEnd; kk++){
            //     itminibatch.VertexLists[Nlayers].push_back( this->minibatchIndices[kk]);
            // }
            // Copy the target vertices
            std::copy(
                this->minibatchIndices.begin() + currentIterationStart,  
                this->minibatchIndices.begin() + currentInterationEnd,
                itminibatch.VertexLists[Nlayers].begin()
            );

            int numberofhopneighbors = currentInterationEnd - currentIterationStart;

            // printf("The thread ID is %d, the number of layers is %d\n", threadid, Nlayers);


            // printf("%d::first layer vertices %d\n", threadid, itminibatch.VertexLists[Nlayers].size());

            for(int kk = Nlayers - 1; kk >= 0; kk--){
                // printf("%d::%d layer sampling\n", threadid, kk);
                numberofhopneighbors = numberofhopneighbors*this->budgets[kk];
                itminibatch.VertexLists[kk].resize(numberofhopneighbors);
                // printf("The thread ID is %d, the number of neighbors is %d\n", threadid, numberofhopneighbors);
                int recorder = 0;

                for(auto ivertex :  itminibatch.VertexLists[kk+1]){
                    // printf("%d::%d vertex\n", threadid, ivertex);
                    for(int ibudgets = 0; ibudgets < this->budgets[kk] ; ibudgets++){
                        int randomindex = rand()%MAXDEGREE;
                        index_type ineighbor = this->adj_table[ivertex].neighbors[randomindex];
                        //itminibatch.VertexLists[kk].push_back(this->adj_table[ivertex].neighbors[randomindex]);
                        (itminibatch.VertexLists[kk])[recorder] = ineighbor;
                        recorder++;
                        // printf("%d::%d recorder\n", threadid, recorder);
                        edge_type iedge;
                        iedge.dst = ivertex;
                        iedge.src = ineighbor;
                        itminibatch.EdgeLists[kk].push_back(iedge);
                    }
                    
                }

              


                std::sort( itminibatch.VertexLists[kk].begin(), itminibatch.VertexLists[kk].end() );
                itminibatch.VertexLists[kk].erase( 
                    std::unique( 
                        itminibatch.VertexLists[kk].begin(), 
                        itminibatch.VertexLists[kk].end()), 
                    itminibatch.VertexLists[kk].end() 
                );
                // printf("The thread ID is %d, the number of sizes is %d\n", threadid, itminibatch.VertexLists[kk].size());
            }

            // vertex ID mapping
            std::vector<std::map<index_type, index_type>> idmapping;
            idmapping.resize(Nlayers);

            for(int kk = Nlayers - 1; kk >= 0; kk--){
                int arraysize = itminibatch.VertexLists[kk + 1].size();
                for(int i = 0; i < arraysize; i++){
                    idmapping[kk][itminibatch.VertexLists[kk + 1][i]] = i;
                    itminibatch.VertexLists[kk + 1][i] = i;
                }
            }

            for(int kk = Nlayers - 1; kk >= 0; kk--){
                int edgesize = itminibatch.EdgeLists[kk].size();
                for(int i = 0; i < edgesize; i++){
                    if(kk!=0){
                        itminibatch.EdgeLists[kk][i].src = idmapping[kk - 1][itminibatch.EdgeLists[kk][i].src];
                    }
                    itminibatch.EdgeLists[kk][i].dst = idmapping[kk][itminibatch.EdgeLists[kk][i].dst];
                }
                std::sort( itminibatch.EdgeLists[kk].begin(), itminibatch.EdgeLists[kk].end(), compareedge);
                itminibatch.EdgeLists[kk].erase( 
                    std::unique( 
                        itminibatch.EdgeLists[kk].begin(), 
                        itminibatch.EdgeLists[kk].end(),
                        equaledge), 
                    itminibatch.EdgeLists[kk].end() 
                );
            }
            



            // printf("%d::first layer vertices %d\n", threadid, itminibatch.VertexLists[Nlayers].size());
            // printf("%d::second layer vertices %d\n", threadid, itminibatch.VertexLists[Nlayers - 1].size());
            // printf("%d::Third layer vertices %d\n", threadid, itminibatch.VertexLists[Nlayers - 2].size());
            // printf("%d::second layer number of edges %d\n", threadid, itminibatch.EdgeLists[1].size());
            // printf("%d::first layer number of edges %d\n", threadid, itminibatch.EdgeLists[0].size());


            // printf("stage3\n");
            itminibatch.start = currentIterationStart;
            itminibatch.end = currentInterationEnd;
            

            return 1;


        }






};



























#endif