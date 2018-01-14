// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#ifndef TRIPLET_DEVICE_H_
#define TRIPLET_DEVICE_H_

#include <map>

namespace triplet{
  class Device{

  public:
    Device();
    Device(int id, float compute, int RAM, float bw, int loc);
    ~Device();

    enum DeviceStatus {
      FREE,
      BUSY,
      UNAVAILABLE
    };

    void SetBusy();
    void SetFree();
    void SetId(int id);
    void SetCompPower(float compute);
    void SetRAM(int RAM);
    void SetBw(float bw);
    void SetLocation(int loc);
    void IncreaseTransTime(float TransTime);
    void IncreaseRunTime(float ExeTime);
    void SetAvaTime(float time);
    void MemAlloc(int size); // Malloc a memory block
    void MemFree(int size); // Free a memory block

    /** Add a new slot into ITS.
     */
    void NewSlot(float startTime, float endTime);

    /** Find a slot from ITS,
	which start no earlier than ESTpred and last at least W_ij.
	If found, return the actual start time;
	if not, return -1.
     */
    float FindSlot(float ESTpred, float W_ij);

    /** Update ITS,
	the parameters Exe_Ts and W_ij define an execution period.
     */
    void UpdateSlot(float Exe_Ts, float W_ij, float current_time=0);

    /** Show the ITS. Output all the slots one by one.
     */
    void ShowSlot();
    
    int GetId();
    bool IsFree();
    bool IsBusy();
    DeviceStatus GetStatus();
    float GetCompPower();
    int GetRAM();
    int GetFreeRAM();
    float GetBw();
    int GetLocation();
    float GetTransTime();
    float GetRunTime();
    float GetAvaTime();
    
    int id_;
    float computing_power;
    int RAM_size;
    int Allocated_RAM;  // number of RAM size in use
    float bandwidth;
    DeviceStatus status;
    int location; //on which node does the device locate
    float execution_time; // The time that the device are calculating
    float data_trans_time; // The time that the device are waiting for input data transmission
    float available_time; // The time that this device will be ready for next task
    std::map<float, float> ITS; // Idel time slot
  };

  class Connections{
  public:
    Connections();
    ~Connections();
    
    typedef std::map<std::pair<int, int>, float> connection;

    void NewLink(int src, int dst, float bw, bool BetweenNode=false); //Add a new link to the connection
    float GetBw(int src, int dst, bool BetweenNode=false); //Get bandwidth
    void Clear(); //clean all the connections
    
    connection NodeConnection;
    connection DeviceConnection;
  };

} //namespace triplet

#endif  // TRIPLET_DEVICE_H_
