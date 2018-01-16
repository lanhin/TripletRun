# =======================
# @2017-09 by lanhin
# ACSA, USTC
# lanhin1@gmail.com
# =======================

CXX = g++
CXXFLAGS = -g -std=c++11
TESTFLAGS = $(CXXFLAGS) -I$(CURDIR) -lpthread

triplet: triplet.o device.o graph.o runtime.o jsoncpp.o
	$(CXX) $(CXXFLAGS) -o $@ $^

triplet.o: src/triplet.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^

device.o: src/device.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^

graph.o: src/graph.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^

runtime.o: src/runtime.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^

jsoncpp.o: src/jsoncpp.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

test: gtest_main.o gtest-all.o device_test.o device.o graph_test.o graph.o runtime_test.o runtime.o jsoncpp.o
	$(CXX) $(TESTFLAGS) -o $@ $^

gtest_main.o: gtest/gtest_main.cc
	$(CXX) $(TESTFLAGS) -c -o $@ $^

gtest-all.o: gtest/gtest-all.cc
	$(CXX) $(TESTFLAGS) -c -o $@ $^

device_test.o: tests/device_test.cc
	$(CXX) $(TESTFLAGS) -c -o $@ $^

graph_test.o: tests/graph_test.cc
	$(CXX) $(TESTFLAGS) -c -o $@ $^

runtime_test.o: tests/runtime_test.cc
	$(CXX) $(TESTFLAGS) -c -o $@ $^

.PHONY: all
all: triplet test

.PHONY: clean
clean:
	rm -f triplet test *.o
