#include "layer.h"

void CONV::run()
{
    if (rst.read() == 1)
    {
        ifstream weight_file(weight_path.c_str());
        ifstream bias_file(bias_path.c_str());

        double element;
        while (weight_file >> element)
        {
            weight.push_back(element);
        }

        while (bias_file >> element)
        {
            bias.push_back(element);
        }

        weight_file.close();
        bias_file.close();

        // for (int i = 0; i < OUT_CH * OUT_HEIGHT * OUT_WIDTH; ++i)
        // {
        //     out_feature[i].write(0);
        // }

        out_valid.write(0);
        return;
    }

    if (in_valid.read())
    {
        // cout << name() << " CONV is running" << endl;
        int in_feature_x{0}, in_feature_y{0};
        int in_feature_idx{0}, out_feature_idx{0}, weight_idx{0};
        double sum, result;

		vector<double> in_feature_tmp(IN_CH * IN_HEIGHT * IN_WIDTH);
	
		for(int i=0; i< IN_CH * IN_HEIGHT * IN_WIDTH; ++i)
        {
			in_feature[i].read(in_feature_tmp[i]);
		}

        for (int o_c = 0; o_c < OUT_CH; ++o_c)
        {
            for (int o_y = 0; o_y < OUT_HEIGHT; ++o_y)
            {
                for (int o_x = 0; o_x < OUT_WIDTH; ++o_x)
                {
                    sum = 0;
                    for (int i_c = 0; i_c < IN_CH; ++i_c)
                    {
                        for (int k_y = int(KERNEL_SIZE / 2) * (-1); k_y < int(KERNEL_SIZE / 2) + 1; ++k_y)
                        {
                            for (int k_x = int(KERNEL_SIZE / 2) * (-1); k_x < int(KERNEL_SIZE / 2) + 1; ++k_x)
                            {

                                in_feature_y = STRIDE * o_y + k_y + int(KERNEL_SIZE / 2) - PADDING;
                                in_feature_x = STRIDE * o_x + k_x + int(KERNEL_SIZE / 2) - PADDING;

                                if (in_feature_y >= 0 && in_feature_y < IN_HEIGHT && in_feature_x >= 0 && in_feature_x < IN_WIDTH)
                                {
                                    in_feature_idx = (i_c * IN_HEIGHT * IN_WIDTH) + (in_feature_y * IN_WIDTH) + (in_feature_x);
                                    weight_idx = (o_c * IN_CH * KERNEL_SIZE * KERNEL_SIZE) + (i_c * KERNEL_SIZE * KERNEL_SIZE) + ((k_y + int(KERNEL_SIZE / 2)) * (KERNEL_SIZE)) + (k_x + int(KERNEL_SIZE / 2));
                                    sum += (in_feature_tmp[in_feature_idx] * weight[weight_idx]);
                                }
                            }
                        }
                    }

                    out_feature_idx = (o_c * OUT_HEIGHT * OUT_WIDTH) + (o_y * OUT_WIDTH) + o_x;

                    result = sum + bias[o_c];
                    // ReLU activation
                    if (result < 0)
                        out_feature[out_feature_idx].write(0);
                    else
                        out_feature[out_feature_idx].write(result);
                }
            }
        }
        out_valid.write(1);
    }
    else
        out_valid.write(0);
};


void MP::run()
{
    if (in_valid.read() == 1)
    {
        // cout << name() << " MP is running" << endl;

		vector<double> in_feature_tmp(IN_CHANNEL * IN_SIZE * IN_SIZE);
	
		for(int i=0; i< IN_CHANNEL * IN_SIZE * IN_SIZE; ++i)
        {
			in_feature[i].read(in_feature_tmp[i]);
		}

        // for(int i = 0; i < 10; ++i)
        // {
        //     cout << in_feature_tmp[i] << endl;
        // }
        // exit(0);

        for (int o_c = 0; o_c < OUT_CHANNEL; ++o_c)
        {
            for (int o_y = 0; o_y < OUT_SIZE; ++o_y)
            {
                for (int o_x = 0; o_x < OUT_SIZE; ++o_x)
                {
                    tmp = 0;
                    for (int k_y = -1; k_y <= 1; ++k_y)
                    {
                        for (int k_x = -1; k_x <= 1; ++k_x)
                        {
                            in_feature_y = o_y * STRIDE + 1 + k_y;
                            in_feature_x = o_x * STRIDE + 1 + k_x;

                            tmp_idx = (o_c * IN_SIZE * IN_SIZE) + (in_feature_y * IN_SIZE) + in_feature_x;

                            if (in_feature_tmp[tmp_idx] > tmp)
                                tmp = in_feature_tmp[tmp_idx];
                        }
                    }

                    out_feature[(o_c * OUT_SIZE * OUT_SIZE) + (o_y * OUT_SIZE) + o_x].write(tmp);
                }
            }
        }
        out_valid.write(1);
    }
    else
        out_valid.write(0);
};



void FC::run()
{
    if (rst.read() == 1)
    {
        ifstream weight_file(weight_path.c_str());
        ifstream bias_file(bias_path.c_str());

        double element;
        while (weight_file >> element)
        {
            weight.push_back(element);
        }

        while (bias_file >> element)
        {
            bias.push_back(element);
        }

        weight_file.close();
        bias_file.close();

        // for (int i = 0; i < OUT_CH; ++i)
        // {
        //     out_feature[i].write(0);
        // }

        out_valid.write(0);

        return;
    }

    if (in_valid.read() == 1)
    {
        // cout << name() << " FC is running" << endl;
        int in_feature_x{0}, in_feature_y{0};
        int in_feature_idx{0}, out_feature_idx{0}, weight_idx{0};
        double sum, result;

		vector<double> in_feature_tmp(IN_CH);
	
		for(int i=0; i< IN_CH; ++i)
        {
			in_feature[i].read(in_feature_tmp[i]);
		}


        for (int o_c = 0; o_c < OUT_CH; ++o_c)
        {
            sum = 0;
            for (int i_c = 0; i_c < IN_CH; ++i_c)
            {
                sum += (in_feature_tmp[i_c] * weight[(o_c * IN_CH) + i_c]);
            }

            result = sum + bias[o_c];

            if (with_relu == true)
            {
                if (result < 0)
                    out_feature[o_c].write(0);
                else
                    out_feature[o_c].write(result);
            }
            else
                out_feature[o_c].write(result);

        }

        out_valid.write(1);
    }
    else
        out_valid.write(0);
};

void SOFTMAX::run()
{
    if (rst.read() == 1)
    {
        // for(int i = 0; i < SIZE; ++i)
        // {
        //     output_linear[i].write(0);
        //     output_softmax[i].write(0);
        // }

        out_valid.write(0);

        return;
    }

    if(in_valid.read() == 1)
    {
        // cout << name() << " SOFTMAX is running" << endl;
        double exp_sum{0}, a{0}, tmp{0};

        vector<double> in_feature_tmp(SIZE);

        for(int i=0; i< SIZE; ++i)
        {
            in_feature[i].read(in_feature_tmp[i]);
        }

        for(int i = 0; i < SIZE; ++i)
            exp_sum += (double) exp(in_feature_tmp[i]);

        for(int i = 0; i < SIZE; ++i)
        {
            tmp = in_feature_tmp[i];
            a = (double) exp(tmp);

            output_linear[i].write(tmp);
            output_softmax[i].write(a / exp_sum);
        }

        out_valid.write(1);
    }
    else 
        out_valid.write(0);
};