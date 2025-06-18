#ifndef ROM_H
#define ROM_H

#include "systemc.h"
#include <iostream>
#include <fstream>

#define FLIT_GROUP 32
// #define FLIT_GROUP 60

using namespace std;

SC_MODULE( ROM ) {
    sc_in  < bool >  clk;
    sc_in  < bool >  rst;
 
    sc_in  < int >   ARADDR;       // '0' means input data
    // sc_in  < int >   layer_id;       // '0' means input data
    // sc_in  < bool >  layer_id_type;  // '0' means weight, '1' means bias (for layer_id == 0, we don't care this signal)
    sc_in  < bool >  ARVALID;
    sc_out < bool >  ARREADY;
    sc_in  < bool >  RREADY;
    
    sc_out < float > RDATA[32];
    sc_out < bool >  RVALID;

    // AXI4 signals
    // sc_in  < bool >  arvalid;  // Request to read a new layer
    // sc_out < bool >  arready;  // ROM is ready to accept request
    // sc_out < bool >  rvalid;   // Data is valid
    // sc_in  < bool >  rready;   // PE is ready to receive data

    void run();
    // vvv Please don't remove these two variables vvv
    string DATA_PATH ;
    string IMAGE_FILE_NAME;     
    // ^^^ Please don't remove these two variables ^^^

    SC_CTOR( ROM )
    {
        DATA_PATH = "../data/";      // Please change this to your own data path

        const char* env_file = getenv("IMAGE_FILE_NAME");
        //IMAGE_FILE_NAME = "cat.txt"; // You can change this to test another image file
        if (env_file != NULL) {
            IMAGE_FILE_NAME = env_file;
        } else {
            IMAGE_FILE_NAME = "cat.txt"; // default fallback
        }

        SC_THREAD( run );
        sensitive << clk.pos() << rst.neg();
    }
};

#endif