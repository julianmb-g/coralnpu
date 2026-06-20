CXX = g++
CXXFLAGS = -std=c++20 -pthread
INCLUDES = -I../../.. -I/usr/local/google/home/julianmb/.gemini/tmp/coralnpu-verilator-core

all: spsc_test trace_test formatter_test

spsc_test: spsc_ring_buffer_test.o test_main.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

trace_test: trace_daemon_test.o trace_daemon.o test_main.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

formatter_test: mpact_trace_formatter_test.o test_main.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

run: all
	./spsc_test
	./trace_test

clean:
	rm -f *.o spsc_test trace_test
