#include "systemc.h"
#include "clockreset.h"
#include <iostream>

#include "core.h"
#include "router.h"
#include "controller.h"
#include "ROM.h"

int sc_main(int argc, char *argv[])
{
    // =======================
    //   signals declaration
    // =======================
    sc_signal<bool> clk, rst;

    // Controller
    sc_signal<int>        layer_id;         // '0' means input data
    sc_signal<bool>       layer_id_type;    // '0' means weight, '1' means bias (for layer_id == 0, we don't care this signal)
    sc_signal<bool>       layer_id_valid;

    sc_signal<float>      data;
    sc_signal<bool>       data_valid;

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
    Controller m_controller("m_controller");
    ROM m_rom("m_rom");

    // =======================
    //   modules connection
    // =======================

    Core *core_ptr[15];
    Router *router_ptr[16];

    m_clock(clk);
    m_reset(rst);

    // m_rom
    m_rom.clk(clk);
    m_rom.rst(rst);
    m_rom.layer_id(layer_id);
    m_rom.layer_id_type(layer_id_type);
    m_rom.layer_id_valid(layer_id_valid);
    m_rom.data(data);
    m_rom.data_valid(data_valid);

    // m_controller
    m_controller.rst(rst);
    m_controller.clk(clk);
    m_controller.layer_id(layer_id);
    m_controller.layer_id_type(layer_id_type);
    m_controller.layer_id_valid(layer_id_valid);
    m_controller.data(data);
    m_controller.data_valid(data_valid);
    m_controller.flit_rx(flit_rx[0]);
    m_controller.req_rx(req_rx[0]);
    m_controller.ack_rx(ack_rx[0]);
    m_controller.flit_tx(flit_tx[0]);
    m_controller.req_tx(req_tx[0]);
    m_controller.ack_tx(ack_tx[0]);

    // string
    std::ostringstream ss;

    for (int i = 0; i < 15; ++i)
    {
        ss << i + 1;
        std::string id_str = ss.str();
        core_ptr[i] = new Core(("m_Core_" + id_str).c_str(), i + 1);
        core_ptr[i]->clk(clk);
        core_ptr[i]->rst(rst);
        core_ptr[i]->flit_rx(flit_rx[i + 1]);
        core_ptr[i]->req_rx(req_rx[i + 1]);
        core_ptr[i]->ack_rx(ack_rx[i + 1]);
        core_ptr[i]->flit_tx(flit_tx[i + 1]);
        core_ptr[i]->req_tx(req_tx[i + 1]);
        core_ptr[i]->ack_tx(ack_tx[i + 1]);
    }

    for (int i = 0; i < 16; ++i)
    {
        ss << i;
        std::string id_str = ss.str();
        router_ptr[i] = new Router(("m_Router_"+ id_str).c_str(), i);
    }

    unsigned int L_id, R_id, U_id, D_id;

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            L_id = (j + 3) % 4;
            R_id = (j + 1) % 4;
            U_id = (i + 3) % 4;
            D_id = (i + 1) % 4;

            int id = i * 4 + j;

            router_ptr[id]->clk(clk);
            router_ptr[id]->rst(rst);
            
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
    sc_start();

    for (int i = 0; i < 15; ++i)
    {
        delete core_ptr[i];
        delete router_ptr[i];
    }
    delete router_ptr[15];
    
    return 0;
}