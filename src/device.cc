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
      finished_tasks(0),
      execution_time(0.0),
      data_trans_time(0.0),
      Allocated_RAM(0),
      available_time(0.0) {}

  Device::Device(int id, float compute, float RAM, float bw, int loc)
    : id_(id),computing_power(compute),RAM_size(RAM),bandwidth(bw),location(loc),finished_tasks(0),execution_time(0.0),data_trans_time(0.0),Allocated_RAM(0), available_time(0.0) {
    status = FREE;}

  Device::~Device(){}

  /** Set the device status to busy.
      If the previous status is not busy, return true,
      otherwise, return false.
   */
  bool Device::SetBusy(){
    bool previous;
    if (status != BUSY){
      previous = true;
    }else{
      previous = false;
    }

    status = BUSY;
    return previous;
  }

  /** Set the device status to free.
      If it's already free, import an error.
  */
  void Device::SetFree(){
    assert(status != FREE);
    status = FREE;
  }

  /** Set the id of the device.
   */
  void Device::SetId(int id){
    assert(id >= 0);
    id_ = id;
  }

  /** Set (float) computation power of the device.
   */
  void Device::SetCompPower(float compute){
    assert(compute >= ZERO_NEGATIVE);
    computing_power = compute;
  }

  /** Set RAM size of the device.
   */
  void Device::SetRAM(float RAM){
    assert(RAM > ZERO_NEGATIVE);
    RAM_size = RAM;
  }

  /** Set memory bandwidth of the device.
   */
  void Device::SetBw(float bw){
    assert(bw >= 0.0);
    bandwidth = bw;
  }

  /** Set location (computer node id) of the device.
   */
  void Device::SetLocation(int loc){
    assert(loc >= 0);
    location = loc;
  }

  /** Record the data trandmission time of the device.
      This value is not very accurate at present,
      since the transmission overlaping is not considered.
  */
  void Device::IncreaseTransTime(float TransTime){
    assert(data_trans_time >= ZERO_NEGATIVE);
    assert(TransTime >= ZERO_NEGATIVE);
    data_trans_time += TransTime;
  }

  /** Increase the execution time of the device.
   */
  void Device::IncreaseRunTime(float ExeTime){
    assert(execution_time >= ZERO_NEGATIVE);
    assert(ExeTime >= ZERO_NEGATIVE);
    execution_time += ExeTime;
  }

  /** Set available time of the device.
      This is usually the time that
      it finishes the last task in its queue.
  */
  void Device::SetAvaTime(float time){
    assert(time >= ZERO_NEGATIVE);

    available_time = time;
  }

  /** Malloc a memory block*/
  void Device::MemAlloc(float size){
    assert(Allocated_RAM + size <= RAM_size + ZERO_POSITIVE);
    Allocated_RAM += size;

#ifdef DEBUG
    std::cout<<"----MemAlloc on device "<<id_<<", allocated RAM: "<<Allocated_RAM<<" KB, total RAM: "<<RAM_size<<" KB."<<std::endl;
#endif
  }

  /** Free a memory block*/
  void Device::MemFree(float size){
    assert(size <= Allocated_RAM + ZERO_POSITIVE);
    Allocated_RAM = std::max((float)0, Allocated_RAM - size);

#ifdef DEBUG
    std::cout<<"----MemFree on device "<<id_<<", allocated RAM: "<<Allocated_RAM<<" KB, total RAM: "<<RAM_size<<" KB."<<std::endl;
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
      If found, return the actual start time;
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
    if (min_Ts >= 0){
      // A slot is found
      return std::max(min_Ts, ESTpred);
    }else{
      // Found nothing
      return min_Ts;
    }
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

  /** Get the id of the device.
   */
  int Device::GetId(){
    return id_;
  }

  /** Check whether the device is FREE.
   */
  bool Device::IsFree(){
    if (status == FREE){
      return true;
    }
    return false;
  }

  /** Check whether the device is BUSY.
   */
  bool Device::IsBusy(){
    if (status == BUSY){
      return true;
    }
    return false;
  }

  /** Get status of the device.
      This is not used at present,
      since IsFree() and IsBusy() are better.
  */
  Device::DeviceStatus Device::GetStatus(){
    return status;
  }

  /** Get the computation power of the device.
   */
  float Device::GetCompPower(){
    return computing_power;
  }

  /** Get the total RAM size of the device.
   */
  float Device::GetRAM(){
    return RAM_size;
  }

  /** Get the free RAM size of the device.
   */
  float Device::GetFreeRAM(){
    assert(RAM_size >= Allocated_RAM + ZERO_NEGATIVE);
    return (RAM_size - Allocated_RAM);
  }

  /** Get the (memory access) bandwidth of the device.
   */
  float Device::GetBw(){
    return bandwidth;
  }

  /** Get the location (computer node id) of the device.
   */
  int Device::GetLocation(){
    return location;
  }

  /** Return the data transmission time of the device.
   */
  float Device::GetTransTime(){
    assert(data_trans_time >= ZERO_NEGATIVE);
    return data_trans_time;
  }

  /** Return the total execution time of the device.
   */
  float Device::GetRunTime(){
    assert(execution_time >= ZERO_NEGATIVE);
    return execution_time;
  }

  /** Get the available time of the device.
   */
  float Device::GetAvaTime(){
    assert(available_time >= ZERO_NEGATIVE);

    return available_time;
  }

  /** Set finished_tasks value.
   */
  void Device::SetTasks(int tasks){
    assert(tasks >= 0);
    this->finished_tasks = tasks;
  }

  /** Increase finished_tasks.
   */
  void Device::IncreaseTasks(int tasks){
    assert(tasks > 0);
    this->finished_tasks += tasks;
  }

  /** Get finished_tasks.
   */
  int Device::GetTasks(){
    return this->finished_tasks;
  }

  // The Connections class
  Connections::Connections(){
    NodeConNum = 0;
    DevConNum = 0;
  }

  Connections::~Connections(){
    NodeConnection.clear();
    DeviceConnection.clear();

    assert(NodeConnection.empty());
    assert(DeviceConnection.empty());
  }

  /**  Add a new link between src and dst
       to the connection.
       bw: bandwidth
       BetweenNode: whether between computer nodes or between two devices
  */
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
      NodeConNum++;
    }else{
      DeviceConnection[std::pair<int, int>(src, dst)] = bw;
      DevConNum++;
    }
    
  }

  /** Get bandwidth
      between divices or between nodes.
  */
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

  /** Return NodeConNum
   */
  int Connections::GetNodeConNum(){
    return this->NodeConNum;
  }

  /** Return DevConNum
   */
  int Connections::GetDevConNum(){
    return this->DevConNum;
  }


  /** Clean all the connections.
   */
  void Connections::Clear(){
    NodeConnection.clear();
    DeviceConnection.clear();
  }

} // namespace triplet
