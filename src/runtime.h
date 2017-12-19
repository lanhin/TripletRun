// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#ifndef TRIPLET_RUNTIME_H_
#define TRIPLET_RUNTIME_H_

#include "graph.h"
#include "device.h"
#include <vector>
#include <map>

namespace triplet{
  typedef std::map<int, Device*> Cluster;

  // Class MemoryBlock
  class MemoryBlock{
  public:
    MemoryBlock();
    MemoryBlock(int id, int devid, int size, int refers);
    ~MemoryBlock();

    int DecRefers(int number = 1); // Decrease the refer number (by 1)
    void DoAlloc(Cluster TaihuLight); // Allocate the memory block physically
    void DoFree(Cluster TaihuLight); // Free this memory block on corresponding device
    int GetRefers(); // Return the number of refers of this memory block

  protected:
    int BlockId; // Set it the same as node id is a good choice
    int DeviceId; // on which device this block locates
    int BlockSize;
    int ReferNum;
  };// Class MemoryBlock

  // Class Runtime
  class Runtime{
  public:
    Runtime();
    ~Runtime();

    void InitGraph(const char * graphFile);
    void InitCluster(const char * clusterFile);
    void InitRuntime();
    void Execute();
    float CalcNearestFinishTime();
    float CalcTransmissionTime(Node nd, Device dev);
    float CalcExecutionTime(Node nd, Device dev);
    void SimulationReport();
    
  protected:
    int deviceNum;
    int deviceInUse;
    //int blockIdCounter;
    Graph global_graph;
    Cluster TaihuLight;
    Connections TaihuLightNetwork;
    float global_timer;

    std::set<int> idset;  //Node id set
    std::vector<int> ready_queue; // nodeid
    std::map<int, int> running_history; //nodeid -> deviceid
    std::map<int, float> execution_queue; // nodeid -> execution finish time
    //std::map<int, float> block_free_queue; // blockid -> refer decrease time
    std::vector<std::pair<int,float>>block_free_queue;
    std::map<int, int> pending_list; // nodeid -> pending input
    std::map<int, MemoryBlock*> BlocksMap; // Memory blocks pool, blockid -> memory block
  };// Class Runtime
}

#endif //TRIPLET_RUNTIME_H_
