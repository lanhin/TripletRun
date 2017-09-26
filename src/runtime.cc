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
  }

  Runtime::~Runtime(){
    ready_queue.clear();
    execution_queue.clear();
    pending_list.clear();
  }

  void Runtime::InitGraph(){
  }

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
  }
}
