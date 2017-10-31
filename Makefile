# =======================
# @2017-09 by lanhin
# ACSA, USTC
# lanhin1@gmail.com
# =======================

CXX = g++
CXXFLAGS = -O3 -std=c++11

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

.PHONY: all
all: triplet

.PHONY: clean
clean:
	rm -f triplet *.o
