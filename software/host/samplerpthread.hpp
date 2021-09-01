

#ifndef SAMPLER_DEF
#define SAMPLER_DEF


#include <stdio.h>
#include <string.h>
#include <random>
#include <ctime>
#include <chrono>
#include <omp.h>
#include <pthread.h>

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


class neighborhoodsampler: public basicsampler {
    public:
        std::vector<adj_table_entry> adj_table;
        std::vector<feature_table_entry> feature_table;

        struct range
        {
            int start;
            int end;
            neighborhoodsampler * myself;
        };


        neighborhoodsampler(std::string &edge_file_name): basicsampler(edge_file_name){
           
        }

        static void * Generate_adjtable_iteration(void *threadarg){


            struct range * threadrange;
	        threadrange = (struct range *) threadarg;

	        int start = threadrange->start;
	        int end = threadrange->end;
            for(int i = start; i<end; i++){
                unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                // printf("step 3\n");
                std::srand ( unsigned ( std::time(0) ) );
                // printf("proceed in vertices %d\n", i);
                int number_of_neighbors = threadrange->myself->mygraph.CSR_indptr[i+1] - threadrange->myself->mygraph.CSR_indptr[i];
                threadrange->myself->adj_table[i].src_id = i;
                threadrange->myself->adj_table[i].neighbors.resize(MAXDEGREE);
                std::default_random_engine generator (seed);
                // printf("step 6\n");
                if (number_of_neighbors > MAXDEGREE){
                    // printf("step 7\n");
                    std::random_shuffle(
                        threadrange->myself->mygraph.CSR_indices.begin() + threadrange->myself->mygraph.CSR_indptr[i], 
                        threadrange->myself->mygraph.CSR_indices.begin() + threadrange->myself->mygraph.CSR_indptr[i+1]
                    );
                    std::copy(
                        threadrange->myself->mygraph.CSR_indices.begin() + threadrange->myself->mygraph.CSR_indptr[i], 
                        threadrange->myself->mygraph.CSR_indices.begin() + threadrange->myself->mygraph.CSR_indptr[i] + MAXDEGREE,
                        threadrange->myself->adj_table[i].neighbors.begin()
                    );
                }
                else{
                    // printf("step 8 (%d, %d)\n", this->mygraph.CSR_indptr[i], this->mygraph.CSR_indptr[i+1]);
                    std::random_shuffle(
                        threadrange->myself->mygraph.CSR_indices.begin() + threadrange->myself->mygraph.CSR_indptr[i], 
                        threadrange->myself->mygraph.CSR_indices.begin() + threadrange->myself->mygraph.CSR_indptr[i+1]
                    );
                    // printf("step 9\n");
                    std::copy(
                        threadrange->myself->mygraph.CSR_indices.begin() + threadrange->myself->mygraph.CSR_indptr[i], 
                        threadrange->myself->mygraph.CSR_indices.begin() + threadrange->myself->mygraph.CSR_indptr[i] + number_of_neighbors,
                        threadrange->myself->adj_table[i].neighbors.begin()
                    );
                    // printf("step 10\n");
                    if(threadrange->myself->mygraph.CSR_indptr[i+1]  > threadrange->myself->mygraph.CSR_indptr[i]){
                        std::uniform_int_distribution<int> distribution(int(threadrange->myself->mygraph.CSR_indptr[i]), int(threadrange->myself->mygraph.CSR_indptr[i+1] -1));
                        for(int j = number_of_neighbors; j <MAXDEGREE; j++){
                            threadrange->myself->adj_table[i].neighbors[j] = threadrange->myself->mygraph.CSR_indices[distribution(generator)];
                        }
                    }
                    else{
                        std::uniform_int_distribution<int> distribution(int(threadrange->myself->mygraph.CSR_indptr[i]), int(threadrange->myself->mygraph.CSR_indptr[i]));
                        for(int j = number_of_neighbors; j <MAXDEGREE; j++){
                            threadrange->myself->adj_table[i].neighbors[j] = threadrange->myself->mygraph.CSR_indices[distribution(generator)];
                        }
                    }
                    
                    
                }
            }
        }

        void Generate_adjtable(){
            // printf("step 1\n");
            this->adj_table.resize(this->mygraph.Nofvertices);
            // printf("step 2\n");
            int Nthreads = 2;
            // printf("step 4\n");
            pthread_t thread[Nthreads];
	        pthread_attr_t attr;
	        pthread_attr_init(&attr);
	        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

            int interval = this->mygraph.Nofvertices/Nthreads;
            int rc;
            // printf("step 5\n");
            // #pragma omp parallel for num_threads(16) schedule(static, 10240) 
            
            struct range irange[Nthreads];
            for(int i = 0;i < Nthreads; i++){
                if(i == Nthreads - 1){
                    irange[i].start = i*interval;
                    irange[i].end = this->mygraph.Nofvertices;
                    irange[i].myself = this;
                    rc = pthread_create(&thread[i], &attr, Generate_adjtable_iteration, (void*)&irange[i]);
                    if(rc){printf("creating error");exit(-1);}
                }
                else{
                    irange[i].start = i*interval;
                    irange[i].end = (i+1)*interval;
                    irange[i].myself = this;
                    rc = pthread_create(&thread[i], &attr, Generate_adjtable_iteration, (void*)&irange[i]);
                    if(rc){printf("creating error");exit(-1);}
                }
            }

            pthread_attr_destroy(&attr);
            for(int i=0; i < Nthreads ; i++){
                rc = pthread_join(thread[i], NULL);
                if(rc){printf("join error %d", rc);exit(-1);}
               
            }

            
        }




};



























#endif