#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "systemc.h"
#include <iostream>
#include <iomanip>

#include <fstream>
#include <vector>
#include <queue>

#include <string>
#include <cmath>
#include <sstream>
#include <algorithm>

using namespace std;

bool compare(const pair<double, int> &a, const pair<double, int> &b)
{
    return a.first > b.first;
}

SC_MODULE(Controller)
{
    sc_in<bool>        clk, rst;

    // to ROM
    sc_out<int>        ARADDR;        // '0' means input data
    // sc_out<bool>       layer_id_type;   // '0' means weight, '1' means bias (for layer_id == 0, we don't care this signal)
    sc_out<bool>       ARVALID;
    sc_in<bool>        ARREADY;
    sc_out<bool>       RREADY;
    // from ROM
    sc_in<float>       RDATA;
    sc_in<bool>        RVALID;

    // to router0
    sc_out<sc_lv<34> > flit_tx;
    sc_out<bool>       req_tx;
    sc_in<bool>        ack_tx;

    // from router0
    sc_in<sc_lv<34> >  flit_rx;
    sc_in<bool>        req_rx;
    sc_out<bool>       ack_rx;

    //--------------------------------------------------------------------------
    int cur_step;
    int next_load;
    int tot_cycle{0};

    vector<float> fc8_result;

    sc_lv<32> float2lv(float d); // Convert float to IEEE 754 logicfield
    float lv2float(sc_lv<32> a); // Convert IEEE 754 logicfield to float

    queue<sc_lv<34> > tx_queue;
    sc_lv<34> send_flit, recieve_flit;

    sc_buffer<bool> RVALID_reg;
    sc_buffer<sc_lv<34> > RDATA_reg;

    bool stop_send;

    void run();

    SC_HAS_PROCESS(Controller);

    Controller(sc_module_name name) : sc_module(name)
    {
        cur_step = 0;
        next_load = 0;
        SC_METHOD(run);
        sensitive << clk.pos();
        dont_initialize();
    }
};

sc_lv<32> Controller::float2lv(const float val)
{
    sc_dt::scfx_ieee_float ieee_val(val);
    sc_lv<32> lv;
    lv[31] = (bool)ieee_val.negative();
    lv.range(30, 23) = ieee_val.exponent();
    lv.range(22, 0) = ieee_val.mantissa();
    return lv;
}

float Controller::lv2float(const sc_lv<32> val)
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

void Controller::run()
{
    if (rst.read())
    {
        ARADDR.write(0);
        // layer_id.write(0);
        // layer_id_type.write(0);
        ARVALID.write(0);
        RREADY.write(0);

        flit_tx.write(0);
        req_tx.write(0);
        ack_rx.write(0);

        cur_step = 0;
        next_load = 0;
        stop_send = 1;
    }
    else
    {
        send_flit = 0;
        send_flit.range(33, 32) = "10";

        static const int dst[] = {
            1, 1, 2, 2, 3, 3, 7, 7,
            6, 6, 5, 5, 4, 4, 12, 12
        };
        
        static const int data_types[] = {
            2, 3, 2, 3, 2, 3, 2, 3,
            2, 3, 2, 3, 2, 3, 2, 3
        };
        
        if (cur_step >= 1 && cur_step <= 16)
        {
        
            send_flit.range(27, 24) = dst[cur_step - 1];
            send_flit.range(23, 22) = data_types[cur_step - 1];
        }
        else if (cur_step == 17)
        {
            send_flit.range(27, 24) = 1;
            send_flit.range(23, 22) = 0;
            send_flit.range(21, 18) = 1;
        }
        
        if (cur_step == 0)
        {
            if (!RVALID.read() && !ARVALID.read() && !ARREADY.read())
            {
                RREADY.write(0);
                if (next_load <= 16)
                    cur_step = next_load + 1;
            }
            else
            {
                // layer_id.write(0);
                ARADDR.write(0);
                // layer_id_type.write(0);
                ARVALID.write(0);
            }
        }
        else if (cur_step == -1)
        {
            ARADDR.write(0);
            // layer_id.write(0);
            // layer_id_type.write(0);
            ARVALID.write(0);

            RREADY.write(1);

            if (RVALID.read())
                cur_step = 0;

            cout << "[INFO] " << "Wait for RVALID | next_load = " << next_load << endl;
        }
        else if (cur_step == 17)
        {
            cout << "[INFO] " << "Fetch Img " << endl;
            tx_queue.push(send_flit);
            ARADDR.write(0);
            // layer_id.write(0);
            // layer_id_type.write(0);
            ARVALID.write(1);
            cur_step = -1;
            next_load++;
        }
        else // (cur_step >= 1 && cur_step <= 16)
        {
            int lid = (cur_step % 2 == 0)? (cur_step / 2) : (cur_step / 2 + 1);
            int type = (cur_step % 2 == 0)? 1 : 0;

            cout << "[INFO] Controller ask for layer_id = " << lid << " | layer_id_type = " << type << endl;
            cout << tx_queue.size() << " flits in tx_queue." << endl;
            tx_queue.push(send_flit);
            ARADDR.write(lid << 1 | type);
            // layer_id.write(lid);
            // layer_id_type.write(type);
            ARVALID.write(1);
            cur_step = -1;
            next_load++;
        }
       

        // RVALID_reg: delay one cycle for RVALID
        if (RVALID.read())
            RDATA_reg.write(float2lv(RDATA.read()));
        

        if (RVALID_reg.read())
        {
            // if (RVALID.read() == 0)
            //     cout << "[INFO] Controller form the tail flit." << endl;
            send_flit.range(33, 32) = (RVALID.read())? "00" : "01";
            send_flit.range(31, 0) = RDATA_reg.read();
            tx_queue.push(send_flit);

        }

        RVALID_reg.write(RVALID.read());

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

            if (recieve_flit[33] != 1)
                fc8_result.push_back(lv2float(recieve_flit.range(31, 0)));
        }

        // ---------- Output port control ----------
        req_tx.write((!tx_queue.empty() && !stop_send)? 1 : 0);
        flit_tx.write((!tx_queue.empty() && !stop_send)? tx_queue.front() : 0);
        ack_rx.write((req_rx.read())? 1 : 0);
        // if (!tx_queue.empty()){
        //     cout << "[INFO] Controller send flit: " << tx_queue.front().range(33, 32) << endl;
        // }
        if (!tx_queue.empty() && tx_queue.front()[32] == 1)
            stop_send = 1;
        else
            stop_send = 0;

        if (fc8_result.size() == 1000)
        {
            ifstream class_file("data/imagenet_classes.txt");
            vector<string> class_name;
            string class_name_element;
            while (getline(class_file, class_name_element))
            {
                class_name.push_back(class_name_element);
            }
            class_file.close();

		    vector<pair<double, int> > indexed_values;

            float tmp;
            float exp_sum{0};

            for (int i = 0; i < 1000; ++i)
                exp_sum += exp(fc8_result[i]);

            for (int i = 0; i < 1000; ++i)
            {
                tmp = (exp(fc8_result[i])) / (exp_sum);
                indexed_values.push_back(make_pair(tmp, i));
            }

            sort(indexed_values.begin(), indexed_values.end(), compare);

            cout << "Top 100 classes:" << endl;
            cout << "Total latency: " << tot_cycle << endl;
            cout << "=================================================" << endl;
            cout << fixed << setprecision(2);
            cout << right << setw(5) << "idx"
                 << " | " << setw(8) << "val"
                 << " | " << setw(11) << "possibility"
                 << " | " << "class name" << endl;
            cout << "-------------------------------------------------" << endl;

            for (int i = 0; i < 100; ++i)
            {
                cout << right << setw(5) << indexed_values[i].second
                     << " | " << setw(8) << double(fc8_result[indexed_values[i].second])
                     << " | " << setw(11) << (indexed_values[i].first * 100)
                     << " | " << class_name[indexed_values[i].second] << endl;
            }

            cout << "=================================================" << endl;
            cout << "Total latency: " << tot_cycle << endl;

            indexed_values.clear();
            fc8_result.clear();
            sc_stop();
        }
    }
    tot_cycle++;
}

#endif