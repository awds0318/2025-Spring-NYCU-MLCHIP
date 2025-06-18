#include "layer.h"
#include "Pattern.h"

#include <systemc.h>
#include <iostream>


int sc_main(int argc, char *argv[])
{	
	string img_name = argv[1];
    sc_clock clk("clk", 1, SC_NS);
    sc_signal<bool> reset;

    // You can add your code here
	sc_signal<bool> in_valid_pat;
	sc_signal<bool> out_valid_conv0;
	sc_signal<bool> out_valid_max0;
	sc_signal<bool> out_valid_conv1;
	sc_signal<bool> out_valid_max1;
	sc_signal<bool> out_valid_conv2;
	sc_signal<bool> out_valid_conv3;
	sc_signal<bool> out_valid_conv4;
	sc_signal<bool> out_valid_max2;
	sc_signal<bool> out_valid_linear0;
	sc_signal<bool> out_valid_linear1;
	sc_signal<bool> out_valid_linear2;
	sc_signal<bool> out_valid_softmax;
	
	sc_vector<sc_fifo<double> > img_out("img_out", 150528);
	sc_vector<sc_fifo<double> > conv0_out("conv0_out", 193600);
	sc_vector<sc_fifo<double> > max0_out("max0_out", 46656);
	sc_vector<sc_fifo<double> > conv1_out("conv1_out", 139968);
	sc_vector<sc_fifo<double> > max1_out("max1_out", 32448);
	sc_vector<sc_fifo<double> > conv3_out("conv3_out", 43264);
	sc_vector<sc_fifo<double> > conv2_out("conv2_out", 64896);
	sc_vector<sc_fifo<double> > conv4_out("conv4_out", 43264);
	sc_vector<sc_fifo<double> > max2_out("max2_out", 9216);
	sc_vector<sc_fifo<double> > linear0_out("linear0_out", 4096);
	sc_vector<sc_fifo<double> > linear1_out("linear1_out", 4096);
	sc_vector<sc_fifo<double> > linear2_out("linear2_out", 1000);
	sc_vector<sc_fifo<double> > softmax_out("softmax_out", 1000);
	sc_vector<sc_fifo<double> > softmax_linear("softmax_linear", 1000);

	Pattern m_Pattern("m_Pattern", img_name);

	CONV m_CONV1("m_CONV1", 3, 224, 224, 64, 55, 55, 4, 11, 2, "../data/conv1_weight.txt", "../data/conv1_bias.txt");
	MP m_MP1("m_MP1", 64, 55, 2, 64, 27);
	CONV m_CONV2("m_CONV2", 64, 27, 27, 192, 27, 27, 1, 5, 2, "../data/conv2_weight.txt", "../data/conv2_bias.txt");
	MP m_MP2("m_MP2", 192, 27, 2, 192, 13);
	CONV m_CONV3("m_CONV3", 192, 13, 13, 384, 13, 13, 1, 3, 1, "../data/conv3_weight.txt", "../data/conv3_bias.txt");
	CONV m_CONV4("m_CONV4", 384, 13, 13, 256, 13, 13, 1, 3, 1, "../data/conv4_weight.txt", "../data/conv4_bias.txt");
	CONV m_CONV5("m_CONV5", 256, 13, 13, 256, 13, 13, 1, 3, 1, "../data/conv5_weight.txt", "../data/conv5_bias.txt");
	MP m_MP5("m_MP5", 256, 13, 2, 256, 6);
	FC m_FC6("m_FC6", 9216, 4096, "../data/fc6_weight.txt", "../data/fc6_bias.txt", true);
	FC m_FC7("m_FC7", 4096, 4096, "../data/fc7_weight.txt", "../data/fc7_bias.txt", true);
	FC m_FC8("m_FC8", 4096, 1000, "../data/fc8_weight.txt", "../data/fc8_bias.txt", false);
	SOFTMAX m_SOFTMAX("m_SOFTMAX", 1000);
	
	m_Pattern.clock(clk);
	m_Pattern.out_valid(out_valid_softmax);
	m_Pattern.rst(reset);
	m_Pattern.in_valid(in_valid_pat);
	m_Pattern.img(img_out);
	m_Pattern.output_softmax(softmax_out);
	m_Pattern.output_linear(softmax_linear);

	m_CONV1.clock(clk);
	m_CONV1.rst(reset);
	m_CONV1.in_valid(in_valid_pat);
	m_CONV1.out_valid(out_valid_conv0);
	m_CONV1.in_feature(img_out);
	m_CONV1.out_feature(conv0_out);

	m_MP1.clock(clk);
	m_MP1.in_valid(out_valid_conv0);
	m_MP1.out_valid(out_valid_max0);
	m_MP1.in_feature(conv0_out);
	m_MP1.out_feature(max0_out);

	m_CONV2.clock(clk);
	m_CONV2.rst(reset);
	m_CONV2.in_valid(out_valid_max0);
	m_CONV2.out_valid(out_valid_conv1);
	m_CONV2.in_feature(max0_out);
	m_CONV2.out_feature(conv1_out);

	m_MP2.clock(clk);
	m_MP2.in_valid(out_valid_conv1);
	m_MP2.out_valid(out_valid_max1);
	m_MP2.in_feature(conv1_out);
	m_MP2.out_feature(max1_out);

	m_CONV3.clock(clk);
	m_CONV3.rst(reset);
	m_CONV3.in_valid(out_valid_max1);
	m_CONV3.out_valid(out_valid_conv2);
	m_CONV3.in_feature(max1_out);
	m_CONV3.out_feature(conv2_out);

	m_CONV4.clock(clk);
	m_CONV4.rst(reset);
	m_CONV4.in_valid(out_valid_conv2);
	m_CONV4.out_valid(out_valid_conv3);
	m_CONV4.in_feature(conv2_out);
	m_CONV4.out_feature(conv3_out);

	m_CONV5.clock(clk);
	m_CONV5.rst(reset);
	m_CONV5.in_valid(out_valid_conv3);
	m_CONV5.out_valid(out_valid_conv4);
	m_CONV5.in_feature(conv3_out);
	m_CONV5.out_feature(conv4_out);

	m_MP5.clock(clk);
	m_MP5.in_valid(out_valid_conv4);
	m_MP5.out_valid(out_valid_max2);
	m_MP5.in_feature(conv4_out);
	m_MP5.out_feature(max2_out);

	m_FC6.clock(clk);
	m_FC6.rst(reset);
	m_FC6.in_valid(out_valid_max2);
	m_FC6.out_valid(out_valid_linear0);
	m_FC6.in_feature(max2_out);
	m_FC6.out_feature(linear0_out);

	m_FC7.clock(clk);
	m_FC7.rst(reset);
	m_FC7.in_valid(out_valid_linear0);
	m_FC7.out_valid(out_valid_linear1);
	m_FC7.in_feature(linear0_out);
	m_FC7.out_feature(linear1_out);

	m_FC8.clock(clk);
	m_FC8.rst(reset);
	m_FC8.in_valid(out_valid_linear1);
	m_FC8.out_valid(out_valid_linear2);
	m_FC8.in_feature(linear1_out);
	m_FC8.out_feature(linear2_out);

	m_SOFTMAX.clock(clk);
	m_SOFTMAX.rst(reset);
	m_SOFTMAX.in_valid(out_valid_linear2);
	m_SOFTMAX.out_valid(out_valid_softmax);
	m_SOFTMAX.in_feature(linear2_out);
	m_SOFTMAX.output_softmax(softmax_out);
	m_SOFTMAX.output_linear(softmax_linear);

	// cout << "Simulation start" << endl;

	sc_start(180, SC_NS);

    return 0;
}