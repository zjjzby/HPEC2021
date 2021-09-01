
#ifndef DATA_TYPE_DEF
#define DATA_TYPE_DEF


#include <stdio.h>
#include <ap_int.h>
#include <ostream>
#include <hls_stream.h>
#include <string.h>

#include "./utility.hpp"

#define VDATA_SIZE 16
#define DST_SIZE 10240

typedef float feature_type;
typedef int index_type;
typedef ap_uint<1> endflag;





// ******************************************
// the data type on FPGA side 




struct v_datatype {
    feature_type data[VDATA_SIZE];
};

struct v_datatype_onchip {
    v_datatype data;
    index_type src;
};


struct edge_type {
    index_type src;
    index_type dst;
};

struct edge_onchip_type {
    index_type src;
    index_type dst;
    feature_type value;
    ap_uint<1> flag; // valid flag
    ap_uint<1> end;  // last batch
};

struct v_edge_src {
    index_type src[8];
    ap_uint<1> flags[8]; 
};


struct v_edges {
    edge_type edges[8];
};

struct v_onchip_edges {
    edge_onchip_type edges[8];
};

struct edge_value {
    feature_type data;
};

struct v_edge_value
{
    edge_value edgevalues[VDATA_SIZE];
};


struct update_type {
    index_type dst;
    v_datatype value;
    ap_uint<1> flag; // valid flag
    ap_uint<1> end;  // last batch
    ap_uint<1> valid;
};

// ******************************************
// data structure for profiling

struct timeProfiler {
    float samplingTime;
    float dataTransmissionTime;
    float GNNOperationTime;
};






// *****************************************
// the data type on host side

#define MAXDEGREE 128
#define FEALENGTH 200

// adjacency table

struct table_element {
    index_type node_id;
    feature_type edge_weight;
};


struct adj_table_entry {
    index_type src_id;
    std::vector<index_type> neighbors;
};

struct feature_table_entry
{
    index_type src_id;
    feature_type features[FEALENGTH];
};




class GraphObj {
    public:
        uint32_t Nofvertices;
        uint32_t Nofedges;
        //std::vector<adj_table_entry> adj_table;
        //std::vector<feature_table_entry> feature_table;
        std::string graphfilename;

        // graph in COO format
        std::vector<index_type> coo_row_indices;
        std::vector<index_type> coo_col_indices;
        std::vector<feature_type> coo_values;

        // graph in CSR format
        std::vector<index_type> CSR_indptr;
        std::vector<index_type> CSR_indices;
        
        GraphObj(){
        };
        GraphObj(std::string &edge_file_name){

            this->graphfilename = edge_file_name;

            int A_nrows, A_ncols, nnz;

            readMtx<float>(graphfilename.c_str(), this->coo_row_indices, this->coo_col_indices, coo_values, A_nrows, A_ncols, nnz);
            
            this->Nofvertices = A_nrows;
            this->Nofedges = nnz;

            this->CSR_indices = this->coo_col_indices;

            this->CSR_indptr.resize(A_nrows + 1);



            index_type itrowindice = this->coo_row_indices[0];
            index_type itpointer = 0;
            
            this->CSR_indptr[0] = itpointer;
            int k =1;
            int j = 0;

            for (j = 0; j < nnz ;j++){
                if(itrowindice != this->coo_row_indices[j]){
                    itrowindice = this->coo_row_indices[j];
                    this->CSR_indptr[k] = j;
                    k++;
                }
            }

            printf("the k is %d\n", k);
            printf("number of vertices is %d\n", this->Nofvertices);

            printf("the j is %d\n", j);
            printf("number of edges is %d\n", this->Nofedges);

            for(j = k; j <=this->Nofvertices ; j++){
                this->CSR_indptr[j] = nnz;
            }

            // for(int i = 0; i< 32; i++){
            //     if(this->CSR_indptr[i] >= this->CSR_indptr[i+1]){
            //         printf("the indtpr[%d] is %d\n",i,  this->CSR_indptr[i]);
            //         printf("the indtpr[%d] is %d\n",i+1,  this->CSR_indptr[i+1]);
            //     }
            // }

            // for(int i = 0; i< 32; i++){
            //     printf("the edges are is (%d, %d)\n", this->coo_row_indices[i], this->coo_col_indices[i]);
                
            // }
        }


        // void Generate_adjtable(){
        //     this->adj_table.resize(Nofvertices);

        //     for(int i = 0; i< Nofvertices; i++){
        //         adj_table[i].src_id = i;
        //         adj_table[i].neighbors[]
        //     }
            

        // }
};







#endif
