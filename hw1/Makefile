HOME = /RAID2/COURSE/2025_Spring/mlchip/mlchipTA01
LIB_DIR=$(HOME)/systemc-2.3.3/lib-linux64
INC_DIR=$(HOME)/systemc-2.3.3/include
LIB=-lsystemc-2.3.3
RPATH=-Wl,-rpath,$(HOME)/systemc-2.3.3/lib-linux64

O = run
C = *.cpp

# Default target
all: $(O)

# Compile and link
$(O): $(C)
	g++ -I . -I  $(INC_DIR) -L . -L $(LIB_DIR) -o $(O) $(C) $(LIB) $(RPATH) -g -w -O3
# Run with cat.txt
cat: $(O)
	./run cat.txt

# Run with dog.txt
dog: $(O)
	./run dog.txt

# Clean up compiled files
clean:
	rm -rf $(O)