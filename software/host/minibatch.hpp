
#ifndef MINIBATCH_DEF
#define MINIBATCH_DEF

#include <stdio.h>
#include <string.h>
#include <random>
#include <ctime>
#include <chrono>

#include "./types.hpp"
#include "./utility.hpp"

class minibatch{
    public:

        // struct edgepair
        // {
        //     index_type source;
        //     index_type destination;
            
        // };

        int numberOfLayers;
        std::vector<std::vector<index_type>> VertexLists;
        std::vector<std::vector<edge_type>> EdgeLists;

        int start;
        int end;
        
        minibatch(){
        }

        // minibatch(int Nlayers){
        //     this->numberOfLayers = Nlayers;
        //     this->VertexLists.resize(Nlayers + 1);
        //     this->EdgeLists.resize(Nlayers);
        // }

        void setnumberoflayers(int Nlayers){
            this->numberOfLayers = Nlayers;
            this->VertexLists.resize(Nlayers + 1);
            this->EdgeLists.resize(Nlayers);
        }




};









#endif