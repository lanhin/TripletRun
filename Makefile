# =======================
# @2017-09 by lanhin
# ACSA, USTC
# lanhin1@gmail.com
# =======================

CXX = g++
CXXFLAGS = -O3

triplet: triplet.o device.o
	 $(CXX) $(CXXFLAGS) -o $@ $^

triplet.o: src/triplet.cc
	   $(CXX) $(CXXFLAGS) -c -o $@ $^

device.o: src/device.cc
	  $(CXX) $(CXXFLAGS) -c -o $@ $^

.PHONY: all
all: triplet

.PHONY: clean
clean:
	rm -f triplet *.o
