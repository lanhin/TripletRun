// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "device.h"
#include <cassert>

namespace triplet{
  Device::Device()
    : id_(-1),
      status(UNAVAILABLE),
      location(-1) {}

  Device::Device(int id, float compute, int RAM, float bw, int location)
    : id_(id),computing_power(compute),RAM_size(RAM),bandwidth(bw){
    status = FREE;}

  Device::~Device(){}

  void Device::SetBusy(){
    status = BUSY;
  }

  void Device::SetFree(){
    status = FREE;
  }

  void Connections::NewLink(int src, int dst, float bw, bool BetweenNode){ //Add a new link to the connection
    assert(src >= 0);
    assert(dst >= 0);
    assert(src != dst);
    assert(bw >= 0.0);

    if (src > dst){ //make sure that src < dst
      std::swap(src, dst);
    }
    
    if (BetweenNode){
      NodesConnection[std::pair<int, int>(src, dst)] = bw;
    }else{
      DeviceConnection[std::pair<int, int>(src, dst)] = bw;
    }
    
  }

  float Connections::GetBW(int src, int dst, bool BetweenNode){ //Get bandwidth
    assert(src >= 0);
    assert(dst >= 0);
    assert(src != dst);

    if (src > dst){ //make sure that src < dst
      std::swap(src, dst);
    }

    connection::iterator it;
    if (BetweenNode){
      if (NodesConnection.find(std::pair<int, int>(src,dst)) != NodesConnection.end()){//find something
	return it->second;
      }else{
	return 0.0;
      }
    }else{
      if (DeviceConnection.find(std::pair<int, int>(src,dst)) != DeviceConnection.end()){//find something
	return it->second;
      }else{
	return 0.0;
      }

    }
  }

} // namespace triplet
