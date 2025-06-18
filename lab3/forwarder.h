#ifndef FORWARDER_ADDR_SC_THREAD_V2_H
#define FORWARDER_ADDR_SC_THREAD_V2_H

#include <systemc.h>
#include <iostream>
#include <vector>

SC_MODULE(Forwarder)
{
    // Ports
    sc_in<bool> ACLK;
    sc_in<bool> ARESETn; // Active-low reset

    // --- Request Interface (Master to Producer) ---
    sc_out<unsigned int> REQ_ID_TO_PROD;
    sc_out<bool> REQ_VALID_TO_PROD;
    sc_in<bool> REQ_READY_FROM_PROD;

    // --- AXI Stream Data Interface (Slave from Producer) ---
    sc_in<bool> S_AXIS_TVALID;
    sc_out<bool> S_AXIS_TREADY;
    sc_in<char> S_AXIS_TDATA;
    sc_in<bool> S_AXIS_TLAST;

    // --- AXI Stream Data Interface (Master to Checker) ---
    sc_out<bool> M_AXIS_TVALID;
    sc_in<bool> M_AXIS_TREADY;
    sc_out<char> M_AXIS_TDATA;
    sc_out<bool> M_AXIS_TLAST;

    const unsigned int TOTAL_STRINGS_TO_REQUEST; // Define how many strings to request

    // hint : You can directly pull S_AXIS_TREADY to 1
    // hint : You can directly set REQ_ID_TO_PROD to 0
    void forwarder_thread_logic()
    {
        // vvvv write your code here vvvv

        S_AXIS_TREADY.write(1);  // continuously ready
        REQ_ID_TO_PROD.write(0); // fixed address

        // reset
        REQ_VALID_TO_PROD.write(0);
        M_AXIS_TVALID.write(0);
        M_AXIS_TDATA.write(0);
        M_AXIS_TLAST.write(0);

        wait();

        while (true)
        {
            // send request to Producer
            REQ_VALID_TO_PROD.write(1);
            do
            {
                wait(); // wait REQ_READY_FROM_PROD high
            } while (!REQ_READY_FROM_PROD.read());
            // REQ_VALID_TO_PROD.write(0); // finish handshake，put down VALID

            // recieve stream data and transfer to Checker
            while (true)
            {
                // cout << "M_AXIS_TREADY: " << M_AXIS_TREADY << endl;
                    
                wait();
                M_AXIS_TVALID = S_AXIS_TVALID;
                M_AXIS_TDATA = S_AXIS_TDATA;
                M_AXIS_TLAST = S_AXIS_TLAST;
                // cout << "S_AXIS_TDATA: " << S_AXIS_TDATA.read() << endl;

                // if (S_AXIS_TVALID.read())
                // {
                //     char data = S_AXIS_TDATA.read();
                //     bool last = S_AXIS_TLAST.read();
                //     cout << "data: " << data << endl;
                //     cout << "last: " << last << endl;
                //     S_AXIS_TREADY.write(0);

                //     // send to Checker
                //     M_AXIS_TDATA.write(data);
                //     M_AXIS_TLAST.write(last);
                //     M_AXIS_TVALID.write(1);

                //     // wait Checker ready
                //     do
                //     {
                //         wait();
                //     } while (!M_AXIS_TREADY.read());
                //     if (!M_AXIS_TREADY.read())
                //     {
                //         M_AXIS_TDATA.write(0);
                //         M_AXIS_TLAST.write(0);
                //         M_AXIS_TVALID.write(0);
                //         S_AXIS_TREADY.write(1);
                //     }

                // }
            }
        }
    }

    // if you need to add more functions, please add them here

    SC_CTOR(Forwarder) : TOTAL_STRINGS_TO_REQUEST(1) // Initialize const member in initializer list
    {
        SC_THREAD(forwarder_thread_logic);
        sensitive << ACLK.pos();
        sensitive << ARESETn.neg();
    }
};

#endif // FORWARDER_ADDR_SC_THREAD_V2_H
