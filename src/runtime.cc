// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "runtime.h"
#include <cassert>

namespace triplet{
  Runtime::Runtime(){
    global_timer = 0.0;
    deviceNum = 0;
    deviceInUse = 0;
  }

  Runtime::~Runtime(){
    global_timer = 0.0;
    deviceNum = 0;
    deviceInUse = 0;

    TaihuLight.clear();
    TaihuLightNetwork.clear();
    glocal_graph.clear();
    ready_queue.clear();
    execution_queue.clear();
    pending_list.clear();
  }

  /**
     Init the global graph from configure file.
   */
  void Runtime::InitGraph(){
  }

  /**
     Init the cluster "TaihuLight" from configure file.
   */
  void Runtime::InitCluster(){
  }

  void Runtime::InitPendingList(){
    for (std::set<int>::iterator iter = idset.begin(); iter != idset.end(); iter++){
      int pend = global_graph.GetNode(*iter).GetInNum();
      assert(pend >= 0);
      pending_list[*iter] = pend;
    }
  }

  void Runtime::StartExecution(){
    // execute until all three queues/lists are empty
    while (!pending_list.empty() || !ready_queue.empty() || !execution_queue.empty()) {

      // 1. if a task finished execution, update pending_list, cluster and ready_queue
      std::map<int, float>::iterator it = execution_queue.begin();
      for (; it != execution_queue.end(); it++){
	if (it->second >= global_timer){
	  
	  // Set free the corresponding device
	  Node nd = global_graph.GetNode(it->first);
	  int devId = nd.GetOccupied();
	  TaihuLight[devId].SetFree();

	  // update pending list and ready queue
	  std::set<int>::iterator ndit;
	  for (ndit = nd.output.begin(); ndit != nd.output.end(); ndit ++){
	    int pendingNum = pending_list[*ndit];
	    pendingNum --;
	    assert(pendingNum >= 0);
	    pending_list[*ndit] = pendingNum;

	    if (pendingNum == 0){
	      ready_queue.push_back(*ndit);
	    }
	  }

	  // erase the task from execution_queue
	  execution_queue.erase(it);

	}
      }
      
      // 2. if cluster contains free devices, process a new task from ready queue and update global_timer, deviceInUse

      // 3. if ready queue is empty or all devices are busy, update global_timer to the nearest finish time
      if ( ready_queue.empty() || (deviceInUse == deviceNum)){
	global_timer = CalcNearestFinishTime();
      }
      
    }

    // Finish running
    // Write simulation report.
    SimulationReport();
  }

  float Runtime::CalcNearestFinishTime(){
    float NearestTime = -1.0;
    std::map<int, float>::iterator ite;
    for (ite = execution_queue.begin(); ite != execution_queue.end(); ite++){
      if (NearestTime > ite->second || NearestTime < 0.0){
	NearestTime = ite->second;
      }
    }
    return NearestTime;
  }
}
