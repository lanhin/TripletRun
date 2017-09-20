// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "runtime.h"

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
    
  }

  void Runtime::StartExecution(){
  }
}
