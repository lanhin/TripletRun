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
    void IncreaseRuntime(float ExeTime);
    
    int GetId();
    bool IsFree();
    bool IsBusy();
    DeviceStatus GetStatus();
    float GetCompPower();
    int GetRAM();
    float GetBw();
    int GetLocation();
    float GetRuntime();
    
    int id_;
    float computing_power;
    int RAM_size;
    float bandwidth;
    DeviceStatus status;
    int location; //on which node does the device locate
    float occupied_time;
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
