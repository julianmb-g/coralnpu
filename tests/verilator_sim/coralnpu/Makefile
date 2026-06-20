CXX = g++
CXXFLAGS = -std=c++20 -pthread -DCORALNPU_SIMD=128
INCLUDES = -I../../.. -I/usr/local/google/home/julianmb/.gemini/tmp/coralnpu-verilator-core

all: core_barebones_tb_test core_rvvi_tb_test core_rvvi_tb

core_barebones_tb_test: core_barebones_tb_test.o ../elf.o ../rvvi/test_main.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

core_rvvi_tb: core_rvvi_tb.o ../elf.o ../rvvi/trace_daemon.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

core_rvvi_tb_test: core_rvvi_tb_test.o ../rvvi/test_main.o ../rvvi/trace_daemon.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

../elf.o: ../elf.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

../rvvi/trace_daemon.o: ../rvvi/trace_daemon.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DVERILATOR_MODEL=VCoreVerification -c $< -o $@

run: all
	./core_barebones_tb_test

clean:
	rm -f *.o core_barebones_tb_test
