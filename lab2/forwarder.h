#ifndef FORWARDER_H
#define FORWARDER_H

#include <systemc.h>

SC_MODULE(Forwarder) {
    // Ports
    sc_in<bool> ACLK;
    sc_in<bool> ARESETn;

    // Slave Interface (from Producer)
    sc_in<bool>         S_AXIS_TVALID;
    sc_out<bool>        S_AXIS_TREADY;
    sc_in<char>         S_AXIS_TDATA; 
    sc_in<bool>         S_AXIS_TLAST;

    // Master Interface (to Checker)
    sc_out<bool>        M_AXIS_TVALID;
    sc_in<bool>         M_AXIS_TREADY;
    sc_out<char>        M_AXIS_TDATA;
    sc_out<bool>        M_AXIS_TLAST;

    //hint : You can directly pull S_AXIS_TREADY to 1
    void forward_data_proc() {
        //vvvv write your code here vvvv
        M_AXIS_TVALID = S_AXIS_TVALID;
        S_AXIS_TREADY = M_AXIS_TREADY;
        M_AXIS_TDATA = S_AXIS_TDATA;
        M_AXIS_TLAST = S_AXIS_TLAST;
    }

    //if you need to add more functions, please add them here


    SC_CTOR(Forwarder)
    {
        SC_METHOD(forward_data_proc);
        sensitive << ACLK.pos() << ARESETn.neg();

    }
};

#endif // FORWARDER_H