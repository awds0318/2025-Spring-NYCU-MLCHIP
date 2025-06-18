#ifndef LAYER_H
#define LAYER_H

#include <systemc.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <string>
#include <sstream>
#include <cmath>

#include <string>
#include <algorithm>

using namespace std;


SC_MODULE(CONV)
{
	sc_in_clk clock;

	sc_in<bool> rst, in_valid;
	sc_out<bool> out_valid;

	sc_vector<sc_fifo_in<double> > in_feature;
	sc_vector<sc_fifo_out<double> > out_feature;

	int IN_CH, IN_HEIGHT, IN_WIDTH;
	int OUT_CH, OUT_HEIGHT, OUT_WIDTH;
	int STRIDE, KERNEL_SIZE, PADDING;

	vector<double> weight, bias;
	string weight_path, bias_path;

	void run();
	
	SC_HAS_PROCESS(CONV);
	
	CONV(
		sc_module_name name, 
		const int IN_CH, const int IN_HEIGHT, const int IN_WIDTH,
		const int OUT_CH, const int OUT_HEIGHT, const int OUT_WIDTH,
		const int STRIDE, const int KERNEL_SIZE, const int PADDING,
		string weight_path, string bias_path
	) : 
		sc_module(name),
		IN_CH(IN_CH), IN_HEIGHT(IN_HEIGHT), IN_WIDTH(IN_WIDTH),
		OUT_CH(OUT_CH), OUT_HEIGHT(OUT_HEIGHT), OUT_WIDTH(OUT_WIDTH),
		STRIDE(STRIDE), KERNEL_SIZE(KERNEL_SIZE), PADDING(PADDING),
		weight_path(weight_path), bias_path(bias_path),
		in_feature{"in_feature", IN_CH * IN_HEIGHT * IN_WIDTH}, out_feature{"out_feature", OUT_CH * OUT_HEIGHT * OUT_WIDTH}
	{
		SC_METHOD(run);
		sensitive << clock.pos();
	}
	
};


SC_MODULE(MP)
{
	sc_in_clk clock;
	sc_in<bool> in_valid;
	sc_out<bool> out_valid;

	sc_vector<sc_fifo_in<double> > in_feature;
	sc_vector<sc_fifo_out<double> > out_feature;

	int IN_CHANNEL, IN_SIZE, STRIDE;
	int OUT_CHANNEL, OUT_SIZE;
	int in_feature_x{0}, in_feature_y{0};
	int tmp_idx;

	double tmp;

	void run();

	SC_HAS_PROCESS(MP);
	
	MP(
		sc_module_name name, 
		const int IN_CHANNEL, const int IN_SIZE, const int STRIDE,
		const int OUT_CHANNEL, const int OUT_SIZE
	) : 
		sc_module(name),
		IN_CHANNEL(IN_CHANNEL), IN_SIZE(IN_SIZE), STRIDE(STRIDE),
		OUT_CHANNEL(OUT_CHANNEL), OUT_SIZE(OUT_SIZE),
		in_feature{"in_feature", IN_CHANNEL * IN_SIZE * IN_SIZE}, out_feature{"out_feature", OUT_CHANNEL * OUT_SIZE * OUT_SIZE}
	{
		SC_METHOD(run);
		sensitive << clock.pos();
	}

};


SC_MODULE(FC)
{
	sc_in_clk clock;
	sc_in<bool> rst, in_valid;
	sc_out<bool> out_valid;

	sc_vector<sc_fifo_in<double> > in_feature;
	sc_vector<sc_fifo_out<double> > out_feature;

	int IN_CH, OUT_CH;

	vector<double> weight, bias;
	string weight_path, bias_path;

	bool with_relu;

	void run();

	SC_HAS_PROCESS(FC);

	FC(
		sc_module_name name, 
		const int IN_CH, const int OUT_CH,
		string weight_path, string bias_path,
		bool with_relu
	) : 
		sc_module(name),
		IN_CH(IN_CH), OUT_CH(OUT_CH),
		weight_path(weight_path), bias_path(bias_path),
		with_relu(with_relu),
		in_feature{"in_feature", IN_CH}, out_feature{"out_feature", OUT_CH}
	{
		SC_METHOD(run);
		sensitive << clock.pos();
	}

};


SC_MODULE(SOFTMAX)
{
	sc_in_clk clock;
	sc_in<bool> rst, in_valid;
	sc_out<bool> out_valid;

	int SIZE;
	sc_vector<sc_fifo_in<double> > in_feature;
	sc_vector<sc_fifo_out<double> > output_softmax;
	sc_vector<sc_fifo_out<double> > output_linear;

	void run();

	SC_HAS_PROCESS(SOFTMAX);

	SOFTMAX(
		sc_module_name name, 
		const int SIZE
	) : 
		sc_module(name),
		SIZE(SIZE),
		in_feature{"in_feature", SIZE}, output_softmax{"output_softmax", SIZE}, output_linear{"output_linear", SIZE}
	{
		SC_METHOD(run);
		sensitive << clock.pos();
	}


};
#endif