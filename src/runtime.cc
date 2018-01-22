// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "runtime.h"
#include "constants.h"
#include "json/json.h"
#include "utils.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cmath>

namespace triplet{
  //Class Runtime
  Runtime::Runtime(){
    ETD = false;
    global_timer = 0.0;
    deviceNum = 0;
    deviceInUse = 0;
    OCT = NULL;
    max_devId = -1;
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

  /** Init the global graph from configure JSON file.
   */
  void Runtime::InitGraph(const char * graphFile){
    Json::CharReaderBuilder reader;
    std::string errs;
    Json::Value root;

    log_start("Graph initialization...");

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

    for (int index = 0; index < root["nodes"].size(); index++){
      std::string id = root["nodes"][index].get("id", "-1").asString();
      std::string computeDemand = root["nodes"][index].get("comDmd", "-1.0").asString();
      std::string dataDemand = root["nodes"][index].get("dataDmd", "-1.0").asString();
      int id1 = std::stoi(id);
      float comDmd1 = std::stof(computeDemand, 0);
      float dataDmd1 = std::stof(dataDemand, 0);
      global_graph.AddNode(id1, comDmd1, dataDmd1);
      idset.insert(id1);

#if 0
      std::cout<<"Node "<<id1<<", com demand: "<<comDmd1<<", data demand: "<<dataDmd1<<std::endl;
#endif
    }

    int i = 0; // Index for available edge weight
    for (int index = 0; index < root["edges"].size(); index++){
      std::string src = root["edges"][index].get("src", "-1").asString();
      std::string dst = root["edges"][index].get("dst", "-1").asString();
      std::string comCost = root["edges"][index].get("weight", "-1").asString();
      int src1 = std::stoi(src);
      int dst1 = std::stoi(dst);
      float comCost1 = std::stof(comCost);
      global_graph.AddEdge(src1, dst1, comCost1);

#if 0
      std::cout<<"Edge "<<src1<<" -> "<<dst1<<", cost: "<<global_graph.GetComCost(src1, dst1)<<std::endl;
#endif
    }

    // Summary report of the graph
    global_graph.SummaryReport();
#if 0
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
#endif

    log_end("Graph initialization.");
  }

  /** Init the cluster "TaihuLight" from configure file.
   */
  void Runtime::InitCluster(const char * clusterFile){
    Json::CharReaderBuilder reader;
    std::string errs;
    Json::Value root;

    log_start("Cluster initialization...");

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

    for (int index = 0; index < root["devices"].size(); index++){
      std::string id = root["devices"][index].get("id", "-1").asString();
      std::string compute = root["devices"][index].get("compute", "-1").asString();
      std::string RAM = root["devices"][index].get("RAM", "-1").asString();
      std::string bw = root["devices"][index].get("bw", "-1").asString();
      std::string loc = root["devices"][index].get("loc", "-1").asString();

      int id1 = std::stoi(id);
      float compute1 = std::stof(compute, 0);
      float RAM1 = std::stof(RAM);
      float bw1 = std::stof(bw, 0);
      int loc1 = std::stoi(loc);

#if 0
      std::cout<<id1<<' '<<compute1<<' '<<RAM1<<' '<<bw1<<' '<<loc1<<std::endl;
#endif
      Device *dev = new Device(id1, compute1, RAM1, bw1, loc1);
      TaihuLight[id1] = dev;
      deviceNum ++;
      max_devId = std::max(max_devId, id1);
      computerset.insert(loc1);
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

#if 0
      std::cout<<src1<<' '<<dst1<<' '<<bw1<<' '<<btNodes1<<std::endl;
#endif
      TaihuLightNetwork.NewLink(src1, dst1, bw1, btNodes1);
    }

    //TODO: Check the constructed cluster
    std::cout<<"-------- Cluster information --------"<<std::endl;
    std::cout<<" Total devices: "<<this->deviceNum<<std::endl;
    std::cout<<" Max device id: "<<this->max_devId<<std::endl;
    std::cout<<" Total computer nodes: "<<this->computerset.size()<<std::endl;
    std::cout<<" Links among computer nodes: "<<this->TaihuLightNetwork.GetNodeConNum()<<std::endl;
    std::cout<<" Links among devices: "<<this->TaihuLightNetwork.GetDevConNum()<<std::endl;
    std::cout<<"-------------------------------------"<<std::endl;

    log_end("Cluster initialization.");
  }

  /** Init the runtime data structures: pending_list and ready_queue
      and calculate the OCT, RankOCT of the graph.
  */
  void Runtime::InitRuntime(SchedulePolicy sch){
    log_start("Runtime initialization...");

    // TODO: Set Scheduler according to the command options.
    Scheduler = sch;
    RRCounter = -1; // Always set it -1 at the beginning of execution?

    // Init ready_queue
    for (std::set<int>::iterator iter = idset.begin(); iter != idset.end(); iter++){
      int pend = global_graph.GetNode(*iter)->GetInNum();
      assert(pend >= 0);

#if 0
      std::cout<<"Node id "<<*iter<<", pending number:"<<pend<<std::endl;
#endif

      pending_list[*iter] = pend;

      if (pend == 0){ //add it into ready_queue
	ready_queue.push_back(*iter);
      }
    }

    log_start("OCT calculation...");
    CalcOCT(); //OCT for PEFT
    log_end("OCT calculation.");

    log_start("Rank OCT calculation...");
    CalcRankOCT(); //RankOCT for PEFT
    log_end("Rank OCT calculation.");

    log_start("OCCW initialization...");
    global_graph.InitAllOCCW(); //OCCW for HSIP
    log_end("OCCW initialization.");

    log_start("Rank_u calculation...");
    CalcRank_u(); // Rank_u for HSIP
    log_end("Rank_u calculation.");

    log_end("Runtime initialization.");
  }

  /** Calculate the OCT (Optimistic Cost Table) used in PEFT.
      This function cannot be moved into class Graph
      since it involves not only the graph topology but also the cluster configures.
  */
  void Runtime::CalcOCT(){

    /** Only for testing.
	May need to move this into the runtime class definition
	since some other member functions also need it.
     */
    int CTM[][3] = {22, 21, 36, 22, 18, 18, 32, 27, 43, 7, 10, 4, 29, 27, 35, 26, 17, 24, 14, 25, 30, 29, 23, 36, 15, 21, 8, 13, 16, 33};

    /** 1. Find the exit vertex, if multiple, creat a new "sink" vertex.
     */
    int maxVertexId = 0;
    int sinkId;
    std::set<int> exitVertexSet;
    for (std::set<int>::iterator iter = idset.begin(); iter != idset.end(); iter++){
      if (maxVertexId < *iter){
	maxVertexId = *iter;
      }
      int outputDegree = global_graph.GetNode(*iter)->GetOutNum();
      if (outputDegree == 0){ //an exit node
	exitVertexSet.insert(*iter);
	sinkId = *iter;
      }
    }

    if(exitVertexSet.size() > 1){ // Multiple exit vertices
      // create a new "sink" vertex
      sinkId = maxVertexId + 1;
      global_graph.AddNode(sinkId, 0, 0);
      idset.insert(sinkId);
      for (auto& it : exitVertexSet){
	global_graph.AddEdge(it, sinkId, 0);
      }
    }


    /** 2. Traversing the DAG from the exit to the entry vertex
	and calculate the OCT. Deal the cross-level edges carefully.
     */

    // 2.1 Create OCT: first tasks then processors
    /** TODO: Use the max task and device ids
	instead of the number of them.
	Or we need to preprocess the graph and device conf files
	to make them equal.
     */
    int tasks = global_graph.MaxNodeId() + 1;//global_graph.Nodes();
    int devs = this->max_devId + 1;//this->deviceNum;
    assert(tasks >= 1);
    assert(devs >= 1);
    OCT = new float*[tasks];
    for(int i = 0; i < tasks; i++ ){
      OCT[i] = new float[devs];
    }

    // Init value: -1.
    for (int i = 0; i < tasks; i++) {
      for (int j = 0; j < devs; j++) {
	OCT[i][j] = -1;
      }
    }

    std::set<int> recent; // Store the vertex ids that calculated recently
    // 2.2 Calculate the sink vertex row
    for (int i = 0; i < devs; i++) {
      OCT[sinkId][i] = 0;
    }
    recent.insert(sinkId);

    // 2.3 Calculate the other rows
    while(!recent.empty()){
      auto it = *(recent.begin());// The first value stored
      recent.erase(recent.begin());
      //for (auto& it : recent){
      Node* nd = global_graph.GetNode(it);
      for (auto& crtVertId : nd->input){
	/** Calculate the whole row for crtVertId
	 */
	// 2.3.0 If it has already been calculated, continue!
	if (OCT[crtVertId][0] >= 0){
	  continue;
	}

	// 2.3.1 If not all of crtVertId's output has been calculated, continue!
	Node* crtNd = global_graph.GetNode(crtVertId);
	bool allSatisfied = true;
	for(auto& succ : crtNd->output){
	  if(OCT[succ][0] < 0){
	    allSatisfied = false;
	  }
	}
	if (!allSatisfied){
	  continue;
	}

	// 2.3.2 Calculate the whole row in OCT
	float current, min, max;
	for (int devId = 0; devId < devs; devId++) {
	  max = 0;
	  for(auto& vertId : crtNd->output){
	    Node* vertex = global_graph.GetNode(vertId);
	    min = -1;
	    for(auto& dev : TaihuLight){
	      if(OCT[vertex->GetId()][dev.first] < 0){ // The OCT item has not been calculated
		continue;
	      }else{
		current = OCT[vertex->GetId()][dev.first] + std::max(((float)vertex->GetCompDmd()) / (dev.second)->GetCompPower(), (float)1.0) + ((dev.first == devId)?0:global_graph.GetComCost(crtVertId, vertId));
		//current = OCT[vertex->GetId()][dev.first] + (CTM[vertId][dev.first]) + ((dev.first == devId)?0:global_graph.GetComCost(crtVertId, vertId));
		if (min > current || min < 0)
		  min = current;
	      }
	    }
	    if (max < min){
	      max = min;
	    }
	  }
	  OCT[crtVertId][devId] = max;
	}

	// 2.3.3 Add crtVertId into recent after calculation
	recent.insert(crtVertId);
      }
    }

    /** 3. Check the OCT if the macro DEBUG is defined.
     */
#if 0
    std::cout<<"The OCT result:"<<std::endl;
    for (int i = 0; i < tasks; i++) {
      for (int j = 0; j < devs; j++) {
	std::cout<<OCT[i][j]<<" ";
      }
      std::cout<<std::endl;
    }
#endif
  }

  /** Calculate the rank_oct used in PEFT based on OCT.
   */
  void Runtime::CalcRankOCT(){
    for (int ndId : idset){
      Node* nd = global_graph.GetNode(ndId);
      int rowOCT = 0;
      for (int i = 0; i < this->deviceNum; i++) {
	rowOCT += OCT[ndId][i];
      }
      nd->SetRankOCT( ((float)rowOCT)/this->deviceNum );

#if 0
      std::cout<<"Vertex "<<ndId<<", rank oct: "<<nd->GetRankOCT()<<std::endl;
#endif
    }
  }

  /** Calculate the computation cost mean value and standard deviation value
      of node ndId on different devices.
      For convinience, return weight_mean * sd directly.
  */
  float Runtime::CalcWeightMeanSD(int ndId){
    double SD = 0;
    // 1. Calculate the average weight and record it in node.
    float tmp_mean_weight;
    Node* nd = global_graph.GetNode(ndId);
    if ( (tmp_mean_weight = nd->GetMeanWeight()) < 0 ) { //The mean weight has not been calculated
      tmp_mean_weight = 0;
      int i = 0;
      for (auto& it : TaihuLight){
	i++;
	tmp_mean_weight += (nd->GetCompDmd() / (it.second)->GetCompPower() - tmp_mean_weight) / i;
      }
      nd->SetMeanWeight(tmp_mean_weight);
    }

    // 2. Calculate the standard deviation value and return it.
    for (auto& it : TaihuLight) {
      SD += std::pow((double)(nd->GetCompDmd() / (it.second)->GetCompPower() - tmp_mean_weight), 2);
    }
    SD = std::sqrt(SD);

    // 3. Return result
    return (float)SD * tmp_mean_weight;
  }

  /** Calculate rank_u, which is used in HSIP policy.
   */
  void Runtime::CalcRank_u(){

    /** 1. Find the exit vertex, if multiple, creat a new "sink" vertex.
     */
    int maxVertexId = 0;
    int sinkId;
    std::set<int> exitVertexSet;
    for (std::set<int>::iterator iter = idset.begin(); iter != idset.end(); iter++){
      if (maxVertexId < *iter){
	maxVertexId = *iter;
      }
      int outputDegree = global_graph.GetNode(*iter)->GetOutNum();
      if (outputDegree == 0){ //an exit node
	exitVertexSet.insert(*iter);
	sinkId = *iter;
      }
    }

    if(exitVertexSet.size() > 1){ // Multiple exit vertices
      // create a new "sink" vertex
      sinkId = maxVertexId + 1;
      global_graph.AddNode(sinkId, 0, 0);
      idset.insert(sinkId);
      for (auto& it : exitVertexSet){
	global_graph.AddEdge(it, sinkId, 0);
      }
    }

    /** 2. Traversing the DAG from the exit to the entry vertex
	and calculate the rank_u. Deal the cross-level edges carefully.
    */

    std::set<int> recent; // Store the vertex ids that calculated recently
    float rank;
    // 2.1 Calculate sink node's rank_u
    Node* nd = global_graph.GetNode(sinkId);
    rank = CalcWeightMeanSD(sinkId);
    nd->SetRank_u(rank);
    recent.insert(sinkId);

    // 2.2 Calculate all the others
    while ( !recent.empty() ){
      auto it = *(recent.begin());// The first value stored
      recent.erase(recent.begin());
      Node* nd = global_graph.GetNode(it);
      for (auto& crtVertId : nd->input){

	Node* crtNd = global_graph.GetNode(crtVertId);
	// 2.2.0 If it has already been calculated, continue
	if (crtNd->GetRank_u() >= 0){
	  continue;
	}

	// 2.2.1 If not all of the output nodes' rank_u have been calculated, continue!
	bool allSatisfied = true;
	for(auto& succ : crtNd->output){
	  Node* succNd = global_graph.GetNode(succ);
	  if (succNd->GetRank_u() < 0){
	    allSatisfied = false;    
	  }
	}
	if (!allSatisfied){
	  continue;
	}

	// 2.2.2 Calculate rank_u for current vertex
	float max_ranku = 0;
	for(auto& succ : crtNd->output){
	  Node* succNd = global_graph.GetNode(succ);
	  if (max_ranku < (succNd->GetRank_u()) ){
	    max_ranku = succNd->GetRank_u();
	  }
	}
	rank = CalcWeightMeanSD(crtVertId) + crtNd->GetOCCW() + max_ranku;
	crtNd->SetRank_u(rank);

	// 2.2.3 Add crtVertId into recent after calculation
	recent.insert(crtVertId);
      }
    }
  }

  /** The whole execution logic.
   */
  // TODO: Count the schduling time itself
  void Runtime::Execute(){
    log_start("Execution...");

    // Execute until all three queues/lists are empty
    while (!ready_queue.empty() || !execution_queue.empty() || !block_free_queue.empty()) {
      /** 0. if a memory block's refer number can be decreased
          decrease it and check if we need to free the block.
      */
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

      /** 1. If a task finished execution,
	  update pending_list, cluster and ready_queue
       */
      std::map<int, float>::iterator it = execution_queue.begin();
      for (; it != execution_queue.end();){
	if (it->second <= (global_timer + ZERO_POSITIVE)){
	  
	  // Set free the corresponding device
	  Node* nd = global_graph.GetNode(it->first);
	  int devId = nd->GetOccupied();
#ifdef DEBUG
	  std::cout<<"Node "<<it->first<<" finished execution. It used device "<<devId<<std::endl;
#endif
	  // Set the finished_tasks properly
	  TaihuLight[devId]->IncreaseTasks(1);

	  if(TaihuLight[devId]->GetAvaTime() <= global_timer){
	    // Only set free the ones that are really free.
	    TaihuLight[devId]->SetFree();
	    deviceInUse --;
	  }

	  //fill running_history map
	  // TODO: do we really need this? I think it can be removed
	  running_history[nd->GetId()] = devId;

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
	  execution_queue.erase(it++);

#if 0
	  // Output the execution_queue to check its contents
	  std::cout<<"Execution queue: "<<std::endl;
	  // TODO: empty queue compatibility
	  for (auto& x: execution_queue)
	    std::cout << " [" << x.first << ':' << x.second << ']'<< std::endl;
#endif
	}else{
	  it++;
	}
      }
      /** 2. If the ready queue is not empty, process a new task from it
	  and update global_timer, deviceInUse
       */
      while ( (!ready_queue.empty()) ) {
	// TODO: 1. consider schedule time here;
 
	//2.1 pick a task from ready_queue (default: choose the first one element)
	int task_node_id = TaskPick();
	Node* nd = global_graph.GetNode(task_node_id);

	//2.2 choose a free device to execute the task (default: choose the first free device)
	/** If this is the only entry task and Scheduler is HSIP,
	    then use entry task duplication policy.
	 */
	if ( ready_queue.size() == 1 && Scheduler == HSIP && nd->GetInNum() == 0){
	  //Entry task duplication
	  EntryTaskDuplication(nd);
	  continue;
	}
	// Entry task duplication policy doesn't need this.
	Device* dev = DevicePick(task_node_id);

	/** If dev == NULL, re-insert the node into ready_queue,
	    and then calculate nearest finish time.

	    TODO: dead loop detection.
	    When no device's free RAM can cover any node's demand
	    in ready_queue and execution_queue is empty, then a
	    dead loop is detected.
	    For a dead loop, report it and stop the simulation.
	 */
	//assert(dev != NULL);
	if ( dev == NULL ){
	  if ( DeadLoopDetect() ){
	    SimulationReport();
	    log_error("Execution interrupted, for dead loop detected.");
	    exit(1);
	  }
	  ready_queue.push_back(task_node_id);
	  global_timer = CalcNearestFinishTime();
	  break; // break the inner while loop
	}

	//2.3 do the schedule: busy the device, calc the finish time, allocate device memory and add the task into execution_queue

	/** Count deviceInUse.
	 */
	if ( dev->SetBusy() ){
	  deviceInUse ++;
	}

	nd->SetOccupied(dev->GetId());

	/** Note: since the scheduled tasks are ready tasks,
	    global_timer + transmission_time is the EST of this task.
	 */
	float transmission_time = CalcTransmissionTime(*nd, *dev);
	float execution_time = CalcExecutionTime(*nd, *dev);

	//Manage memory blocks data structures
	int block_id = nd->GetId();
	MemoryBlock* block = new MemoryBlock(block_id, dev->GetId(), std::max(nd->GetDataDmd(), (float)0.0), nd->GetOutNum());
	block->DoAlloc(TaihuLight);
	// TODO: check if the block id already exists, which is illegal
	BlocksMap[block_id] = block;

	for (std::set<int>::iterator iter = nd->input.begin(); iter != nd->input.end(); iter ++){
	  Node* input_nd = global_graph.GetNode(*iter);
	  block_free_queue.push_back(std::pair<int, float>(input_nd->GetId(), (transmission_time + global_timer)));
	}

	// Process the execution time
	float AST = -1; // Actual Start Time
	if ( (AST = dev->FindSlot(global_timer+transmission_time, execution_time)) >= 0 ){// Insertion into ITS.
	  // Update ITS
	  dev->UpdateSlot(AST, execution_time, global_timer);
	}else{// Not insertion, normal execution.
	  AST = std::max(dev->GetAvaTime(), global_timer + transmission_time);

	  assert(AST >= ZERO_NEGATIVE);

	  //Record the available time of the corresponding device
	  dev->SetAvaTime(AST + execution_time);

	  if( AST > dev->GetAvaTime()){
	    // Set the ITS of the scheduled device
	    dev->NewSlot(dev->GetAvaTime(), global_timer + transmission_time);
	  }
	}

	// Fill the execution_queue
	execution_queue.emplace(task_node_id, (AST + execution_time));

	//Record the AFT of the node/vertex
	nd->SetAFT(AST + execution_time);

	dev->IncreaseTransTime(transmission_time);
	dev->IncreaseRunTime(execution_time); // TODO: add transmission here as well?

#if 0
	std::cout<<"Execution queue: "<<std::endl;
	for (auto& x: execution_queue)
	  std::cout << " [" << x.first << ':' << x.second << ']'<< std::endl;

	std::cout<<"Block free queue: "<<std::endl;
	for (auto& x: block_free_queue)
	  std::cout << " [" << x.first << ':' << x.second << ']'<< std::endl;

	//Debug
	std::cout<<"Schedule node "<<task_node_id<<" onto Device "<<dev->GetId();
	std::cout<<", global time = "<<global_timer<<" s, expected transmission time = "<<transmission_time<<" s, execution time = "<<execution_time<<" s."<<std::endl;
	std::cout<<"Device ava time updated to: "<<dev->GetAvaTime()<<"s, Node AFT updated to: "<<nd->GetAFT()<<std::endl;
#endif
      }

      /** 3. If ready queue is empty,
	  update global_timer to the nearest finish time
       */
      if ( ready_queue.empty() ){
	global_timer = CalcNearestFinishTime();
      }

    }

    // Finish running
    // Write simulation report.
    SimulationReport();

    log_end("Execution.");
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
    }
      break;

    case PRIORITY:
      break;

    case HSIP:
    case PEFT:{
      /** Traverse all the tasks in the ready queue,
	  pick the one with the max priority.
       */
      float maxPriority = -1;
      std::vector<int>::iterator iter = ready_queue.begin();
      std::vector<int>::iterator maxIter; // Point to the task with max priority.
      for (; iter != ready_queue.end(); iter++){
	Node* nd = global_graph.GetNode(*iter);
	if(Scheduler == PEFT){
	  if (maxPriority < nd->GetRankOCT()){
	    maxPriority = nd->GetRankOCT();
	    maxIter = iter;
	    taskIdx = *iter;
	  }
	}else{ //HSIP
	  if (maxPriority < nd->GetRank_u()){
	    maxPriority = nd->GetRank_u();
	    maxIter = iter;
	    taskIdx = *iter;
	  }
	}
      }
      ready_queue.erase(maxIter);
    }
      break;

    case MULTILEVEL:
      break;

    case DATACENTRIC:
      /** If device in use is small, fetch a task from the ready queue tail.
	  Else, fetch a task from the head.
	  We need a flag var. For example, ...
       */
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
    std::cout<<"DevicePick: Scheduler "<<Scheduler<<", node "<<ndId<<std::endl;
#endif

    //int CTM[][3] = {22, 21, 36, 22, 18, 18, 32, 27, 43, 7, 10, 4, 29, 27, 35, 26, 17, 24, 14, 25, 30, 29, 23, 36, 15, 21, 8, 13, 16, 33};

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

    case HSIP:
    case PEFT:{
      /** Traverse all the devices,
	  pick the one with the min EFT.
       */
      float min_OEFT = -1;
      // 1.Traverse all the devices
      for (auto& it: TaihuLight){
	if( (nd->GetDataDmd()) > (it.second)->GetFreeRAM() + ZERO_POSITIVE ){
	  // The free memory of the device is too little, skip
	  continue;
	}

	float w = nd->GetCompDmd() / (it.second)->GetCompPower();

	// 2. Calculate EST by traversing all the pred(ndId)
	float EST = 0;
	float tmpEST;
	// TODO: The logic below can be replaced by CalcTransmissionTime()
	for (auto& pred : nd->input){
	  Node* predNd = global_graph.GetNode(pred);
	  if (predNd->GetOccupied() == it.first || (predNd->GetInNum() == 0 && this->ETD) ){
	    /** Execute on the same device
		or the pred node is an entry node with ETD == true
	     */
	    /** Note: since the scheduled tasks are ready tasks,
		tmpEST = global_timer
	     */
	    tmpEST = std::max(predNd->GetAFT(), global_timer);
	  }else{//on different device
	    float ct;// Comunication time
	    ct = TaihuLightNetwork.GetBw((it.second)->GetId(), predNd->GetOccupied()); //bandwith
	    if (ct <= BW_ZERO){//get bandwith between nodes
	      ct = TaihuLightNetwork.GetBw((it.second)->GetLocation(), TaihuLight[predNd->GetOccupied()]->GetLocation(), true);
	    }
	    ct = CommunicationDataSize(pred, ndId) / ct;

	    /** Note: since the scheduled tasks are ready tasks,
		tmpEST = global_timer + ct
	    */
	    tmpEST = std::max(predNd->GetAFT(), global_timer) + ct;
	  }
	  EST = std::max(tmpEST, EST);
	}

	// 3. Calculate EFT(nd, it) = EST + w
	// Two ways to calculate w for debugging
	if ( (it.second)->FindSlot(EST, w) >= ZERO_NEGATIVE ){
	  // Insertion into ITS
	  EST = (it.second)->FindSlot(EST, w);
	}else{
	  // Not Insertion
	  EST = std::max( EST, (it.second)->GetAvaTime() );
	}

	float EFT = EST + w;
	//float EFT = EST + CTM[ndId][it.first];

	// 4. Calculate OEFT = EFT + OCT for PEFT
	float OEFT;
	if (Scheduler == PEFT){
	OEFT = EFT + OCT[ndId][it.first];
	}else{ //HSIP
	  OEFT = EFT;
	}

	if ( (min_OEFT < 0) || (min_OEFT > OEFT) ){
	  min_OEFT = OEFT;
	  dev = it.second;
	}
      }
    }
      break;

    case MULTILEVEL:
      break;

    case DATACENTRIC:
      /** Pick the device according to the data location
	  or the min data transformation time.
       */
      break;

    default:
      std::cout<<"Error: unrecognized scheduling policy "<<Scheduler<<std::endl;
      exit(1);
    }

    return dev;
  }

  /** Calculate the nearest time that a new decision can be made.
   */
  float Runtime::CalcNearestFinishTime(){
    float NearestTime = -1.0;
    std::map<int, float>::iterator ite;

    // 1. Search the execution_queue
    for (ite = execution_queue.begin(); ite != execution_queue.end(); ite++){
      if (NearestTime > ite->second || NearestTime < ZERO_NEGATIVE){
	NearestTime = ite->second;
      }
    }

    // 2. Search the block_free_queue
    for (auto& ite: block_free_queue){
      if (NearestTime > ite.second  ||  NearestTime < ZERO_NEGATIVE){
	NearestTime = ite.second;
      }
    }

    if (NearestTime > ZERO_POSITIVE){
      return NearestTime;
    }
    else{
      return global_timer;
    }
  }

  /** Calculate the data transmission time if we put nd on dev,
      return the result as a float.
  */
  float Runtime::CalcTransmissionTime(Node nd, Device dev){
    float data_transmission_time=0.0;

    float total_data_output = 0.0;
    for (std::set<int>::iterator iter = nd.input.begin(); iter != nd.input.end(); iter ++){
      Node* input_nd = global_graph.GetNode(*iter);
      total_data_output += std::max(input_nd->GetDataDmd(), (float)0.0);
    }

    float network_bandwidth = 0.0;
    float data_trans_ratio = 1.0;
    if( total_data_output > nd.GetDataDmd() ){
      data_trans_ratio = std::max(nd.GetDataDmd(), (float)0.0) / total_data_output;
    }
    for (std::set<int>::iterator iter = nd.input.begin(); iter != nd.input.end(); iter ++){
      Node* input_nd = global_graph.GetNode(*iter);
      int input_dev_id = input_nd->GetOccupied();

      // On the same device, ignore the data transmission time
      // The input node is an entry node with entry task duplication, ignore the transmission time
      if (input_dev_id == dev.GetId() || (input_nd->GetInNum() == 0 && this->ETD)){
	continue;
      }

      // Get the bandwith between the two devices
      if ((network_bandwidth = TaihuLightNetwork.GetBw(dev.GetId(), input_dev_id)) <= BW_ZERO){
	network_bandwidth = TaihuLightNetwork.GetBw(dev.GetLocation(), TaihuLight[input_dev_id]->GetLocation(),  true);
      }

      float ct; // Communication time
      if ( (global_graph.GetComCost( *iter, nd.GetId() )) >= 0 ) {
	// The edge has a weight
	ct = global_graph.GetComCost( *iter, nd.GetId() ) / network_bandwidth;
      }else{
	// The edge doesn't have a weight
	ct = std::max(input_nd->GetDataDmd(), (float)0.0) * data_trans_ratio / network_bandwidth;
      }

      data_transmission_time = std::max(ct, data_transmission_time);
    }

#ifdef DEBUG
    std::cout<<"Data transmission time: "<<data_transmission_time<<std::endl;
#endif

    return data_transmission_time;
  }

  /** Calculate the execution time if we put nd on dev,
      return the result as a float.
  */
  float Runtime::CalcExecutionTime(Node nd, Device dev){
    // TODO: More accurate exection time calculation
    float calc_time, data_time;

    //1. calculation time
    calc_time = nd.GetCompDmd() / dev.GetCompPower();

    //2. data access time
    data_time = std::max(nd.GetDataDmd(), (float)0.0) / dev.GetBw();

    return std::max(calc_time, data_time);
  }

  /** Get the data size need to be transfered from predId to succId.
      Return it as a float value.
   */
  float Runtime::CommunicationDataSize(int predId, int succId){
    float dataSize = global_graph.GetComCost(predId, succId);
    if (dataSize < 0){//No edge weight found, need to calculate
      float total_data_output = 0.0;
      float data_trans_ratio = 1.0;

      // Calculate the total output size of succId's all pred nodes
      Node* nd = global_graph.GetNode(succId);
      for (auto& it : nd->input){
	Node* input_nd = global_graph.GetNode(it);
	total_data_output += std::max(input_nd->GetDataDmd(), (float)0.0);
      }

      // Get how much of the output data should be transfered.
      if (total_data_output > nd->GetDataDmd()){
	data_trans_ratio = std::max(nd->GetDataDmd(), (float)0.0) / total_data_output;
      }

      dataSize = std::max(global_graph.GetNode(predId)->GetDataDmd(), (float)0.0) * data_trans_ratio;
    }
    return dataSize;
  }

  /** Use entry task duplication policy on nd.
   */
  // TODO: Consider data transmission time of the succ nodes of the entry task.
  void Runtime::EntryTaskDuplication(Node* nd){
    /** 1. Calculate execution time on every device,
	set every device's available time by it and get the min execution time.
	Set busy every device and update device in use counter.
	Add the min execution time into execution queue.
     */
    float min_w = -1; //min execution time
    for (auto& it : TaihuLight) {
      float w = CalcExecutionTime(*nd, *(it.second));
      (it.second)->SetAvaTime(w);
      (it.second)->SetBusy();
      deviceInUse++;
      if (min_w < 0 || min_w > w){
	// Record the min_w and occupied device
	min_w = w;
	nd->SetOccupied((it.second)->GetId());
      }
    }
    assert(min_w >= ZERO_NEGATIVE);
    execution_queue.emplace(nd->GetId(), min_w);

    /** 2. Alloc memory block
     */
    int block_id = nd->GetId();
    MemoryBlock* block = new MemoryBlock(block_id, nd->GetOccupied(), std::max(nd->GetDataDmd(), (float)0.0), nd->GetOutNum());
    block->DoAlloc(TaihuLight);
    // TODO: check if the block id already exists, which is illegal
    BlocksMap[block_id] = block;

    /** 3. Erase the task from the ready queue
	Actually I think ready_queue.clear() is good enough.
     */
    for (std::vector<int>::iterator it = ready_queue.begin(); it != ready_queue.end();) {
      if (*it == nd->GetId()){
	//std::cout<<*it<<std::endl;
	ready_queue.erase(it);
      }else{
	it++;
      }
    }

    // 4. Set the ETD flag
    this->ETD = true;
  }

  /** Detect dead loop and return the result.
      If a dead loop is detected, report it and return true, otherwise return false.
  */
  bool Runtime::DeadLoopDetect(){
    /** 1. If the execution_queue is not empty or the ready_queue is empty, just return false.
     */
    if ( (!execution_queue.empty()) || ready_queue.empty() ){
      return false;
    }

    /** 2. Pick the min RAM demand from ready_queue
     */
    float min_data_demand = -1;
    for (auto& it : ready_queue) {
      Node* nd = global_graph.GetNode(it);
      if ( min_data_demand < 0 || min_data_demand > nd->GetDataDmd()){
	min_data_demand = nd->GetDataDmd();
      }
    }

    /** 3. Traverse all the devices, if no free RAM covers the min RAM demand, report the dead loop and return true!
     */
    bool dead_loop = true;
    for (auto& it : TaihuLight) {
      if ( (it.second)->GetFreeRAM() >= min_data_demand + ZERO_POSITIVE){
	dead_loop = false;
      }
    }

    if (dead_loop){
      // Dead loop report
      std::cout<< RED <<"\n****** Emergency! Dead loop detected ******"<< RESET <<std::endl;
      /** 1. Every device's free RAM size.
       */
      for (auto& it : TaihuLight) {
	std::cout<<"| Device "<<it.first<<", free RAM: "<<it.second->GetFreeRAM()<<std::endl;
      }

      /** 2. Memory block output
       */
      for (auto& it : BlocksMap) {
	std::cout<<"| Memory Block "<<it.first<<", on device:"<<(it.second)->DeviceLocation()<<", block size:"<<(it.second)->GetBlockSize()<<std::endl;
      }
      /** 3. Ready queue nodes' data demand
       */
      for (auto& it : ready_queue) {
	std::cout<<"| Node "<<it<<", data demand:"<<global_graph.GetNode(it)->GetDataDmd()<<std::endl;
      }
      std::cout<< RED <<"******************"<< RESET <<std::endl;
    }

    return dead_loop;
  }

  /** Output the simlulation report.
   */
  void Runtime::SimulationReport(){
    std::cout<<"-------- Simulation Report --------"<<std::endl;
    std::cout<<" Global timer:"<<global_timer<<std::endl;

    int devId, tasks;
    float occupyTime, dataTransTime;
    Cluster::iterator it = TaihuLight.begin();
    for(; it != TaihuLight.end(); it++){
      devId = it->first;
      occupyTime = (it->second)->GetRunTime();
      dataTransTime = (it->second)->GetTransTime();
      tasks = (it->second)->GetTasks();
      assert(devId >= 0);
      assert(occupyTime >= 0.0);

      std::cout<<" Device id:"<<devId<<"  occupied time:"<<occupyTime<<"  proportion:"<<occupyTime/global_timer<<"  data transfer time:"<<dataTransTime<<", finished number of tasks:"<<tasks<<std::endl;
    }
    std::cout<<"-----------------------------------"<<std::endl;

  }

  /** Return the global_graph.
      Just for testing.
  */
  Graph Runtime::GetGraph(){
    return global_graph;
  }

  /** Return TaihuLight.
      Just for testing.
  */
  Cluster Runtime::GetCluster(){
    return TaihuLight;
  }

  /** Return the execution_queue.
      Just for testing.
  */
  std::map<int, float> Runtime::GetExeQueue(){
    return execution_queue;
  }

  /** Return the ready_queue.
      Just for testing.
  */
  std::vector<int> Runtime::GetReadyQueue(){
    return ready_queue;
  }


  
  // Class MemoryBlock
  // TODO: Add unit test for MemoryBlock and remove the DEBUG blocks.
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

  /** Decrease the refer number and return the decreased number.
   */
  int MemoryBlock::DecRefers(int number){
    ReferNum -= number;
    ReferNum = std::max(0, ReferNum);

    return ReferNum;
  }

  /** Allocate the memory block physically
   */
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

  /** Free this memory block on corresponding device
   */
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

  /** Return the number of refers of this memory block
   */
  int MemoryBlock::GetRefers(){
    assert(ReferNum >= 0);
    return ReferNum;
  }

  /** Get device location.
   */
  int MemoryBlock::DeviceLocation(){
    return DeviceId;
  }

  /** Get block size.
   */
  int MemoryBlock::GetBlockSize(){
    return BlockSize;
  }

}
