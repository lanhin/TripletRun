// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "device.h"
#include <iostream>

const char* StatusArray[] = {
  "FREE",
  "BUSY",
  "UNAVAILABLE"
};

void ShowDeviceInfo(triplet::Device dev){
  std::cout<<"========Device info========"<<std::endl;
  std::cout<<" ID: "<<dev.GetId()<<std::endl;
  std::cout<<" Status: "<<StatusArray[dev.GetStatus()]<<std::endl;
  std::cout<<" Computing Power: "<<dev.GetCompPower()<<std::endl;
  std::cout<<" RAM(MB): "<<dev.GetRAM()<<std::endl;
  std::cout<<" Bandwidth(GB/s): "<<dev.GetBw()<<std::endl;
  std::cout<<" Location: "<<dev.GetLocation()<<std::endl;
  std::cout<<"==========================="<<std::endl;
}

int main(int argc, char *argv[])
{
  // Create and init devices
  triplet::Device deva(0, 1.0, 2048, 96.0, 0);

  ShowDeviceInfo(deva);

  // Read and init graph

  // Process the graph
  
  return 0;
}
