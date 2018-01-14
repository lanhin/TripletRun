// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "device.h"
#include "constants.h"
#include <cassert>
#include <iostream>

namespace triplet{
  //The Device class
  Device::Device()
    : id_(-1),
      status(UNAVAILABLE),
      location(-1),
      execution_time(0.0),
      data_trans_time(0.0),
      Allocated_RAM(0),
      available_time(0.0) {}

  Device::Device(int id, float compute, int RAM, float bw, int loc)
    : id_(id),computing_power(compute),RAM_size(RAM),bandwidth(bw),location(loc),execution_time(0.0),data_trans_time(0.0),Allocated_RAM(0), available_time(0.0) {
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
    assert(compute >= ZERO_NEGATIVE);
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

  void Device::IncreaseTransTime(float TransTime){
    assert(data_trans_time >= ZERO_NEGATIVE);
    assert(TransTime >= ZERO_NEGATIVE);
    data_trans_time += TransTime;
  }
  
  void Device::IncreaseRunTime(float ExeTime){
    assert(execution_time >= ZERO_NEGATIVE);
    assert(ExeTime >= ZERO_NEGATIVE);
    execution_time += ExeTime;
  }

  void Device::SetAvaTime(float time){
    assert(time >= ZERO_NEGATIVE);

    available_time = time;
  }

  void Device::MemAlloc(int size){
    assert(Allocated_RAM + size <= RAM_size + ZERO_POSITIVE);
    Allocated_RAM += size;

#ifdef DEBUG
    std::cout<<"----MemAlloc on device "<<id_<<", allocated RAM: "<<Allocated_RAM<<" MB, total RAM: "<<RAM_size<<" MB."<<std::endl;
#endif
  }

  void Device::MemFree(int size){
    assert(size <= Allocated_RAM + ZERO_POSITIVE);
    Allocated_RAM = std::max(0, Allocated_RAM - size);

#ifdef DEBUG
    std::cout<<"----MemFree on device "<<id_<<", allocated RAM: "<<Allocated_RAM<<" MB, total RAM: "<<RAM_size<<" MB."<<std::endl;
#endif
  }

  /** Add a new slot into ITS.
   */
  void Device::NewSlot(float startTime, float endTime){
    assert(startTime >= ZERO_NEGATIVE);
    assert(endTime + ZERO_POSITIVE >= startTime);

    this->ITS.insert(std::make_pair(startTime, endTime));
  }

  /** Find a slot from ITS,
      which start no earlier than ESTpred and last at least W_ij.
      If found, return the start time of the slot;
      if not, return -1.
  */
  float Device::FindSlot(float ESTpred, float W_ij){
    assert(ESTpred >= ZERO_NEGATIVE);
    assert(W_ij >= ZERO_NEGATIVE);

    float min_Ts = -1;
    // If found multiple, return the earliest one.
    for (auto& it : this->ITS) {
      if ( (it.second - std::max(ESTpred, it.first)) >= W_ij ){
	// a good slot is found
	if (min_Ts < 0 || min_Ts > it.first){
	  min_Ts = it.first;
	}
      }
    }
    return std::max(min_Ts, ESTpred);
  }

  /** Update ITS,
      the parameters Exe_Ts and W_ij define an execution period.
      At first, remove slots earlier than current_time.
  */
  void Device::UpdateSlot(float Exe_Ts, float W_ij, float current_time){

    //1. Remove slots that earlier than current_time.
    for (auto it = this->ITS.begin(); it != this->ITS.end();) {
      if ( (*it).second < current_time){
	this->ITS.erase(it++);
      }else{
	it++;
      }
    }

    //2. Now update ITS according to Exe_Ts and W_ij.
    float fir, sec;
    for (auto it = this->ITS.begin(); it != this->ITS.end();) {
      fir = (*it).first;
      sec = (*it).second;
      if (Exe_Ts <= fir && Exe_Ts + W_ij >= fir){
	// Just delete it from ITS
	this->ITS.erase(it++);
	if (Exe_Ts + W_ij <= sec){
	  this->ITS.insert(std::make_pair(Exe_Ts + W_ij, sec));
	}
      }else if(Exe_Ts > fir && Exe_Ts <= sec){
	if (Exe_Ts + W_ij < sec){
	  // Delete it and add <it.first, Exe_Ts>, <Exe_Ts+W_ij, it.second>
	  // This should be the most common case.
	  this->ITS[fir] = Exe_Ts;
	  //this->ITS.insert(std::make_pair((*it).first, Exe_Ts));
	  this->ITS.insert(std::make_pair(Exe_Ts + W_ij, sec));
	  it++;
	}else{
	  // Delete it and add <it.first, Exe_Ts>
	  this->ITS[fir] = Exe_Ts;
	  //this->ITS.insert(std::make_pair((*it).first, Exe_Ts));
	  it++;
	}
      }else{
	it++;
      }
    }
  }

  /** Show the ITS. Output all the slots one by one,
      mainly for debugging.
   */
  void Device::ShowSlot(){
    std::cout<<"======ITS for Dev"<<GetId()<<"======"<< std::endl;
    for (auto& it : ITS) {
      std::cout << it.first << " to " << it.second << std::endl;
    }
    std::cout << "==================" << std::endl;
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

  int Device::GetFreeRAM(){
    assert(RAM_size >= Allocated_RAM + ZERO_NEGATIVE);
    return (RAM_size - Allocated_RAM);
  }

  float Device::GetBw(){
    return bandwidth;
  }

  int Device::GetLocation(){
    return location;
  }

  float Device::GetTransTime(){
    assert(data_trans_time >= ZERO_NEGATIVE);
    return data_trans_time;
  }

  float Device::GetRunTime(){
    assert(execution_time >= ZERO_NEGATIVE);
    return execution_time;
  }

  float Device::GetAvaTime(){
    assert(available_time >= ZERO_NEGATIVE);

    return available_time;
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
