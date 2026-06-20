CXX = g++
CXXFLAGS = -std=c++20 -pthread
INCLUDES = -I../../.. -I../../..

all: core_barebones_tb_test

core_barebones_tb_test: core_barebones_tb_test.o ../elf.o ../rvvi/test_main.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

../elf.o: ../elf.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

run: all
	./core_barebones_tb_test

clean:
	rm -f *.o core_barebones_tb_test
