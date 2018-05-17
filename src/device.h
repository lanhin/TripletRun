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
    Device(int id, float compute, float RAM, float bw, int loc);
    ~Device();

    enum DeviceStatus {
      FREE,
      BUSY,
      UNAVAILABLE
    };

    /** Set the device status to busy.
	If the previous status is not busy, return true,
	otherwise, return false.
   */
    bool SetBusy();

    /** Set the device status to free.
	If it's already free, import an error.
     */
    void SetFree();

    /** Set the id of the device.
     */
    void SetId(int id);

    /** Set (float) computation power of the device.
     */
    void SetCompPower(float compute);

    /** Set RAM size of the device.
     */
    void SetRAM(float RAM);

    /** Set memory bandwidth of the device.
     */
    void SetBw(float bw);

    /** Set location (computer node id) of the device.
     */
    void SetLocation(int loc);

    /** Record the data trandmission time of the device.
	This value is not very accurate at present,
	since the transmission overlaping is not considered.
     */
    void IncreaseTransTime(float TransTime);

    /** Increase the execution time of the device.
     */
    void IncreaseRunTime(float ExeTime);

    /** Set available time of the device.
	This is usually the time that
	it finishes the last task in its queue.
     */
    void SetAvaTime(float time);

    /** Malloc a memory block*/
    void MemAlloc(float size);

    /** Free a memory block*/
    void MemFree(float size);

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

    /** Get the id of the device.
     */
    int GetId();

    /** Check whether the device is FREE.
     */
    bool IsFree();

    /** Check whether the device is BUSY.
     */
    bool IsBusy();

    /** Get status of the device.
	This is not used at present,
	since IsFree() and IsBusy() are better.
     */
    DeviceStatus GetStatus();

    /** Get the computation power of the device.
     */
    float GetCompPower();

    /** Get the total RAM size of the device.
     */
    float GetRAM();

    /** Get the free RAM size of the device.
     */
    float GetFreeRAM();

    /** Get the (memory access) bandwidth of the device.
     */
    float GetBw();

    /** Get the location (computer node id) of the device.
     */
    int GetLocation();

    /** Return the data transmission time of the device.
     */
    float GetTransTime();

    /** Return the total execution time of the device.
     */
    float GetRunTime();

    /** Get the available time of the device.
     */
    float GetAvaTime();

    /** Set finished_tasks value.
     */
    void SetTasks(int tasks);

    /** Increase finished_tasks (by 1).
     */
    void IncreaseTasks(int tasks = 1);

    /** Get finished_tasks.
     */
    int GetTasks();
    
    /** Increase running_tasks (by 1).
     */
    void IncreaseLoad(int tasks = 1);

    /** Decrease running_tasks (by 1).
     */
    void DecreaseLoad(int tasks = 1);

    /** Return running_tasks.
     */
    int GetLoad();

  protected:
    int id_;
    float computing_power;
    float RAM_size; /* TODO: just int? */
    float Allocated_RAM;  // number of RAM size in use
    float bandwidth;
    DeviceStatus status;
    int location; //on which node does the device locate
    int finished_tasks; // The number of tasks finished by this device
    int running_tasks; // The number of tasks still running on this device
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

    /**  Add a new link between src and dst
	 to the connection.
	 bw: bandwidth
	 BetweenNode: whether between computer nodes or between two devices
     */
    void NewLink(int src, int dst, float bw, bool BetweenNode=false);

    /** Get bandwidth
	between divices or between nodes.
     */
    float GetBw(int src, int dst, bool BetweenNode=false); //Get bandwidth

    /** Return NodeConNum
     */
    int GetNodeConNum();

    /** Return DevConNum
     */
    int GetDevConNum();

    /** Get the mean bandwidth.
     */
    float GetMeanBW();

    /** Clean all the connections.
     */
    void Clear();

  protected:
    int NodeConNum;
    int DevConNum;
    float MeanBW; // Mean Bandwidth
    connection NodeConnection;
    connection DeviceConnection;
  };

} //namespace triplet

#endif  // TRIPLET_DEVICE_H_
