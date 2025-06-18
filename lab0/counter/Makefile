HOME = /RAID2/COURSE/2025_Spring/mlchip/mlchipTA01
LIB_DIR=-L $(HOME)/systemc-2.3.3/lib-linux64
INC_DIR=-I $(HOME)/systemc-2.3.3/include
LIB=-lsystemc -Wl,-rpath,$(HOME)/systemc-2.3.3/lib-linux64
APP=counter
TESTBENCH=counter_tb

counter:counter.cpp counter_tb.cpp
	g++ $(APP).cpp $(TESTBENCH).cpp -o $(APP) $(LIB_DIR) $(INC_DIR) $(LIB) && ./$(APP)

clean:
	rm -rf $(APP) *.vcd