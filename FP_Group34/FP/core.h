#ifndef CORE_H
#define CORE_H
#define SC_INCLUDE_FX

#include "systemc.h"
#include "pe.h"
//#include "pe_load.h"

#include <iostream>
#include <fstream>
#include <vector>

#include <string>
#include <sstream>
#include <cmath>

#include <string>
#include <algorithm>
#include <bitset>
#include <queue>

using namespace std;

SC_MODULE( Core ) {
    sc_in<bool>        clk, rst;
    // receive
    sc_in<sc_lv<34> >  flit_rx;	  // The input channel
    sc_in<bool>        req_rx;	  // The request associated with the input channel
    sc_out<bool>       ack_rx;	  // The outgoing ack signal associated with the input channel
    // transmit
    sc_out<sc_lv<34> > flit_tx;   // The output channel
    sc_out<bool>       req_tx;	  // The request associated with the output channel
    sc_in<bool>        ack_tx;	  // The outgoing ack signal associated with the output channel
  
    PE pe;
    
    int core_id;
    int cnt{0};
    sc_lv<32> float2lv(float d); // Convert float to IEEE 754 logicfield
    float lv2float(sc_lv<32> a); // Convert IEEE 754 logicfield to float

    queue <sc_lv<34> > tx_queue;
    sc_lv<34> send_flit, recieve_flit;

    Packet* packet2pe;
    Packet* packet2router; // packet from pe
    
    void run();

    SC_HAS_PROCESS(Core);

	Core(
		sc_module_name name, 
		int core_id
	) : 
		sc_module(name),
		core_id(core_id),
        pe()
	{
        pe.init(core_id);
		SC_METHOD(run);
        dont_initialize();
		sensitive << clk.pos();
	}
};

sc_lv<32> Core::float2lv(const float val)
{
    sc_dt::scfx_ieee_float ieee_val(val);
    sc_lv<32> lv;
    lv[31] = (bool)ieee_val.negative();
    lv.range(30, 23) = ieee_val.exponent();
    lv.range(22, 0) = ieee_val.mantissa();
    return lv;
}

float Core::lv2float(const sc_lv<32> val)
{
    sc_dt::scfx_ieee_float id;
    sc_lv<32> sign = 0;
    sc_lv<32> exp = 0;
    sc_lv<32> mantissa = 0;
    sign[0] = val[31];
    exp.range(7, 0) = val.range(30, 23);
    mantissa.range(22, 0) = val.range(22, 0);
    id.negative(sign.to_uint());
    id.exponent(exp.to_int());
    id.mantissa(mantissa.to_uint());
    return float(id);
}



void Core::run(){
    if(rst.read())
    {
        // cout << "core rst" << endl;
    }
    else
    {
        if (tx_queue.empty()) // The queue is empty. Prepare to get a packet from PE. Otherwise, wait until the queue is finished before getting one.
        {
            packet2router = pe.get_packet(); // Take out a packet
            if(packet2router != NULL)
            {
                // [1] header flit
                send_flit = 0;
                send_flit.range(33,32) = "10";
                send_flit.range(31, 28) = packet2router->source_id;
                send_flit.range(27, 24) = packet2router->dest_id;

                if(packet2router->is_parameter)
                {
                    if(packet2router->is_bias) send_flit.range(23,22) = 3;
                    else                       send_flit.range(23,22) = 2;
                }
                else
                    send_flit.range(23,22)  = 0;

                cout << "is_parameter= " << packet2router->is_parameter << endl;
                cout << "is_bias= " << packet2router->is_bias << endl;

                send_flit.range(21,18)  = packet2router->op_id;
                tx_queue.push(send_flit);

                // [2] body/tail flits
                // if(packet2router->datas.size() == 0)
                //     cout << "[Fatal] PE return datas is empty!"<<endl;
                
                
                for(int i = 0; i < (packet2router->datas).size(); ++i)
                {
                    if(i == ((packet2router->datas).size() - 1)) 
                        cout << "[INFO] Core_" << packet2router->source_id << " form the tail flit." << endl;

                    send_flit = 0;
                    send_flit.range(33, 32) = ((i == (packet2router->datas.size() - 1))? "01" : "00");
                    send_flit.range(31, 0) = float2lv(packet2router->datas[i]);
                    tx_queue.push(send_flit);
                }

            }
        }
            
        // req_tx: Request Transmit
        // Indicates that the Core has data (flit) to send, please prepare the Router to receive it.
        // Core pulls req_tx high to indicate "I want to send data"

        // ack_tx: Acknowledge Transmit
        // Indicates that the Router is ready to receive flit.
        // When req_tx == 1 and ack_tx == 1, Core can safely send the flit
        if (ack_tx.read() && req_tx.read()) // If the previous flit is received by the Router (ack_tx==1), pop the flit
            tx_queue.pop();

        // ---------- receive data：Router → Core → PE ----------
        // req_rx: Request Receive
        // Indicates that the sender (Router) is ready to send a flit to the Core
        // This signal is pulled high by the Router, indicating that I have data to send to you

        // ack_rx: Acknowledge Receive
        // Indicates that the receiving end (Core) is ready to receive flit

        if (req_rx.read() && ack_rx.read()) // When req_rx == 1 && ack_rx == 1, it means both parties are ready, and the flit can be successfully transmitted from the Router to the Core.
        {
            recieve_flit = flit_rx.read();

            if (recieve_flit[33] == 1) // header flit: start a new packet
            {
                cout << "recieve_flit: " << recieve_flit.range(33, 32) << endl;
                packet2pe = new Packet();
          
                packet2pe->source_id = recieve_flit(31,28).to_uint();
                packet2pe->dest_id = recieve_flit(27,24).to_uint();
                packet2pe->is_parameter = recieve_flit[23].to_bool();
                packet2pe->is_bias = recieve_flit[22].to_bool();

                packet2pe->op_id = recieve_flit(21,18).to_uint();

                packet2pe->datas.clear();
                
                cout << "Source: " << packet2pe->source_id << ", Dst: " << packet2pe->dest_id << endl;

            }
            else // body flit & tail flit
            {
                cnt++;
                packet2pe->datas.push_back(lv2float(recieve_flit.range(31, 0)));
                
                if (recieve_flit[32] == 1)
                {
                    cout << cnt << "th tail flit received." << endl;
                    cnt = 0;
                    pe.check_packet(packet2pe);
                    delete packet2pe;
                }
            }
        }

        // ---------- Output port control ----------
        req_tx.write((tx_queue.empty())? 0 : 1);
        flit_tx.write((tx_queue.empty())? 0 : tx_queue.front());
        ack_rx.write((req_rx.read())? 1 : 0);
        // cout << ((tx_queue.size() > 0)) << endl;
    }
}

#endif