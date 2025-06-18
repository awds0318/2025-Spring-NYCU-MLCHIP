#ifndef ROUTER_H
#define ROUTER_H

#define FIFO_SIZE 3

#include "systemc.h"

#include <queue>

using namespace std;




SC_MODULE( Router ) {
    sc_in <bool>       clk, rst;

    sc_out<sc_lv<34> > out_flit[5];
    sc_out<bool>       out_req[5];
    sc_in <bool>       in_ack[5];

    sc_in< sc_lv<34> > in_flit[5];
    sc_in<bool>        in_req[5];
    sc_out<bool>       out_ack[5];
    
    int router_id;

    int s_row, s_col;        // source
    int d_row[5], d_col[5];  // destination
    int dif_r[5], dif_c[5];
    int dst_id[5];
    int curr_dst[5];
    bool output_busy[5];
    int occupy_by_input[5];
    sc_lv <34> in_flit_reg[5];

    queue <sc_lv<34> > input_fifo[5];

    const int LEFT{0}, RIGHT{1}, UP{2}, DOWN{3}, CORE{4};
    
    void run();
    
    SC_HAS_PROCESS(Router);

	Router(
		sc_module_name name, 
		int router_id
	) : 
		sc_module(name),
		router_id(router_id)
	{
		SC_METHOD(run);
        dont_initialize();
		sensitive << clk.pos();
	}
};

void Router::run()
{
    // L->R->U->D->C
    s_row = (router_id) / 4;
    s_col = (router_id) % 4;

    // ========== for input port ============
    for(int i = 0; i < 5; ++i)
    {
        if(rst.read())
        {
            in_flit_reg[i] = 0;
            dst_id[i] = i;
            curr_dst[i] = i;
            output_busy[i] = false;
            occupy_by_input[i] = i;
        }
        else
        {
            if (in_req[i].read()) // When a flit is passed in
            {
                in_flit_reg[i] = in_flit[i].read();

                if (in_flit_reg[i][33] == 1)  // header flit
                {
                    dst_id[i] = in_flit_reg[i].range(27, 24).to_uint();

                    d_row[i] = dst_id[i] / 4;
                    d_col[i] = dst_id[i] % 4;

                    dif_r[i] = (4 + int(d_row[i]) - int(s_row)) % 4;
                    dif_c[i] = (4 + int(d_col[i]) - int(s_col)) % 4;

                    // Simplified version of XY routing (prefer lateral movement)
                    // If they are in the same column, use the row difference to decide whether to go up or down. Otherwise, use the column difference to decide whether to go left or right.
                    if (dst_id[i] == uint32_t(router_id))
                        curr_dst[i] = CORE;
                    else if (dif_c[i] == 0)
                        curr_dst[i] = (dif_r[i] <= 2)? DOWN : UP;
                    else
                        curr_dst[i] = (dif_c[i] <= 2)? RIGHT : LEFT;
                    
                    if (!output_busy[curr_dst[i]] && input_fifo[i].empty()) // If the direction is idle, the flit is allowed to be forwarded
                    {
                        output_busy[curr_dst[i]] = true;
                        occupy_by_input[curr_dst[i]] = i;

                        input_fifo[i].push(in_flit_reg[i]);
                        out_ack[i].write(1);  // Response I received
                    }
                }
                else // body & tail flit
                {
                    if (out_ack[i].read())
                        input_fifo[i].push(in_flit_reg[i]);

                    if (input_fifo[i].size() > FIFO_SIZE)
                        out_ack[i].write(0);
                    else
                        out_ack[i].write(1);
                }
            }
            else
                out_ack[i].write(0); // No flit received, so write 0
        }
    }

    // ========== for output port ============
    for(int i = 0; i < 5; ++i)
    {
        if(rst.read())
        {
            out_req[i].write(0);
            out_ack[i].write(0);
            out_flit[i].write(0);
        }
        else
        {
            if(output_busy[i])
            // An input port (recorded in occupy_by_input[i]) has claimed this output port.
            // A flit (packet fragment) from this input port may be ready to be sent through this output.
            // But it does not mean that there must be data in FIFO that can be sent immediately!
            {
                queue<sc_lv<34> >& fifo = input_fifo[occupy_by_input[i]];

                if (!fifo.empty())  // There is data to send
                {
                    if (out_req[i].read() && in_ack[i].read())
                    {
                        
                        if (fifo.front()[32] == 1) // Transmission completed
                        {
                            fifo.pop();
                            output_busy[i] = false;
                            occupy_by_input[i] = i;
                            out_flit[i].write(0);
                            out_req[i].write(0);
                        }
                        else
                        {
                            fifo.pop();
                            out_flit[i].write((fifo.empty())? 0 : fifo.front());
                            out_req[i].write((fifo.empty())? 0 : 1);
                        }
                    }
                    else
                    {
                        out_flit[i].write(fifo.front());
                        out_req[i].write(1);
                    }
                }
                else // FIFO empty
                {
                    out_flit[i].write(0);
                    out_req[i].write(0);
                }
            }
        }
    }
}

#endif