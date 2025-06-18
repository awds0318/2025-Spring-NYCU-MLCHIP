#include "ROM.h"
#include <sstream>

void ROM::run()
{
    int id;
    bool type;
    string filename;
    ifstream file;
    float value;
    bool is_reading_data = false;
    float buffer[FLIT_GROUP];

    while(rst.read())
        wait();
    while(true)
    {
        if(!is_reading_data)
        {
            if(ARVALID.read())
            {
                ARREADY.write(1);
                // Read signals
                id = ARADDR.read() >> 1;
                type = ARADDR.read() & 0x1;
                if (id == 0)
                {
                    filename = string(DATA_PATH) + IMAGE_FILE_NAME;
                }
                else if (id <= 5)
                {
                    std::stringstream ss;
                    ss << id;
                    if (type == 0)
                        filename = string(DATA_PATH) + "conv" + ss.str() + "_weight.txt";
                    else
                        filename = string(DATA_PATH) + "conv" + ss.str() + "_bias.txt";
                }
                else if (id <= 8)
                {
                    std::stringstream ss;
                    ss << id;
                    if (type == 0)
                        filename = string(DATA_PATH) + "fc" + ss.str() + "_weight.txt";
                    else
                        filename = string(DATA_PATH) + "fc" + ss.str() + "_bias.txt";
                }
                else
                {
                    cout << "Error: Invalid layer id " << id << "." << endl;
                    sc_stop();
                }
                file.open(filename.c_str());
                // if (!file.is_open()) {
                //     std::cerr << "Error: Failed to open file: " << filename << std::endl;
                // } else {
                //     std::cout << "Successfully opened file: " << filename << std::endl;
                // }
                is_reading_data = true;
            }
        }
        else 
        {
            ARREADY.write(0);

            if(ARVALID.read())
            {
                cout << "Error: ARVALID should be low when reading data." << endl;
                sc_stop();
            }

            if(RREADY.read())
            {   
                RVALID.write(true);
                for (int i = 0; i < FLIT_GROUP; ++i)
                    buffer[i] = 0; // Clear previous data
                
                for (int i = 0; i < FLIT_GROUP; ++i)
                {
                    file >> value;
                    if(file.eof())
                    {
                        //cout << "End of file reached for layer " << id << endl;
                        for (int j = i; j < FLIT_GROUP; ++j)
                            buffer[j] = 0; // Fill remaining slots with 0
                        
                        file.close();
                        is_reading_data = false;
                        RVALID.write(false);
                        // RDATA.write(0);
                        break;
                    }
                    buffer[i] = value;
                }

                for (int i = 0; i < FLIT_GROUP; ++i)
                    RDATA[i].write(buffer[i]);
                // file >> value;
                // if(file.eof())
                // {
                //     //cout << "End of file reached for layer " << layer_id.read() << endl;
                //     file.close();
                //     RVALID.write(false);
                //     RDATA.write(0);
                //     is_reading_data = false;
                //     continue; 
                // }
                // RDATA.write(value);
                //cout << "Layer " << layer_id.read() << " data: " << value << endl;
            }
        }
        wait();
    }
}