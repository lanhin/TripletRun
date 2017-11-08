// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "device.h"
#include <cassert>
#include <iostream>

namespace triplet{
  //The Device class
  Device::Device()
    : id_(-1),
      status(UNAVAILABLE),
      location(-1) {}

  Device::Device(int id, float compute, int RAM, float bw, int loc)
    : id_(id),computing_power(compute),RAM_size(RAM),bandwidth(bw),location(loc){
    status = FREE;}

  Device::~Device(){}

  void Device::SetBusy(){
    assert(status != BUSY);
    status = BUSY;
  }

  void Device::SetFree(){
    assert(status != FREE);
    status = FREE;
  }

  void Device::SetId(int id){
    assert(id >= 0);
    id_ = id;
  }

  void Device::SetCompPower(float compute){
    assert(compute >= 0.0);
    computing_power = compute;
  }

  void Device::SetRAM(int RAM){
    assert(RAM >= 0);
    RAM_size = RAM;
  }

  void Device::SetBw(float bw){
    assert(bw >= 0.0);
    bandwidth = bw;
  }

  void Device::SetLocation(int loc){
    assert(loc >= 0);
    location = loc;
  }

  int Device::GetId(){
    return id_;
  }

  bool Device::IsFree(){
    if (status == FREE){
      return true;
    }
    return false;
  }

  bool Device::IsBusy(){
    if (status == BUSY){
      return true;
    }
    return false;
  }

  Device::DeviceStatus Device::GetStatus(){
    return status;
  }

  float Device::GetCompPower(){
    return computing_power;
  }

  int Device::GetRAM(){
    return RAM_size;
  }

  float Device::GetBw(){
    return bandwidth;
  }

  int Device::GetLocation(){
    return location;
  }

  // The Connections class
  Connections::Connections(){
  }

  Connections::~Connections(){
    NodeConnection.clear();
    DeviceConnection.clear();

    assert(NodeConnection.empty());
    assert(DeviceConnection.empty());
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
      NodeConnection[std::pair<int, int>(src, dst)] = bw;
    }else{
      DeviceConnection[std::pair<int, int>(src, dst)] = bw;
    }
    
  }

  /** Note: if two devices' bandwidth is less than a threshold(like 0.01),
      the caller should re-call this function to get the nodes' bandwith.
   */
  float Connections::GetBw(int src, int dst, bool BetweenNode){ //Get bandwidth
    assert(src >= 0);
    assert(dst >= 0);
    assert(src != dst);

    if (src > dst){ //make sure that src < dst
      std::swap(src, dst);
    }

    connection::iterator it;
    if (BetweenNode){
      if ((it = NodeConnection.find(std::pair<int, int>(src,dst))) != NodeConnection.end()){//find something
	return it->second;
      }else{
	return 0.0;
      }
    }else{
      if ((it = DeviceConnection.find(std::pair<int, int>(src,dst))) != DeviceConnection.end()){//find something
	return it->second;
      }else{
	return 0.0;
      }

    }
  }

  void Connections::Clear(){
    NodeConnection.clear();
    DeviceConnection.clear();
  }

} // namespace triplet
