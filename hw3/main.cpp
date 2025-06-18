#include "systemc.h"
#include "clockreset.h"

#include "core.h"
#include "router.h"

int sc_main(int argc, char* argv[])
{
    // =======================
    //   signals declaration
    // =======================
    sc_signal<bool> clk, rst_n;

    
    sc_signal<sc_lv<34> > flit_rx[16];      // The input channel
    sc_signal<bool>       req_rx[16];       // The request associated with the input channel
    sc_signal<bool>       ack_rx[16];       // The outgoing ack signal associated with the input channel
    // transmit
    sc_signal<sc_lv<34> > flit_tx[16];      // The output channel
    sc_signal<bool>       req_tx[16];       // The request associated with the output channel
    sc_signal<bool>       ack_tx[16];       // The outgoing ack signal associated with the output channel
    
    // Router: Input ports
    sc_signal<sc_lv<34> > in_flit_buf[16][5];
    sc_signal<bool>       in_req_buf[16][5];
    sc_signal<bool>       in_ack_buf[16][5];

    // =======================
    //   modules declaration
    // =======================
    Clock m_clock("m_clock", 10);
    Reset m_reset("m_reset", 15);
    

    // =======================
    //   modules connection
    // =======================
    m_clock(clk);
    m_reset(rst_n);

    Core *core_ptr[16];
    Router *router_ptr[16];

    // string
    std::ostringstream ss;

    for (int i = 0; i < 16; ++i)
    {
        ss << i;
        std::string id_str = ss.str();
        core_ptr[i] = new Core(("m_Core_" + id_str).c_str(), i);
        router_ptr[i] = new Router(("m_Router_"+ id_str).c_str(), i);

        core_ptr[i]->clk(clk);
        core_ptr[i]->rst_n(rst_n);
        core_ptr[i]->flit_rx(flit_rx[i]);
        core_ptr[i]->req_rx(req_rx[i]);
        core_ptr[i]->ack_rx(ack_rx[i]);
        core_ptr[i]->flit_tx(flit_tx[i]);
        core_ptr[i]->req_tx(req_tx[i]);
        core_ptr[i]->ack_tx(ack_tx[i]);
    }

    unsigned int L_id, R_id, U_id, D_id;

    for (int i = 0; i < 4; ++i) // row
    {
        for (int j = 0; j < 4; ++j) // col
        {
            L_id = (j + 3) % 4;
            R_id = (j + 1) % 4;
            U_id = (i + 3) % 4;
            D_id = (i + 1) % 4;

            int id = i * 4 + j;

            router_ptr[id]->clk(clk);
            router_ptr[id]->rst_n(rst_n);

            // Router In
            router_ptr[id]->in_flit[0](in_flit_buf[id][0]); // Left
            router_ptr[id]->in_flit[1](in_flit_buf[id][1]); // Right
            router_ptr[id]->in_flit[2](in_flit_buf[id][2]); // Up
            router_ptr[id]->in_flit[3](in_flit_buf[id][3]); // Down
            router_ptr[id]->in_flit[4](flit_tx[id]);        // Core
            
            router_ptr[id]->in_req[0](in_req_buf[id][0]);
            router_ptr[id]->in_req[1](in_req_buf[id][1]);
            router_ptr[id]->in_req[2](in_req_buf[id][2]);
            router_ptr[id]->in_req[3](in_req_buf[id][3]);
            router_ptr[id]->in_req[4](req_tx[id]);       

            router_ptr[id]->in_ack[0](in_ack_buf[id][0]);
            router_ptr[id]->in_ack[1](in_ack_buf[id][1]);
            router_ptr[id]->in_ack[2](in_ack_buf[id][2]);
            router_ptr[id]->in_ack[3](in_ack_buf[id][3]);
            router_ptr[id]->in_ack[4](ack_rx[id]);       

            // Router out
            router_ptr[id]->out_flit[0](in_flit_buf[i * 4 + L_id][1]); // Left[0] output connect to Right[1] input
            router_ptr[id]->out_flit[1](in_flit_buf[i * 4 + R_id][0]); // Right
            router_ptr[id]->out_flit[2](in_flit_buf[U_id * 4 + j][3]); // Up
            router_ptr[id]->out_flit[3](in_flit_buf[D_id * 4 + j][2]); // Down
            router_ptr[id]->out_flit[4](flit_rx[id]);                  // Core
 
            router_ptr[id]->out_req[0](in_req_buf[i * 4 + L_id][1]);
            router_ptr[id]->out_req[1](in_req_buf[i * 4 + R_id][0]);
            router_ptr[id]->out_req[2](in_req_buf[U_id * 4 + j][3]);
            router_ptr[id]->out_req[3](in_req_buf[D_id * 4 + j][2]);
            router_ptr[id]->out_req[4](req_rx[id]);                 

            router_ptr[id]->out_ack[0](in_ack_buf[i * 4 + L_id][1]);
            router_ptr[id]->out_ack[1](in_ack_buf[i * 4 + R_id][0]);
            router_ptr[id]->out_ack[2](in_ack_buf[U_id * 4 + j][3]);
            router_ptr[id]->out_ack[3](in_ack_buf[D_id * 4 + j][2]);
            router_ptr[id]->out_ack[4](ack_tx[id]);                 


        }
    }
    sc_start(10000, SC_NS);

    for (int i = 0; i < 16; ++i)
    {
        delete core_ptr[i];
        delete router_ptr[i];
    }

    return 0;
}