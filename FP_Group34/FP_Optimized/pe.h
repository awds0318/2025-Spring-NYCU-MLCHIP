#ifndef PE_H
#define PE_H

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <queue>
#include <map>

#include <string>
#include <cmath>
#include <sstream>

#include "systemc.h"

using namespace std;

struct Packet
{
    int source_id;
    int dest_id;
    int op_id;
    bool is_parameter;
    bool is_bias;
    vector<float> datas;
};

struct LayerConfig
{
    int next_dst;
    int next_op;

    int in_ch, in_height, in_width;
    int out_ch, out_height, out_width;
    int stride, kernel_size, padding;

    int mp_out_ch, mp_out_size, mp_stride;

    string weight_path, bias_path;
};

class PE
{
public:
    PE() {};
    Packet *get_packet();
    void check_packet(Packet *p);
    void init(int pe_id);

private:
    int id;

    LayerConfig config;

    vector<float> weights_vec;
    vector<float> bias_vec;

    queue<Packet *> send_packets;
};

Packet *PE::get_packet()
{
    if (!send_packets.empty())
    {
        Packet *tmp_packet = send_packets.front();
        cout << "[INFO]PE_" << id << ": get_packet->datasize:" << tmp_packet->datas.size() << endl;
        send_packets.pop();
        return tmp_packet;
    }
    else
        return NULL;
}

LayerConfig make_config(
    int next_dst, int next_op,
    int in_ch, int in_height, int in_width,
    int out_ch, int out_height, int out_width,
    int stride, int kernel_size, int padding,
    int mp_out_ch, int mp_out_size, int mp_stride,
    string weight_path, string bias_path)
{
    LayerConfig cfg;
    cfg.next_dst = next_dst;
    cfg.next_op = next_op;
    cfg.in_ch = in_ch;
    cfg.in_height = in_height;
    cfg.in_width = in_width;
    cfg.out_ch = out_ch;
    cfg.out_height = out_height;
    cfg.out_width = out_width;
    cfg.stride = stride;
    cfg.kernel_size = kernel_size;
    cfg.padding = padding;
    cfg.mp_out_ch = mp_out_ch;
    cfg.mp_out_size = mp_out_size;
    cfg.mp_stride = mp_stride;
    cfg.weight_path = weight_path;
    cfg.bias_path = bias_path;
    return cfg;
}

map<int, LayerConfig> get_pe_configs()
{
    map<int, LayerConfig> m;
    m[0] = make_config(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", "");
    m[1] = make_config(2, 1, 3, 224, 224, 64, 55, 55, 4, 11, 2, 64, 27, 2, "data/conv1_weight.txt", "data/conv1_bias.txt");
    m[2] = make_config(3, 2, 64, 27, 27, 192, 27, 27, 1, 5, 2, 192, 13, 2, "data/conv2_weight.txt", "data/conv2_bias.txt");
    m[3] = make_config(7, 2, 192, 13, 13, 384, 13, 13, 1, 3, 1, 0, 0, 0, "data/conv3_weight.txt", "data/conv3_bias.txt");
    m[7] = make_config(6, 1, 384, 13, 13, 256, 13, 13, 1, 3, 1, 0, 0, 0, "data/conv4_weight.txt", "data/conv4_bias.txt");
    m[6] = make_config(5, 3, 256, 13, 13, 256, 13, 13, 1, 3, 1, 256, 6, 2, "data/conv5_weight.txt", "data/conv5_bias.txt");
    m[5] = make_config(4, 3, 9216, 0, 0, 4096, 0, 0, 0, 0, 0, 0, 0, 0, "data/fc6_weight.txt", "data/fc6_bias.txt");
    m[4] = make_config(12, 4, 4096, 0, 0, 4096, 0, 0, 0, 0, 0, 0, 0, 0, "data/fc7_weight.txt", "data/fc7_bias.txt");
    m[12] = make_config(0, 0, 4096, 0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0, "data/fc8_weight.txt", "data/fc8_bias.txt");
    return m;
}

void PE::init(int pe_id)
{
    id = pe_id;
    static const map<int, LayerConfig> PE_CONFIGS = get_pe_configs();
    if (PE_CONFIGS.find(pe_id) != PE_CONFIGS.end())
        config = PE_CONFIGS.at(pe_id);
    else
        config = PE_CONFIGS.at(0);
}

vector<float> conv(const Packet *p, const LayerConfig &cfg, const vector<float> &weights, const vector<float> &bias)
{
    vector<float> output(cfg.out_ch * cfg.out_height * cfg.out_width, 0.0f);

    for (int o_c = 0; o_c < cfg.out_ch; ++o_c)
    {
        for (int o_y = 0; o_y < cfg.out_height; ++o_y)
        {
            for (int o_x = 0; o_x < cfg.out_width; ++o_x)
            {
                float sum = 0.0f;
                for (int i_c = 0; i_c < cfg.in_ch; ++i_c)
                {
                    for (int k_y = -int(cfg.kernel_size / 2); k_y <= int(cfg.kernel_size / 2); ++k_y)
                    {
                        for (int k_x = -int(cfg.kernel_size / 2); k_x <= int(cfg.kernel_size / 2); ++k_x)
                        {
                            int i_y = cfg.stride * o_y + k_y + int(cfg.kernel_size / 2) - cfg.padding;
                            int i_x = cfg.stride * o_x + k_x + int(cfg.kernel_size / 2) - cfg.padding;

                            if (i_y >= 0 && i_y < cfg.in_height && i_x >= 0 && i_x < cfg.in_width)
                            {
                                int in_idx = i_c * cfg.in_height * cfg.in_width + i_y * cfg.in_width + i_x;
                                int w_idx = (o_c * cfg.in_ch * cfg.kernel_size * cfg.kernel_size) + (i_c * cfg.kernel_size * cfg.kernel_size) + ((k_y + int(cfg.kernel_size / 2)) * cfg.kernel_size) + (k_x + int(cfg.kernel_size / 2));
                                sum += p->datas[in_idx] * weights[w_idx];
                            }
                        }
                    }
                }
                int out_idx = (o_c * cfg.out_height * cfg.out_width) + (o_y * cfg.out_width) + o_x;
                output[out_idx] = max(0.0f, sum + bias[o_c]); // ReLU
            }
        }
    }
    return output;
}

void PE::check_packet(Packet *p)
{
    if (p->is_parameter)
    {
        if (p->is_bias)
        {
            vector<float> golden_weight;
            vector<float> golden_bias;

            if (id != 1 && id != 2 && id != 3 && id != 7 && id != 6 && id != 5 && id != 4 && id != 12)
                cerr << "[FATAL] Don't have this id!" << endl;


            float element;
            int cnt{0};

            ifstream weight_file(config.weight_path.c_str());
            while (weight_file >> element)
            {
                golden_weight.push_back(element);
                cnt++;
            }
            weight_file.close();

            ifstream bias_file(config.bias_path.c_str());
            while (bias_file >> element)
            {
                golden_bias.push_back(element);
                cnt++;
            }
            bias_file.close();

            cout << "[INFO] PE_" << id << " get bias" << endl;
            if (!bias_vec.empty())
                bias_vec.clear();

            for (int i = 0; i < (p->datas.size()); ++i)
                bias_vec.push_back(p->datas[i]);

            cout << "[INFO] Weight num: " << weights_vec.size() << endl;
            cout << "[INFO] Bias num: " << bias_vec.size() << endl;
            cout << "[INFO] Weight[0]: " << weights_vec[0] << endl;
            cout << "[INFO] Weight[1]: " << weights_vec[1] << endl;
            cout << "[INFO] Weight[2]: " << weights_vec[2] << endl;
            cout << "[INFO] Weight[3]: " << weights_vec[3] << endl;
            cout << "[INFO] Weight[last]: " << weights_vec[weights_vec.size() - 1] << endl;
            cout << "[INFO] Bias[0]: " << bias_vec[0] << endl;
            cout << "[INFO] Bias[last]: " << bias_vec[bias_vec.size() - 1] << endl;
        }
        else
        {
            cout << "[INFO] PE_" << id << " get weight" << endl;

            if (!weights_vec.empty())
                weights_vec.clear();

            for (int i = 0; i < (p->datas.size()); ++i)
                weights_vec.push_back(p->datas[i]);

            cout << "[INFO] Weight num: " << weights_vec.size() << endl;
            cout << "[INFO] Bias num: " << bias_vec.size() << endl;
        }
    }
    else
    {
        Packet *tmp_packet = new Packet();
        LayerConfig &cfg = config;
        tmp_packet->source_id = id;
        tmp_packet->dest_id = cfg.next_dst;
        tmp_packet->op_id = cfg.next_op;
        tmp_packet->is_parameter = 0;
        tmp_packet->is_bias = 0;
        tmp_packet->datas.clear();

        cout << "[INFO] PE_" << id << " recieve op" << endl;
        cout << "[INFO] " << "Src: " << p->source_id << endl;
        cout << "[INFO] " << "Dst: " << p->dest_id << endl;
        cout << "[INFO] " << "Op_id: " << p->op_id << endl;
        cout << "[INFO] Weight num: " << weights_vec.size() << endl;
        cout << "[INFO] Bias num: " << bias_vec.size() << endl;
        cout << "[INFO] Input Feature num: " << p->datas.size() << endl;

        if (p->op_id == 1) // Conv + MaxPooling
        {
            for (int i = 0; i < cfg.mp_out_ch * cfg.mp_out_size * cfg.mp_out_size; ++i)
                tmp_packet->datas.push_back(0);

            vector<float> conv_result = conv(p, cfg, weights_vec, bias_vec);

            // MaxPooling
            for (int o_c = 0; o_c < cfg.mp_out_ch; ++o_c)
            {
                for (int o_y = 0; o_y < cfg.mp_out_size; ++o_y)
                {
                    for (int o_x = 0; o_x < cfg.mp_out_size; ++o_x)
                    {
                        float tmp = 0;
                        for (int k_y = -1; k_y <= 1; ++k_y)
                        {
                            for (int k_x = -1; k_x <= 1; ++k_x)
                            {
                                int i_y = o_y * cfg.mp_stride + 1 + k_y;
                                int i_x = o_x * cfg.mp_stride + 1 + k_x;
                                int tmp_idx = (o_c * cfg.out_height * cfg.out_width) + (i_y * cfg.out_width) + i_x;

                                if (conv_result[tmp_idx] > tmp)
                                    tmp = conv_result[tmp_idx];
                            }
                        }
                        int out_idx = (o_c * cfg.mp_out_size * cfg.mp_out_size) + (o_y * cfg.mp_out_size) + o_x;
                        tmp_packet->datas[out_idx] = tmp;
                    }
                }
            }
            send_packets.push(tmp_packet);
        }
        else if (p->op_id == 2) // Conv
        {
            for (int i = 0; i < cfg.out_ch * cfg.out_height * cfg.out_width; ++i)
                tmp_packet->datas.push_back(0);

            tmp_packet->datas = conv(p, cfg, weights_vec, bias_vec);

            send_packets.push(tmp_packet);
        }
        else if (p->op_id == 3 || p->op_id == 4) // Linear + ReLU
        {
            // Linear
            for (int i = 0; i < cfg.out_ch; ++i)
                tmp_packet->datas.push_back(0);


            for (int o_c = 0; o_c < cfg.out_ch; ++o_c)
            {
                float sum = 0.0f;

                for (int i_c = 0; i_c < cfg.in_ch; ++i_c)
                    sum += p->datas[i_c] * weights_vec[(o_c * cfg.in_ch) + i_c];

                sum += bias_vec[o_c];
                tmp_packet->datas[o_c] = max(0.0f, sum);  // ReLU

                send_packets.push(tmp_packet);
            }
        }
    }
}

#endif