// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "runtime.h"
#include "constants.h"
#include "json/json.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>


namespace triplet{
  //Class Runtime
  Runtime::Runtime(){
    global_timer = 0.0;
    deviceNum = 0;
    deviceInUse = 0;
    //blockIdCounter = 0;
  }

  Runtime::~Runtime(){
    global_timer = 0.0;
    deviceNum = 0;
    deviceInUse = 0;

    Cluster::iterator it = TaihuLight.begin();
    for(; it != TaihuLight.end(); it++){
      delete it->second;
    }
    
    TaihuLight.clear();
    TaihuLightNetwork.Clear();
    global_graph.Clear();
    ready_queue.clear();
    execution_queue.clear();
    pending_list.clear();
  }

  /**
     Init the global graph from configure JSON file.
   */
  void Runtime::InitGraph(const char * graphFile){
    Json::CharReaderBuilder reader;
    std::string errs;
    Json::Value root;

    // check if the json file exists
    if (access(graphFile, F_OK) != 0){
      std::cout<<"The json file "<<graphFile<<" doesn't exist!"<<std::endl;
      return;
    }

    std::ifstream jsondoc(graphFile, std::ifstream::binary);

    bool parsingStatusOK = Json::parseFromStream(reader, jsondoc, &root, &errs);

    if (!parsingStatusOK){
      // report to the user the failure and their locations in the document.
      std::cout  << "Failed to parse configuration\n"
		 << errs;
      return;
    }

    //std::cout<<"InitGraph: Graph Parsed"<<std::endl;
    //std::cout<<root<<std::endl;

    //std::cout<<root["nodes"].size()<<std::endl;
    for (int index = 0; index < root["nodes"].size(); index++){
      std::string id = root["nodes"][index].get("id", "-1").asString();
      std::string computeDemand = root["nodes"][index].get("comDmd", "-1.0").asString();
      std::string dataDemand = root["nodes"][index].get("dataDmd", "-1.0").asString();
      int id1 = std::stoi(id);
      float comDmd1 = std::stof(computeDemand, 0);
      float dataDmd1 = std::stof(dataDemand, 0);
      global_graph.AddNode(id1, comDmd1, dataDmd1);
      idset.insert(id1);
      std::cout<<id1<<' '<<comDmd1<<' '<<dataDmd1<<std::endl;
    }

    for (int index = 0; index < root["edges"].size(); index++){
      std::string src = root["edges"][index].get("src", "-1").asString();
      std::string dst = root["edges"][index].get("dst", "-1").asString();
      int src1 = std::stoi(src);
      int dst1 = std::stoi(dst);
      std::cout<<src1<<' '<<dst1<<std::endl;
      global_graph.AddEdge(src1, dst1);
    }

    // Check the constructed graph
    for (int index = 0; index < root["nodes"].size(); index++){
      std::string id = root["nodes"][index].get("id", "-1").asString();
      int id1 = std::stoi(id);

      triplet::Node* nd = global_graph.GetNode(id1);
      std::cout<<std::endl<<" Node id: "<<id1<<std::endl;
      std::cout<<" Node computing demand: "<<nd->GetCompDmd()<<std::endl;
      std::cout<<" Node data demand: "<<nd->GetDataDmd()<<std::endl;
      std::cout<<" Node occupied: "<<nd->GetOccupied()<<std::endl;
      std::cout<<" Node input: "<<nd->GetInNum()<<std::endl;
      std::cout<<" Node output: "<<nd->GetOutNum()<<std::endl;
      std::cout<<"==========================="<<std::endl;
    }
  }

  /**
     Init the cluster "TaihuLight" from configure file.
   */
  void Runtime::InitCluster(const char * clusterFile){
    Json::CharReaderBuilder reader;
    std::string errs;
    Json::Value root;

    // check if the json file exists
    if (access(clusterFile, F_OK) != 0){
      std::cout<<"The json file "<<clusterFile<<" doesn't exist!"<<std::endl;
      return;
    }
    
    std::ifstream jsondoc(clusterFile, std::ifstream::binary);

    bool parsingStatusOK = Json::parseFromStream(reader, jsondoc, &root, &errs);

    if (!parsingStatusOK){
      // report to the user the failure and their locations in the document.
      std::cout  << "Failed to parse configuration\n"
		 << errs;
      return;
    }

    //std::cout<<"InitCluster: Cluster Parsed"<<std::endl;
    //std::cout<<root<<std::endl;

    for (int index = 0; index < root["devices"].size(); index++){
      std::string id = root["devices"][index].get("id", "-1").asString();
      std::string compute = root["devices"][index].get("compute", "-1").asString();
      std::string RAM = root["devices"][index].get("RAM", "-1").asString();
      std::string bw = root["devices"][index].get("bw", "-1").asString();
      std::string loc = root["devices"][index].get("loc", "-1").asString();

      int id1 = std::stoi(id);
      float compute1 = std::stof(compute, 0);
      int RAM1 = std::stoi(RAM);
      float bw1 = std::stof(bw, 0);
      int loc1 = std::stoi(loc);

      std::cout<<id1<<' '<<compute1<<' '<<RAM1<<' '<<bw1<<' '<<loc1<<std::endl;
      Device *dev = new Device(id1, compute1, RAM1, bw1, loc1);
      TaihuLight[id1] = dev;
      deviceNum ++;
    }

    for (int index = 0; index < root["links"].size(); index++){
      std::string src = root["links"][index].get("src", "-1").asString();
      std::string dst = root["links"][index].get("dst", "-1").asString();
      std::string bw = root["links"][index].get("bw", "-1").asString();
      std::string btNodes = root["links"][index].get("BetweenNode", "flase").asString();

      int src1 = std::stoi(src);
      int dst1 = std::stoi(dst);
      float bw1 = std::stof(bw, 0);
      bool btNodes1;
      std::istringstream(btNodes) >> std::boolalpha >> btNodes1;

      std::cout<<src1<<' '<<dst1<<' '<<bw1<<' '<<btNodes1<<std::endl;
      TaihuLightNetwork.NewLink(src1, dst1, bw1, btNodes1);
    }

    //TODO: Check the constructed cluster

  }

  // Init the runtime data structures: pending_list and ready_queue
  void Runtime::InitRuntime(){
    // TODO: Set Scheduler according to the command options.
    Scheduler = RR;
    RRCounter = -1;
    for (std::set<int>::iterator iter = idset.begin(); iter != idset.end(); iter++){
      int pend = global_graph.GetNode(*iter)->GetInNum();
      assert(pend >= 0);

      //if (VERBOSE)
      std::cout<<"Node id "<<*iter<<", pending number:"<<pend<<std::endl;

      pending_list[*iter] = pend;

      if (pend == 0){ //add it into ready_queue
	ready_queue.push_back(*iter);
      }
    }
  }

  void Runtime::Execute(){
    // execute until all three queues/lists are empty
    while (!ready_queue.empty() || !execution_queue.empty() || !block_free_queue.empty()) {
      /** 0. if a memory block's refer number can be decreased
          decrease it and check if we need to free the block. */
      //std::map<int, float>::iterator it = block_free_queue.begin();
      //      for (; it != block_free_queue.end(); it++){
      for (int i=0; i<block_free_queue.size(); i++){
	auto& it = block_free_queue[i];
	if (it.second <= (global_timer + ZERO_POSITIVE)){
	  MemoryBlock * blk_pointer = BlocksMap[it.first];
	  if ( (blk_pointer->DecRefers()) <= 0 ){ // do the free
	    blk_pointer->DoFree(TaihuLight);

	    //Remove the block entry from the BlockMap and block_free_queue
	    delete BlocksMap[it.first];
	    BlocksMap.erase(it.first);
	  }
	  // Erase the block free tag from block_free_queue
	  // once the time reached
	  block_free_queue.erase(block_free_queue.begin()+i);
	}
      }


      // 1. if a task finished execution, update pending_list, cluster and ready_queue
      //std::cout<<"stage 1"<<std::endl;
      std::map<int, float>::iterator it = execution_queue.begin();
      for (; it != execution_queue.end(); it++){
	if (it->second <= (global_timer + ZERO_POSITIVE)){
	  
	  // Set free the corresponding device
	  Node* nd = global_graph.GetNode(it->first);
	  int devId = nd->GetOccupied();
#ifdef DEBUG
	  std::cout<<"Node "<<it->first<<" finished execution. It used device "<<devId<<std::endl;
#endif
	  TaihuLight[devId]->SetFree();
	  deviceInUse --;

	  //fill running_history map
	  running_history[nd->GetId()] = devId;

	  //std::cout<<"Update pending list.."<<std::endl;
	  // update pending list and ready queue
	  std::set<int>::iterator ndit;
	  for (ndit = nd->output.begin(); ndit != nd->output.end(); ndit ++){
	    int pendingNum = pending_list[*ndit];
	    pendingNum --;
#ifdef DEBUG
	    std::cout<<"Node: "<<*ndit<<", pending num:"<<pendingNum<<std::endl;
#endif
	    assert(pendingNum >= 0);
	    pending_list[*ndit] = pendingNum;

	    if (pendingNum == 0){
	      ready_queue.push_back(*ndit);
	    }
	  }

	  // erase the task from execution_queue
	  execution_queue.erase(it);

#ifdef DEBUG
	  // Output the execution_queue to check its contents
	  std::cout<<"Execution queue: "<<std::endl;
	  for (auto& x: execution_queue)
	    std::cout << " [" << x.first << ':' << x.second << ']'<< std::endl;
#endif
	}
      }
      
      // 2. if cluster contains free devices, process a new task from ready queue and update global_timer, deviceInUse
      //std::cout<<"stage 2"<<std::endl;
      while ( (deviceInUse < deviceNum)  &&  (!ready_queue.empty()) ) {
	// TODO: 1. consider schedule time here; 2. record the occupy time of a device
 
	//2.1 pick a task from ready_queue (default: choose the first one element)
	int task_node_id = TaskPick();//ready_queue.front();
	//ready_queue.erase(ready_queue.begin());
	Node* nd = global_graph.GetNode(task_node_id);

	// TODO: Get the node's input data location
	
	
	//2.2 choose a free device to execute the task (default: choose the first free device)
	Device* dev = DevicePick(task_node_id);
	assert(dev != NULL);
	/*Cluster::iterator it = TaihuLight.begin();
	for(; it != TaihuLight.end(); it++){
	  if ((it->second)->IsFree() && ((nd->GetDataDmd())<=(it->second)->GetFreeRAM())){
	    break;
	  }
	  }*/

	//2.3 do the schedule: busy the device, calc the finish time, allocate device memory and add the task into execution_queue

	// TODO: add more operations when IT is the end of TaihuLight
	//assert(it != TaihuLight.end());

	//(it->second)->SetBusy();
	dev->SetBusy();
	deviceInUse ++;
	//nd->SetOccupied(it->first);
	nd->SetOccupied(dev->GetId());
	//std::cout<<"Device occupy: "<<nd->GetId()<<" occupys device "<<nd->GetOccupied()<<std::endl;
	//
	float transmission_time = CalcTransmissionTime(*nd, *dev);
	float execution_time = CalcExecutionTime(*nd, *dev);

	//Manage memory blocks data structures
	//blockIdCounter ++;
	int block_id = nd->GetId();
	MemoryBlock* block = new MemoryBlock(block_id, it->first, nd->GetDataDmd(), nd->GetOutNum());
	block->DoAlloc(TaihuLight);
	// TODO: check if the block id already exists, which is illegal
	BlocksMap[block_id] = block;

	for (std::set<int>::iterator iter = nd->input.begin(); iter != nd->input.end(); iter ++){
	  Node* input_nd = global_graph.GetNode(*iter);
	  block_free_queue.push_back(std::pair<int, float>(input_nd->GetId(), (transmission_time + global_timer)));
	}
	execution_queue.emplace(task_node_id, (transmission_time + execution_time + global_timer));

	dev->IncreaseTransTime(transmission_time);
	dev->IncreaseRunTime(execution_time); // TODO: add transmission here as well?

#ifdef DEBUG
	std::cout<<"Execution queue: "<<std::endl;
	for (auto& x: execution_queue)
	  std::cout << " [" << x.first << ':' << x.second << ']'<< std::endl;

	std::cout<<"Block free queue: "<<std::endl;
	for (auto& x: block_free_queue)
	  std::cout << " [" << x.first << ':' << x.second << ']'<< std::endl;

	//Debug
	std::cout<<"Schedule node "<<task_node_id<<" onto Device "<<dev->GetId();
	std::cout<<", global time = "<<global_timer<<" s, expected transmission time = "<<transmission_time<<" s, execution time = "<<execution_time<<" s."<<std::endl;
#endif
      }

      // 3. if ready queue is empty or all devices are busy, update global_timer to the nearest finish time
      //std::cout<<"stage 3"<<std::endl;
      if ( ready_queue.empty() || (deviceInUse == deviceNum)){
	global_timer = CalcNearestFinishTime();
      }
      
    }

    // Finish running
    // Write simulation report.
    SimulationReport();
  }

  /** Pick a task from ready queue according to the
      scheduling policy.
      Erase the correspinding task index from ready_queue.
   */
  int Runtime::TaskPick(){
#ifdef DEBUG
    std::cout<<"TaskPick: Scheduler "<<Scheduler<<std::endl;
#endif

    int taskIdx = -1;
    switch(Scheduler){
    case UNKNOWN:
      break;

    case FCFS:
    case RR:
      taskIdx = ready_queue.front();
      ready_queue.erase(ready_queue.begin());
      break;

    case SJF:{ // Pick the shortest job
      int tmpComDmd, leastComDmd = -1;
      std::vector<int>::iterator iter = ready_queue.begin();
      std::vector<int>::iterator leastIter;
      for (; iter != ready_queue.end(); iter++){
	if (leastComDmd < 0){ // the first node in ready_queue
	  Node * nd = global_graph.GetNode(*iter);
	  leastComDmd = nd->GetCompDmd();
	  leastIter = iter;
	  taskIdx = *iter;
	}else{
	  Node * nd = global_graph.GetNode(*iter);
	  tmpComDmd = nd->GetCompDmd();
	  if (leastComDmd > tmpComDmd){
	    leastComDmd = tmpComDmd;
	    leastIter = iter;
	    taskIdx = *iter;
	  }
	}
      }
      ready_queue.erase(leastIter);
      break;
    }

      break;
    case PRIORITY:
      break;

    default:
      std::cout<<"Error: unrecognized scheduling policy "<<Scheduler<<std::endl;
      exit(1);
    }

    return taskIdx;
  }

  /** Pick a free device according to the task requirements and
      scheduling policy.
   */
  Device* Runtime::DevicePick(int ndId){
#ifdef DEBUG
    std::cout<<"DevicePick: Scheduler"<<Scheduler<<std::endl;
#endif

    Node * nd = global_graph.GetNode(ndId);
    Device * dev = NULL;
    float exeTime = -1.0;
    switch(Scheduler){
    case UNKNOWN:
      break;

    case FCFS:
    case SJF:
    case PRIORITY:
      for (auto& it: TaihuLight){
	if((it.second)->IsFree() && ((nd->GetDataDmd())<=(it.second)->GetFreeRAM())){
	  float tmpExeTime = CalcExecutionTime(*nd, *(it.second));
	  //std::cout<<"node "<<nd->GetId()<<" on dev "<<(it.second)->GetId()<<" Exe time:"<<tmpExeTime<<std::endl;
	  if (exeTime < 0.0){
	    exeTime = tmpExeTime;
	    dev = it.second;
	  }else{
	    if (exeTime > tmpExeTime){
	      exeTime = tmpExeTime;
	      dev = it.second;
	    }
	  }
	}
      }
      break;

    case RR:
      for (int i = RRCounter+1; i<TaihuLight.size(); i++){
	Device * tmpDev = TaihuLight[i];
	if ( tmpDev->IsFree()  &&  ((nd->GetDataDmd())<=tmpDev->GetFreeRAM()) ){
	  dev = tmpDev;
	  RRCounter = i;
	  break;
	}
      }
      if (dev == NULL){// Haven't found a good device
	for (int i = 0; i<=RRCounter; i++){
	  Device * tmpDev = TaihuLight[i];
	  if ( tmpDev->IsFree()  &&  ((nd->GetDataDmd())<=tmpDev->GetFreeRAM()) ){
	    dev = tmpDev;
	    RRCounter = i;
	    break;
	  }
	}
      }
      break;

    default:
      std::cout<<"Error: unrecognized scheduling policy "<<Scheduler<<std::endl;
      exit(1);
    }

    return dev;
  }

  // TODO: add the memory free queue here
  float Runtime::CalcNearestFinishTime(){
    float NearestTime = -1.0;
    std::map<int, float>::iterator ite;
    for (ite = execution_queue.begin(); ite != execution_queue.end(); ite++){
      if (NearestTime > ite->second || NearestTime < ZERO_NEGATIVE){
	NearestTime = ite->second;
      }
    }

    //for (ite = block_free_queue.begin(); ite != block_free_queue.end(); ite++){
    for (auto& ite: block_free_queue){
      if (NearestTime > ite.second  ||  NearestTime < ZERO_NEGATIVE){
	NearestTime = ite.second;
      }
    }

    std::cout<<"****Nearest Time:"<<NearestTime<<std::endl;
    if (NearestTime > ZERO_POSITIVE){
      return NearestTime;
    }
    else{
      return global_timer;
    }
  }

  float Runtime::CalcTransmissionTime(Node nd, Device dev){
    float data_transmission_time=0.0;

    // 1. data transmission time, splited from CalcExecutionTime()
    float total_data_output = 0.0;
    for (std::set<int>::iterator iter = nd.input.begin(); iter != nd.input.end(); iter ++){
      Node* input_nd = global_graph.GetNode(*iter);
      total_data_output += input_nd->GetOutputSize();
    }

    float network_bandwith = 0.0;
    float data_trans_ratio = 1.0;
    if( total_data_output > nd.GetDataDmd() ){
      data_trans_ratio = nd.GetDataDmd() / total_data_output;
    }
    for (std::set<int>::iterator iter = nd.input.begin(); iter != nd.input.end(); iter ++){
      Node* input_nd = global_graph.GetNode(*iter);
      int input_dev_id = running_history[*iter];

      // On the same device, ignore the data transmission time
      if (input_dev_id == dev.GetId())
	continue;

      // Get the bandwith between the two devices
      if ((network_bandwith = TaihuLightNetwork.GetBw(dev.GetId(), input_dev_id)) <= BW_ZERO){
	network_bandwith = TaihuLightNetwork.GetBw(dev.GetLocation(), TaihuLight[input_dev_id]->GetLocation(),  true);
      }
      
      data_transmission_time = std::max((input_nd->GetOutputSize() * data_trans_ratio) / network_bandwith, data_transmission_time);
    }

    std::cout<<"Data transmission time: "<<data_transmission_time<<std::endl;

    return data_transmission_time;
  }

  // TODO: split the calculation and transmission time
  float Runtime::CalcExecutionTime(Node nd, Device dev){
    // TODO: More accurate exection time calculation
    float calc_time, data_time;

    //1. calculation time
    calc_time = nd.GetCompDmd() / dev.GetCompPower();

    //2. data access time
    data_time = nd.GetDataDmd() / dev.GetBw();

    return std::max(calc_time, data_time);
  }

  void Runtime::SimulationReport(){
    // TODO: implementation
    std::cout<<"****** Simulation Report ******"<<std::endl;
    std::cout<<"Global timer:"<<global_timer<<std::endl;

    int devId;
    float occupyTime, dataTransTime;
    Cluster::iterator it = TaihuLight.begin();
    for(; it != TaihuLight.end(); it++){
      devId = it->first;
      occupyTime = (it->second)->GetRunTime();
      dataTransTime = (it->second)->GetTransTime();
      assert(devId >= 0);
      assert(occupyTime >= 0.0);

      std::cout<<"Device id:"<<devId<<"  occupied time:"<<occupyTime<<"  proportion:"<<occupyTime/global_timer<<"  data transfer time:"<<dataTransTime<<std::endl;
    }

  }

  // Class MemoryBlock
  MemoryBlock::MemoryBlock()
    :BlockId(-1),
     DeviceId(-1),
     BlockSize(0),
     ReferNum(0){}

  MemoryBlock::MemoryBlock(int id, int devid, int size, int refers)
    :BlockId(id),
     DeviceId(devid),
     BlockSize(size),
     ReferNum(refers){}

  MemoryBlock::~MemoryBlock(){}

  // Decrease the refer number and return the decreased number
  int MemoryBlock::DecRefers(int number){
    ReferNum -= number;
    ReferNum = std::max(0, ReferNum);

    return ReferNum;
  }

  void MemoryBlock::DoAlloc(Cluster TaihuLight){
    assert(BlockId >= 0);
    assert(DeviceId >= 0);
    assert(BlockSize > 0);
    assert(ReferNum >= 0); // The sink node has ReferNum=0

#ifdef DEBUG
    std::cout<<"**Memory block allocate**"<<std::endl;
    std::cout<<"| BlockId:"<<BlockId<<std::endl;
    std::cout<<"| DeviceId:"<<DeviceId<<std::endl;
    std::cout<<"| BlockSize:"<<BlockSize<<std::endl;
    std::cout<<"| ReferNum:"<<ReferNum<<std::endl;
    std::cout<<"*************************"<<std::endl;
#endif
    TaihuLight[DeviceId]->MemAlloc(BlockSize);
  }

  void MemoryBlock::DoFree(Cluster TaihuLight){
    assert(DeviceId >= 0);
    assert(BlockSize >= 0);
    assert(ReferNum <= 0);

#ifdef DEBUG
    std::cout<<"**Memory block Free**"<<std::endl;
    std::cout<<"| BlockId:"<<BlockId<<std::endl;
    std::cout<<"| DeviceId:"<<DeviceId<<std::endl;
    std::cout<<"| BlockSize:"<<BlockSize<<std::endl;
    std::cout<<"| ReferNum:"<<ReferNum<<std::endl;
    std::cout<<"*************************"<<std::endl;
#endif
    TaihuLight[DeviceId]->MemFree(BlockSize);

  }

  int MemoryBlock::GetRefers(){
    assert(ReferNum >= 0);
    return ReferNum;
  }
}
