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
  class Runtime{
  public:
    Runtime();
    ~Runtime();

    void InitGraph(const char * graphFile);
    void InitCluster(const char * clusterFile);
    void InitPendingList();
    void StartExecution();
    float CalcNearestFinishTime();
    void SimulationReport();

    typedef std::map<int, Device*> Cluster;
    
  protected:
    int deviceNum;
    int deviceInUse;
    Graph global_graph;
    Cluster TaihuLight;
    Connections TaihuLightNetwork;
    float global_timer;
    std::set<int> idset;  //Node id set
    std::vector<int> ready_queue; // nodeid
    std::map<int, float> execution_queue; // nodeid -> execution finish time
    std::map<int, int> pending_list; // nodeid -> pending input
  };
}

#endif //TRIPLET_RUNTIME_H_
