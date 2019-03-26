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
    max_step = 0;
    total_step = 1;
    step_done = -1;
    issue_width = 1;
    global_timer = 0.0;
    scheduler_ava_time = 0.0;
    scheduler_mean_cost = 0.0;
    deviceNum = 0;
    deviceInUse = 0;
    mem_full_dev = 0;
    OCT = NULL;
    max_devId = -1;
    task_total = 0;
    task_hit_counter = 0;
    dev_hit_counter = 0;
    dc_valid_counter = 0;
    mean_computing_power = 0;
    DCRatio = 0;
    max_parallel = 0;
    load_balance_threshold = 0;
    load_time = -1;
    with_conflicts = false;
    mem_full_threshold = 0.9;
    dev_full_threshold = 0.2;
    max_devCompute = 0.0;
    max_cpath_cc = 0.0;
    max_cpath_cc_mem = 0.0;
    absCP = 0.0;
    min_execution_time = 0.0;
    min_transmission_time = 0.0;
    alpha_DON = 0.5;
    min_free_mem = -1;

    graph_init_time = 0.0;
    cluster_init_time = 0.0;
    oct_time = 0.0;
    rankoct_time = 0;
    rank_u_time = 0;
    rank_d_time = 0;
    ndon_time = 0;
    task_pick_time = 0;
    device_pick_time = 0;

    graph_file_name = "";
    cluster_file_name = "";
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
    graph_file_name = graphFile;
    DECLARE_TIMING(graph);
    START_TIMING(graph);

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
      std::string dataDemand = root["nodes"][index].get("dataDmd", "1.0").asString();
      std::string dataConsume = root["nodes"][index].get("c", "-1.0").asString();
      std::string dataGenerate = root["nodes"][index].get("g", "-1.0").asString();
      std::string loc = root["nodes"][index].get("loc", "-1").asString();
      int id1 = std::stoi(id);
      int loc1 = std::stoi(loc);
      float comDmd1 = std::stof(computeDemand);
      float dataDmd1 = std::stof(dataDemand);
      float dataConsume1 = std::stof(dataConsume);
      float dataGenerate1 = std::stof(dataGenerate);
      dataDmd1 = std::max(dataDmd1, dataGenerate1);

      if (comDmd1 < 1){
	comDmd1 = 0.1;
      }
      global_graph.AddNode(id1, comDmd1, dataDmd1, dataConsume1, dataGenerate1, loc1);
      idset.insert(id1);

#if 0
      std::cout<<"Node "<<id1<<", com demand: "<<comDmd1<<", data demand: "<<dataDmd1<<", data consume: "<<dataConsume1<<", data generate: "<<dataGenerate1<<std::endl;
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

    for (int index = 0; index < root["ratios"].size(); index++){
      std::string id = root["ratios"][index].get("id", "-1").asString();
      std::string type = root["ratios"][index].get("type", "-1").asString();
      std::string ratio = root["ratios"][index].get("ratio", "1.0").asString();

      int id1 = std::stoi(id);
      int type1 = std::stoi(type);
      float ratio1 = std::stof(ratio);

      if(id1 >= 0 && type1 >= 0){
	global_graph.GetNode(id1)->AddRatio(type1, ratio1);

#if 0
	std::cout<<"Node "<<id1<<", type: "<<type1<<", speed ratio: "<<ratio1<<", recheck: "<<global_graph.GetNode(id1)->GetRatio(type1)<<std::endl;
#endif
      }
    }


    /** Add source and sink vertex if need.
     */
    /** 1. Find the entry and exit vertex,
	if multiple, creat new "source" and "sink" vertice.
     */
    int maxVertexId = 0;
    int sourceId, sinkId;
    std::set<int> entryVertexSet;
    std::set<int> exitVertexSet;
    for (std::set<int>::iterator iter = idset.begin(); iter != idset.end(); iter++){
      if (maxVertexId < *iter){
	maxVertexId = *iter;
      }

      int inputDegree = global_graph.GetNode(*iter)->GetInNum();
      if (inputDegree == 0){ //an entry node
	entryVertexSet.insert(*iter);
	sourceId = *iter;
      }

      int outputDegree = global_graph.GetNode(*iter)->GetOutNum();
      if (outputDegree == 0){ //an exit node
	exitVertexSet.insert(*iter);
	sinkId = *iter;
      }

    }

    if(entryVertexSet.size() > 1){ // Multiple entry vertices
      // create a new "source" vertex
      sourceId = ++maxVertexId;
      global_graph.AddNode(sourceId, 0.1, 0.1);
      idset.insert(sourceId);
      for (auto& it : entryVertexSet){
	global_graph.AddEdge(sourceId, it, 0);
      }
    }

    if(exitVertexSet.size() > 1){ // Multiple exit vertices
      // create a new "sink" vertex
      sinkId = ++maxVertexId;
      global_graph.AddNode(sinkId, 0.1, 0.1);
      idset.insert(sinkId);
      for (auto& it : exitVertexSet){
	global_graph.AddEdge(it, sinkId, 0);
      }
    }

    global_graph.SetSourceId(sourceId);
    global_graph.SetSinkId(sinkId);


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

    STOP_TIMING(graph);
    this->graph_init_time = GET_TIMING(graph);
    log_end("Graph initialization.");
  }

  /** Init the cluster "TaihuLight" from configure file.
   */
  void Runtime::InitCluster(const char * clusterFile){
    Json::CharReaderBuilder reader;
    std::string errs;
    Json::Value root;

    cluster_file_name = clusterFile;

    DECLARE_TIMING(cluster);
    START_TIMING(cluster);
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
      std::string type = root["devices"][index].get("type", "-1").asString();
      std::string compute = root["devices"][index].get("compute", "-1").asString();
      std::string RAM = root["devices"][index].get("RAM", "-1").asString();
      std::string bw = root["devices"][index].get("bw", "-1").asString();
      std::string loc = root["devices"][index].get("loc", "-1").asString();

      int id1 = std::stoi(id);
      int type1 = std::stoi(type);
      float compute1 = std::stof(compute, 0);
      float RAM1 = std::stof(RAM);
      float bw1 = std::stof(bw, 0);
      int loc1 = std::stoi(loc);

#if 0
      std::cout<<id1<<' '<<compute1<<' '<<RAM1<<' '<<bw1<<' '<<loc1<<std::endl;
#endif
      Device *dev = new Device(id1, compute1, RAM1, bw1, loc1, type1);
      TaihuLight[id1] = dev;
      deviceNum ++;
      max_devId = std::max(max_devId, id1);
      mean_computing_power += (compute1 - mean_computing_power) / (deviceNum);
      computerset.insert(loc1);

      //Record the max compute power
      if(this->max_devCompute < compute1){
	this->max_devCompute = compute1;
	this->max_computeDevId = id1;
      }
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
    std::cout<<" Max device compute power: "<<this->max_devCompute<<std::endl;
    std::cout<<" Total computer nodes: "<<this->computerset.size()<<std::endl;
    std::cout<<" Links among computer nodes: "<<this->TaihuLightNetwork.GetNodeConNum()<<std::endl;
    std::cout<<" Links among devices: "<<this->TaihuLightNetwork.GetDevConNum()<<std::endl;
    std::cout<<"-------------------------------------"<<std::endl;

    STOP_TIMING(cluster);
    this->cluster_init_time = GET_TIMING(cluster);
    log_end("Cluster initialization.");
  }

  /** Init the runtime data structures: pending_list and ready_queue
      and calculate the OCT, RankOCT of the graph.
  */
  void Runtime::InitRuntime(SchedulePolicy sch, float dc, bool wc){
    log_start("Runtime initialization...");

    // Init min_execution_time and min_transmission_time
    this->min_execution_time = 0.01;
    this->min_transmission_time = 0.01;

    std::cout<<" Scheduler: "<<sch<<std::endl;
    std::cout<<" DC ratio: "<<dc<<std::endl;
    Scheduler = sch;
    RRCounter = -1; // Always set it -1 at the beginning of execution?
    DCRatio = dc;
    this->with_conflicts = wc;

    if (Scheduler == PEFT){
      DECLARE_TIMING(oct);
      log_start("OCT calculation...");
      START_TIMING(oct);
      CalcOCT(); //OCT for PEFT
      STOP_TIMING(oct);
      this->oct_time = GET_TIMING(oct);
      std::cout<<" Execution time of CalcOCT():"<<GET_TIMING(oct)<<" s"<<std::endl;
      log_end("OCT calculation.");

      DECLARE_TIMING(rankoct);
      log_start("Rank OCT calculation...");
      START_TIMING(rankoct);
      CalcRankOCT(); //RankOCT for PEFT
      STOP_TIMING(rankoct);
      this->rankoct_time = GET_TIMING(rankoct);
      std::cout<<" Execution time of CalcRankOCT():"<<GET_TIMING(rankoct)<<" s"<<std::endl;
      log_end("Rank OCT calculation.");
    }

    if (Scheduler == HSIP || Scheduler == HEFT || Scheduler == CPOP){
      log_start("OCCW initialization...");
      global_graph.InitAllOCCW(); //OCCW for HSIP
      log_end("OCCW initialization.");

      DECLARE_TIMING(ranku);
      log_start("Rank_u calculation...");
      START_TIMING(ranku);
      CalcRank_u(); // Rank_u for HSIP and HEFT
      STOP_TIMING(ranku);
      this->rank_u_time = GET_TIMING(ranku);
      std::cout<<" Execution time of CalcRank_u():"<<GET_TIMING(ranku)<<" s"<<std::endl;
      log_end("Rank_u calculation.");

      DECLARE_TIMING(rankd);
      log_start("Rank_d calculation...");
      START_TIMING(rankd);
      CalcRank_d(); // Rank_d for CPOP
      STOP_TIMING(rankd);
      this->rank_d_time = GET_TIMING(rankd);
      std::cout<<" Execution time of CalcRank_d():"<<GET_TIMING(rankd)<<" s"<<std::endl;
      log_end("Rank_d calculation.");

      // Calculate priority used in CPOP policy.
      this->absCP = global_graph.CalcPriorityCPOP();
    }

    if (Scheduler == DONF || Scheduler == DONF2 || Scheduler == ADON || Scheduler == DONFM || Scheduler == DONFL || Scheduler == DONFL2 || Scheduler == ADONL){
      DECLARE_TIMING(NDON);
      log_start("NDON calculation...");
      START_TIMING(NDON);
      CalcNDON();
      if(Scheduler == ADON || Scheduler == ADONL){
	CalcADON();
      }
      STOP_TIMING(NDON);
      this->ndon_time = GET_TIMING(NDON);
      std::cout<<" Execution time of CalcNDON():"<<GET_TIMING(NDON)<<" s"<<std::endl;

      log_end("NDON calculation.");

    }

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
	global_graph.GetNode(*iter)->SetStatus(READY); // Set node's status

	// Calculate Cpath computatin cost
	global_graph.CalcCpathCC(*iter, this->max_devCompute, this->min_execution_time);
	global_graph.CalcCpathCCMem(*iter, this->max_devCompute, TaihuLight[this->max_computeDevId]->GetBw(), this->min_execution_time);

	//Record the current time
	global_graph.GetNode(*iter)->SetWaitTime(this->global_timer);
      }
    }

    log_end("Runtime initialization.");
  }

  /** Set the total_step value.
   */
  void Runtime::SetStep(int step){
    assert(step >=1);
    this->total_step = step;
#ifdef DEBUG
    std::cout<<"Set total_step: "<<step<<std::endl;
    std::cout<<"Check total_step: "<<this->total_step<<std::endl;
#endif
  }

  /** Set issue_width value.
   */
  void Runtime::SetIssueWidth(int width){
    assert(width >= 1);
    this->issue_width = width;
#ifdef DEBUG
    std::cout<<"Set issue_width: "<<width<<std::endl;
    std::cout<<"Check issue_width: "<<this->issue_width<<std::endl;
#endif
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

    /** 1. Find the "sink" vertex.
     */
    int sinkId = global_graph.GetSinkId();


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
		float meanct = CommunicationDataSize(crtVertId, vertId) / TaihuLightNetwork.GetMeanBW();
		// The max op is for avoiding calculation time ignorance
		current = OCT[vertex->GetId()][dev.first] + std::max(((float)vertex->GetCompDmd()) / (dev.second)->GetCompPower(), (float)1.0) + ((dev.first == devId)?0:meanct);
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
  // TODO: avoid overflow for big value
  void Runtime::CalcRankOCT(){
    for (int ndId : idset){
      Node* nd = global_graph.GetNode(ndId);
      float rowOCT = 0;
      for (int i = 0; i < this->deviceNum; i++) {
	//rowOCT += OCT[ndId][i];
	rowOCT += (float(OCT[ndId][i]) - rowOCT) / (i+1);
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

    /** 1. Find the "sink" vertex.
     */
    int sinkId = global_graph.GetSinkId();

    /** 2. Traversing the DAG from the exit to the entry vertex
	and calculate the rank_u. Deal the cross-level edges carefully.
    */

    std::set<int> recent; // Store the vertex ids that calculated recently
    float rank_HSIP, rank_HEFT;
    // 2.1 Calculate sink node's rank_u
    Node* nd = global_graph.GetNode(sinkId);
    rank_HSIP = CalcWeightMeanSD(sinkId);
    nd->SetRank_u_HSIP(rank_HSIP);
    nd->SetRank_u_HEFT(nd->GetMeanWeight());
    recent.insert(sinkId);

    // 2.2 Calculate all the others
    while ( !recent.empty() ){
      auto it = *(recent.begin());// The first value stored
      recent.erase(recent.begin());
      Node* nd = global_graph.GetNode(it);
      for (auto& crtVertId : nd->input){

	Node* crtNd = global_graph.GetNode(crtVertId);
	// 2.2.0 If it has already been calculated, continue
	if (crtNd->GetRank_u_HSIP() >= 0){
	  continue;
	}

	// 2.2.1 If not all of the output nodes' rank_u have been calculated, continue!
	bool allSatisfied = true;
	for(auto& succ : crtNd->output){
	  Node* succNd = global_graph.GetNode(succ);
	  if (succNd->GetRank_u_HSIP() < 0){
	    allSatisfied = false;
	  }
	}
	if (!allSatisfied){
	  continue;
	}

	// 2.2.2 Calculate rank_u for current vertex
	float max_ranku_HSIP = 0;
	float max_ranku_HEFT = 0;
	float tmp_ranku;
	for(auto& succ : crtNd->output){
	  Node* succNd = global_graph.GetNode(succ);
	  if (max_ranku_HSIP < (succNd->GetRank_u_HSIP()) ){
	    max_ranku_HSIP = succNd->GetRank_u_HSIP();
	  }

	  tmp_ranku = CommunicationDataSize(crtVertId, succ) / TaihuLightNetwork.GetMeanBW() + succNd->GetRank_u_HEFT();
	  if (max_ranku_HEFT < tmp_ranku){
	    max_ranku_HEFT = tmp_ranku;
	  }
	}
	rank_HSIP = CalcWeightMeanSD(crtVertId) + crtNd->GetOCCW() + max_ranku_HSIP;
	rank_HEFT = crtNd->GetMeanWeight() + max_ranku_HEFT;
	crtNd->SetRank_u_HSIP(rank_HSIP);
	crtNd->SetRank_u_HEFT(rank_HEFT);

	// 2.2.3 Add crtVertId into recent after calculation
	recent.insert(crtVertId);
      }
    }
  }

  /** Calculate rank_d, which is used in CPOP policy.
   */
  void Runtime::CalcRank_d(){

    /** 1. Find the "source" vertex.
     */
    int sourceId = global_graph.GetSourceId();

    /** 2. Traversing the DAG from the source to the exit vertex
	and calculate the rank_d.
    */
    std::set<int> recent; // Store the vertex ids that calculated recently
    // 2.1 Calculate source node's rank_d
    Node* nd = global_graph.GetNode(sourceId);
    nd->SetRank_d_CPOP(0);
    recent.insert(sourceId);

    // 2.2 Calculate all the others
    while ( !recent.empty() ){
      auto it = *(recent.begin());// The first value stored
      recent.erase(recent.begin());
      Node* nd = global_graph.GetNode(it);
      for (auto& crtVertId : nd->output){
	Node* crtNd = global_graph.GetNode(crtVertId);
	// 2.2.0 If it has already been calculated, continue
	if (crtNd->GetRank_d_CPOP() >= 0){
	  continue;
	}

	// 2.2.1 If not all of the input nodes' rank_d have been calculated, continue!
	bool allSatisfied = true;
	for(auto& pred : crtNd->input){
	  Node* predNd = global_graph.GetNode(pred);
	  if (predNd->GetRank_d_CPOP() < 0){
	    allSatisfied = false;
	  }
	}
	if (!allSatisfied){
	  continue;
	}

	// 2.2.2 Calculate rank_d for current vertex
	float max_rankd_CPOP = 0;
	float tmp_rankd;
	for(auto& pred : crtNd->input){
	  Node* predNd = global_graph.GetNode(pred);
	  tmp_rankd = CommunicationDataSize(pred, crtVertId) / TaihuLightNetwork.GetMeanBW() + predNd->GetMeanWeight() + predNd->GetRank_d_CPOP();
	  max_rankd_CPOP = std::max(max_rankd_CPOP, tmp_rankd);
	}

	crtNd->SetRank_d_CPOP(max_rankd_CPOP);

	// 2.2.3 Add crtVertId into recent after calculation
	recent.insert(crtVertId);
      }
    }
  }


  /** Calculate normalized degree of node.
   */
  float Runtime::NDON(Node * nd, int degree){
    // Only support degree = 1 or 2 at present.
    assert(degree >= 1 && degree <= 2);
    float normdegree = nd->GetNDON();
    if(degree == 2){
      for (auto it : nd->output) {
	normdegree += this->alpha_DON * global_graph.GetNode(it)->GetNDON();
      }
    }

#ifdef DEBUG
    std::cout<<"Norm degree of node "<<nd->GetId()<<": "<<normdegree<<std::endl;
#endif
    return normdegree;
  }

  /** Calculate NDON for all nodes in initruntime().
   */
  void Runtime::CalcNDON(){
    for (auto it : idset) {
      float normdegree = 0;
      Node* crtNd = global_graph.GetNode(it);
      for (auto succit : crtNd->output) {
	normdegree += 1.0 / global_graph.GetNode(succit)->GetInNum();
      }
      crtNd->SetNDON(normdegree);
    }
  }

  /** Calculate ADON for all nodes in initruntime().
   */
  void Runtime::CalcADON(){
    /** 1. Find the "sink" vertex.
     */
    int sinkId = global_graph.GetSinkId();

    /** 2. Traversing the DAG from the exit to the entry vertex
	and calculate the rank_ADON.
    */
    std::set<int> recent; // Store the vertex ids that calculated recently
    // 2.1 Calculate sink node's rank_ADON
    Node* nd = global_graph.GetNode(sinkId);
    nd->SetRank_ADON(0);
    recent.insert(sinkId);

    // 2.2 Calculate all the others
    while ( !recent.empty() ){
      auto it = *(recent.begin());// The first value stored
      recent.erase(recent.begin());
      Node* nd = global_graph.GetNode(it);
      for (auto& crtVertId : nd->input){

	Node* crtNd = global_graph.GetNode(crtVertId);
	// 2.2.0 If it has already been calculated, continue
	if (crtNd->GetRank_ADON() >= 0){
	  continue;
	}

	// 2.2.1 If not all of the output nodes' rank_ADON have been calculated, continue!
	bool allSatisfied = true;
	for(auto& succ : crtNd->output){
	  Node* succNd = global_graph.GetNode(succ);
	  if (succNd->GetRank_ADON() < 0){
	    allSatisfied = false;
	  }
	}
	if (!allSatisfied){
	  continue;
	}

	// 2.2.2 Calculate rank_ADON for current vertex
	float tmp_rank_ADON = crtNd->GetNDON();
	for(auto& succ : crtNd->output){
	  Node* succNd = global_graph.GetNode(succ);
	  tmp_rank_ADON += this->alpha_DON * succNd->GetRank_ADON();
	}
	crtNd->SetRank_ADON(tmp_rank_ADON);

	// 2.2.3 Add crtVertId into recent after calculation
	recent.insert(crtVertId);
      }
    }
  }

  /** Output NodeStatus as enum items.
   */

  std::ostream& operator<<(std::ostream& out, const NodeStatus values){
    static std::map<NodeStatus, std::string> Nodestrings;
    if (Nodestrings.size() == 0){
#define INSERT_ELEMENT(p) Nodestrings[p] = #p
      INSERT_ELEMENT(INIT);
      INSERT_ELEMENT(READY);
      INSERT_ELEMENT(RUNNING);
      INSERT_ELEMENT(FINISHED);
#undef INSERT_ELEMENT
    }

    return out << Nodestrings[values];
    }

  /** The whole execution logic.
   */
  // TODO: Count the schduling time itself
  void Runtime::Execute(){
    log_start("Execution...");

    DECLARE_TIMING(taskpick);
    DECLARE_TIMING(devicepick);

    // Execute until all three queues/lists are empty
    while (!ready_queue.empty() || !execution_queue.empty() || !block_free_queue.empty()) {
      /** 0. if a memory block's refer number can be decreased
	  decrease it and check if we need to free the block.
      */
      for (int i=block_free_queue.size()-1; i>=0; i--){
	auto& it = block_free_queue[i];
	if (it.second <= (global_timer + ZERO_POSITIVE)){
	  MemoryBlock * blk_pointer = BlocksMap[it.first];
	  if ( (blk_pointer->DecRefers()) <= 0 ){ // do the free
	    //blk_pointer->DoFree(TaihuLight);

	    //Check if need to set mem_full_dev
	    Device* dv = TaihuLight[blk_pointer->DeviceLocation()];
	    if((dv->GetFreeRAM() / dv->GetRAM() > this->mem_full_threshold) && dv->IsFull()){
	      dv->SetFull(false);
	      this->mem_full_dev--;
	    }

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

	  Node* nd = global_graph.GetNode(it->first);

	  // Check if the task can be committed
	  bool commitable = true;
	  std::set<int>::iterator ndit;
	  for (ndit = nd->output.begin(); ndit != nd->output.end(); ndit ++){
	    Node* tmpNd = global_graph.GetNode(*ndit);
	    if(tmpNd->GetStep() > nd->GetStep()){
	      // An error detected
	      commitable = false;
	      log_error("Succ task's step is larger than its pred task.");
	      exit(-1);
	    }else if(tmpNd->GetStep() < nd->GetStep() &&
		     (tmpNd->GetStatus() == INIT ||
		      tmpNd->GetStatus() == READY ||
		      tmpNd->GetStatus() == RUNNING ||
		      tmpNd->GetStatus() == FINISHED)){
	      commitable = false;
	    }
	  }
	  if(!commitable){
	    //The task cannot be removed from execution queue
	    //Update execution queue item
#ifdef DEBUG
	    std::cout<<"Update "<<it->first<<"'s second from "<<it->second;
#endif
	    it->second = CalcNearestFinishTime(it->second + ZERO_POSITIVE);
#ifdef DEBUG
	    std::cout<<" to "<<it->second<<std::endl;
#endif
	    it++;
	    continue;
	  }

	  // Record cpath computation cost summary
	  this->max_cpath_cc = std::max(this->max_cpath_cc, nd->GetCpathCC());
	  this->max_cpath_cc_mem = std::max(this->max_cpath_cc_mem, nd->CpathCCMem());

	  // Set free the corresponding device
	  int devId = nd->GetOccupied();

	  // Set the finished_tasks properly
	  TaihuLight[devId]->IncreaseTasks(1);

	  // Decrease load of the corresponding device
	  TaihuLight[devId]->DecreaseLoad(1);

	  if(TaihuLight[devId]->GetAvaTime() <= global_timer && TaihuLight[devId]->IsBusy()){
	    // Only set free the ones that are really free.
	    TaihuLight[devId]->SetFree();
	    deviceInUse --;
	  }

	  //fill running_history map
	  // TODO: do we really need this? I think it can be removed
	  running_history[nd->GetId()] = devId;

	  // update pending list and ready queue and success nodes' level
	  for (ndit = nd->output.begin(); ndit != nd->output.end(); ndit ++){
	    if(global_graph.GetNode(*ndit)->GetStep() < nd->GetStep()){
	      // In a pipeline, only one step difference is permitted
	      assert(nd->GetStep() ==
		     global_graph.GetNode(*ndit)->GetStep() + 1);
	      // Reset ndit's pending number then update its step
	      pending_list[*ndit] = global_graph.GetNode(*ndit)->GetInNum();
	      global_graph.GetNode(*ndit)->SetStep(nd->GetStep());
	    }
	    int pendingNum = pending_list[*ndit];
	    pendingNum --;
#ifdef DEBUG
	    std::cout<<"Node: "<<*ndit<<", pending num:"<<pendingNum<<std::endl;
#endif
	    assert(pendingNum >= 0);
	    pending_list[*ndit] = pendingNum;

	    if (pendingNum == 0){
	      ready_queue.push_back(*ndit);
	      global_graph.GetNode(*ndit)->SetStatus(READY); // Set node's status

	      // Calculate Cpath computatin cost
	      global_graph.CalcCpathCC(*ndit, this->max_devCompute, this->min_execution_time);
	      global_graph.CalcCpathCCMem(*ndit, this->max_devCompute, TaihuLight[this->max_computeDevId]->GetBw(), this->min_execution_time);

	      //Record the current time
	      global_graph.GetNode(*ndit)->SetWaitTime(this->global_timer);
	    }
	    if(nd->GetStep() == 0){
	      global_graph.GetNode(*ndit)->SetLevel(nd->GetLevel() + 1);
	    }
	  }

	  // erase the task from execution_queue
	  execution_queue.erase(it++);
	  nd->SetStatus(FINISHED);

	  // Check and set the status of the pred tasks
	  for (auto& it : nd->input){
	    Node* input_nd = global_graph.GetNode(it);
	    if(input_nd->GetStep() == nd->GetStep() &&
	       input_nd->GetStatus() == FINISHED){
	      int num_finished_succ = 0;
	      for (auto& innerit : input_nd->output){
		if(global_graph.GetNode(innerit)->GetStatus() == FINISHED ||
		   global_graph.GetNode(innerit)->GetStatus() == REUSEABLE){
		  num_finished_succ ++;
		}
	      }
	      assert(num_finished_succ <= input_nd->GetOutNum());
	      if(num_finished_succ == input_nd->GetOutNum()){
		// All the succ tasks have finished
		input_nd->SetStatus(REUSEABLE);
#if 0
		std::cout<<"Reuse node "<<input_nd->GetId()<<std::endl;
#endif

	      }
	    }
	  }

	  //check for sink node
	  if(nd->GetId() == global_graph.GetSinkId()){
	    nd->SetStatus(REUSEABLE);
	    this->step_done ++;
#if 1
	    std::cout<<"Sink node "<<nd->GetId()<<" finished, step_done: "<<this->step_done<<std::endl;
#endif
	  }

#if 0
	  std::cout<<"Node "<<it->first<<" finished execution at "<<global_timer<< ". It used device "<<devId<<", status:"<<nd->GetStatus()<<std::endl;
#endif


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

      /** 2. If the ready queue is not empty and the scheduler is available,
	  process a new task from it and update global_timer, deviceInUse
       */
      while ( (!ready_queue.empty()) && global_timer >= (scheduler_ava_time + ZERO_NEGATIVE) ) {

	/** For debug: check the task status in ready_queue and execution_queue
	 */
	for(auto& ite: ready_queue){
	  assert(global_graph.GetNode(ite)->IsReady());
	}

	// TODO: 1. consider schedule time here;

	// 2.0 Update max_parallel value
	max_parallel = std::max(max_parallel, (int)(ready_queue.size() + execution_queue.size()));

	//2.1 pick a task from ready_queue (default: choose the first one element)
	START_TIMING(taskpick);
	int task_node_id = TaskPick();
	STOP_TIMING(taskpick);

	/** Task counter: total tasks and tasks that DONF hits
	 */
	task_total++;
	int NODF_task_id = TaskPick(DONF);
	if(task_node_id == NODF_task_id){
	  task_hit_counter++;
	}

	// Erase the task id from the ready_queue
	std::vector<int>::iterator iter = ready_queue.begin();
	for (; iter != ready_queue.end(); iter++){
	  if (*iter == task_node_id){
	    ready_queue.erase(iter);
	    break;
	  }
	}

	Node* nd = global_graph.GetNode(task_node_id);

	//2.2 choose a free device to execute the task (default: choose the first free device)
	/** If this is the only entry task and Scheduler is HSIP,
	    then use entry task duplication policy.
	 */
	if ( ready_queue.size() == 1 && Scheduler == HSIP && nd->GetInNum() == 0){
	  //Entry task duplication
	  EntryTaskDuplication(nd);
	  nd->SetStatus(RUNNING);
	  continue;
	}
	// Entry task duplication policy doesn't need this.
	START_TIMING(devicepick);
	Device* dev = DevicePick(task_node_id);
	STOP_TIMING(devicepick);

	/** Test if DATACENTRIC could hit this pick.
	 */
	Device* test_dev = DevicePick(task_node_id, DATACENTRIC);
	if(dev != NULL && test_dev != NULL &&  dev->GetId() == test_dev->GetId()){
	  dev_hit_counter++;
	}
	/** If dev == NULL, re-insert the node into ready_queue,
	    and then calculate nearest finish time.

	    Dead loop detection:
	    When no device's free RAM can cover any node's demand
	    in ready_queue and execution_queue is empty, then a
	    dead loop is detected.
	    For a dead loop, report it and stop the simulation.
	 */
	//assert(dev != NULL);
	if ( dev == NULL ){
	  if ( DeadLoopDetect() ){
	    this->task_pick_time = GET_TOTAL_TIMING(taskpick);
	    this->device_pick_time = GET_TOTAL_TIMING(devicepick);

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
	nd->SetStatus(RUNNING);

	dev->IncreaseLoad(1);

	/** Note: since the scheduled tasks are ready tasks,
	    global_timer + transmission_time is the EST of this task.
	 */
	float transmission_time = CalcTransmissionTime(*nd, *dev, true, true);
	float execution_time = CalcExecutionTime(*nd, *dev);


	// TODO: change all the time from second to microsecond
	// To avoid error, execution time should not be less than 1ms.
	execution_time = std::max(execution_time, this->min_execution_time);

	// This should be done in CalcTransmissionTime()
	//transmission_time = std::max(transmission_time, this->min_transmission_time);

	//Manage memory blocks data structures
	int block_id = (nd->GetStep() * global_graph.Nodes()) + nd->GetId();
	MemoryBlock* block = new MemoryBlock(block_id, dev->GetId(), std::max(CalcMemDmd(nd, dev),(float)0.0), nd->GetOutNum());
	nd->SetMemAlloc(std::max(CalcMemDmd(nd, dev), 0.0f));
	block->DoAlloc(TaihuLight);
	// TODO: check if the block id already exists, which is illegal
	if(BlocksMap.find(block_id) != BlocksMap.end()){
	  // The memory block already exists.
	  log_error("Block id "<<block_id<<" already exists");
	  exit(-1);
	}
	BlocksMap[block_id] = block;

	//Check if need to set mem_full_dev
	if((dev->GetFreeRAM() / dev->GetRAM() <= this->mem_full_threshold) && !dev->IsFull()){
	  dev->SetFull(true);
	  this->mem_full_dev++;
	}

	//Calc min_free_mem
	float sum_free_RAM = 0;
	for (auto& it: TaihuLight){
	  sum_free_RAM += (it.second)->GetFreeRAM();
	}
	if(this->min_free_mem < 0 || this->min_free_mem > sum_free_RAM){
	  this->min_free_mem = sum_free_RAM;
	}

	for (std::set<int>::iterator iter = nd->input.begin(); iter != nd->input.end(); iter ++){
	  Node* input_nd = global_graph.GetNode(*iter);
	  block_free_queue.push_back(std::pair<int, float>((nd->GetStep() * global_graph.Nodes())+input_nd->GetId(), (transmission_time + global_timer)));
	}

	// Process the execution time
	float AST = -1; // Actual Start Time
	if ( (AST = dev->FindSlot(global_timer+transmission_time, execution_time)) >= 0 ){// Insertion into ITS.
	  // Update ITS
	  dev->UpdateSlot(AST, execution_time, global_timer);
	}else{// Not insertion, normal execution.
	  AST = std::max(dev->GetAvaTime(), global_timer + transmission_time);

	  assert(AST >= ZERO_NEGATIVE);

	  //Record the task waiting time
	  nd->SetWaitTime(AST - nd->GetWaitTime());

	  //Record the available time of the corresponding device
	  dev->SetAvaTime(AST + execution_time);

	  if( AST > dev->GetAvaTime()){
	    // Set the ITS of the scheduled device
	    dev->NewSlot(dev->GetAvaTime(), global_timer + transmission_time);
	  }
	}

	// Fill the execution_queue
	execution_queue.emplace(task_node_id, (AST + execution_time));

	// Set schduler_ava_time
	scheduler_ava_time = global_timer + scheduler_mean_cost;

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
#endif
#if 1
	//Debug
	std::cout<<"(Step "<<nd->GetStep()<<")"<<" node "<<task_node_id<<" on "<<dev->GetId();
	std::cout<<" at "<<global_timer<<", trans "<<transmission_time<<", exe "<<execution_time<<", finish "<<nd->GetAFT()<<", status: "<<nd->GetStatus()<<", wait time:"<<nd->GetWaitTime()<<", free RAM:"<<dev->GetFreeRAM()<<std::endl;
#endif
#if 0
	std::cout<<"Device ava time updated to: "<<dev->GetAvaTime()<<"s, Node AFT updated to: "<<nd->GetAFT()<<std::endl;
#endif
      }

      /** 3. Try to add a new iteration.
       */
      if((this->max_step < this->total_step - 1) &&
	 (this->max_step - this->step_done < this->issue_width)){
	//Check if the source node can be added into ready queue
	Node* sourceNd = global_graph.GetNode(global_graph.GetSourceId());
	if(sourceNd->GetStatus() == REUSEABLE){
	  // Add (the only) source node into ready queue
	  ready_queue.push_back(sourceNd->GetId());
	  sourceNd->SetStatus(READY); // Set node's status

	  sourceNd->SetWaitTime(this->global_timer);
	  sourceNd->SetStep(sourceNd->GetStep() + 1);
	  this->max_step = sourceNd->GetStep();
#if 1
	  std::cout<<"The source node added into ready queue, max_step:"<<this->max_step<<std::endl;
#endif
	}
      }

      /** 4. Update global_timer to the nearest finish time
       */
      global_timer = CalcNearestFinishTime();

      /** For debug: check the task status in ready_queue and execution_queue
       */
      for(auto& ite: ready_queue){
	assert(global_graph.GetNode(ite)->IsReady());
      }

      for(auto& ite: execution_queue){
	assert(global_graph.GetNode(ite.first)->GetStatus() == RUNNING);
      }

    }

    this->task_pick_time = GET_TOTAL_TIMING(taskpick);
    this->device_pick_time = GET_TOTAL_TIMING(devicepick);

    // Finish running
    // Write simulation report.
    SimulationReport();

    log_end("Execution.");
  }

  /** Pick a task from ready queue according to the
      scheduling policy.
      Erase the correspinding task index from ready_queue.
   */
  int Runtime::TaskPick(SchedulePolicy sch){
#ifdef DEBUG
    std::cout<<"TaskPick: Scheduler "<<Scheduler<<std::endl;
#endif

    SchedulePolicy InnerScheduler;
    if (sch == UNKNOWN){
      InnerScheduler = this->Scheduler;
    }else{
      InnerScheduler = sch;
    }

    int taskIdx = -1;
    switch(InnerScheduler){
    case UNKNOWN:
      break;

    case FCFS:
    case RR:
      taskIdx = ready_queue.front();
      break;

    case SJF:{ // Pick the shortest job
      int tmpComDmd, leastComDmd = -1;
      std::vector<int>::iterator iter = ready_queue.begin();
      for (; iter != ready_queue.end(); iter++){
	if (leastComDmd < 0){ // the first node in ready_queue
	  Node * nd = global_graph.GetNode(*iter);
	  leastComDmd = nd->GetCompDmd();
	  taskIdx = *iter;
	}else{
	  Node * nd = global_graph.GetNode(*iter);
	  tmpComDmd = nd->GetCompDmd();
	  if (leastComDmd > tmpComDmd){
	    leastComDmd = tmpComDmd;
	    taskIdx = *iter;
	  }
	}
      }
    }
      break;

    case PRIORITY:
      break;

    case HEFT:
    case CPOP:
    case HSIP:
    case PEFT:{
      /** Traverse all the tasks in the ready queue,
	  pick the one with the max priority.
       */
      float maxPriority = -1;
      std::vector<int>::iterator iter = ready_queue.begin();
      for (; iter != ready_queue.end(); iter++){
	Node* nd = global_graph.GetNode(*iter);
	if(Scheduler == PEFT){ //PEFT
	  if (maxPriority < nd->GetRankOCT()){
	    maxPriority = nd->GetRankOCT();
	    taskIdx = *iter;
	  }
	}else if(Scheduler == HSIP){ //HSIP
	  if (maxPriority < nd->GetRank_u_HSIP()){
	    maxPriority = nd->GetRank_u_HSIP();
	    taskIdx = *iter;
	  }
	}else if(Scheduler == HEFT){ //HEFT
	  if (maxPriority < nd->GetRank_u_HEFT()){
	    maxPriority = nd->GetRank_u_HEFT();
	    taskIdx = *iter;
	  }
	}else{ //CPOP
	  if (maxPriority < nd->GetPriorityCPOP()){
	    maxPriority = nd->GetPriorityCPOP();
	    taskIdx = *iter;
	  }
	}
      }
    }
      break;

    case DATACENTRIC:
    case DONF:
    case DONF2:
    case ADON:
    case DONFM:
    case DONFL:
    case DONFL2:
    case ADONL:{
      if(InnerScheduler == DONFM){//this->mem_full_dev / this->deviceNum >= this->dev_full_threshold){
	//RAM full pick
	/*
	int min_level = -1;
	std::vector<int>::iterator iter = ready_queue.begin();
	for (; iter != ready_queue.end(); iter++){
	  Node* nd = global_graph.GetNode(*iter);
	  if(min_level < 0 || min_level > nd->GetLevel()){
	    min_level = nd->GetLevel();
	    taskIdx = *iter;
	  }
	  }*/
	//std::cout<<"RAM full pick."<<std::endl;
	//Pick a mem block (also a node), on a full device and min dependencies
	int min_deps = -1;
	int mem_block_id = -1;
	for (auto& it:BlocksMap) {
	  Device* dv = TaihuLight[(it.second)->DeviceLocation()];
	  if(dv->IsFull() && (min_deps < 0 || min_deps > (it.second)->GetRefers())){
	    int readynum = 0;
	    Node* nd_pointer = global_graph.GetNode(it.first);
	    for(auto& innerit : nd_pointer->output){
	      if(global_graph.GetNode(innerit)->IsReady()){
		readynum ++;
	      }
	    }
	    if(readynum > 0){
	      min_deps = (it.second)->GetRefers();
	      mem_block_id = it.first;
	    }
	  }
	}
	//assert(mem_block_id >= 0);
	if(mem_block_id >= 0){
	  //traverse the ready queue, pick a task which is the seccess nodes of the picked node
	  Node* nd_pointer = global_graph.GetNode(mem_block_id);
	  std::vector<int>::iterator iter = ready_queue.begin();
	  for (; iter != ready_queue.end(); iter++){
	    if(nd_pointer->output.find(*iter) != nd_pointer->output.end()){//it's a success node
	      taskIdx = *iter;
	      break;
	    }
	  }
	}
	if(taskIdx < 0){//did not find a good task
	  std::cout<<"RAM full pick: did not find a good task!"<<std::endl;
	  taskIdx = ready_queue.front();
	}
      }else{
	float degree, maxOutDegree = -1;
	std::vector<int>::iterator iter = ready_queue.begin();
	for (; iter != ready_queue.end(); iter++){
	  Node* nd = global_graph.GetNode(*iter);
	  if( Scheduler == ADON || Scheduler == ADONL){ // ADON, ADONL
	    degree = nd->GetRank_ADON();
	  }else if( Scheduler == DONF2 || Scheduler == DONFL2 ){ // DONF2, DONFL2
	    degree = NDON(nd, 2);
	  }else{ // DC, DONF, DONFL
	    degree = NDON(nd);
	  }
	  if (maxOutDegree < degree){
	    maxOutDegree = degree;
	    taskIdx = *iter;
	  }
	}
      }
    }
      break;

    case MULTILEVEL:
      break;

      //case DATACENTRIC:
      /** If device in use is small, fetch a task from the ready queue tail.
	  Else, fetch a task from the head.
	  We need a flag var. For example, ...
       */
      //break;
    default:
      std::cout<<"Error: unrecognized scheduling policy "<<Scheduler<<std::endl;
      exit(1);
    }

    return taskIdx;
  }

  /** Calculate the memory demand for node nd on device dev.
   */
  float Runtime::CalcMemDmd(Node * nd, Device * dev){
    float dmd = 0;
    dmd = std::max(nd->GetDataDmd(), nd->GetDataGenerate());
    for (auto& pred : nd->input){
      Node* predNd = global_graph.GetNode(pred);
      if(predNd->GetOccupied() == dev->GetId()){
	dmd -= predNd->GetDataGenerate();
      }
    }
    dmd = std::max(dmd, 0.5f);
    return dmd;
  }

  /** Pick a free device according to the task requirements and
      scheduling policy.
   */
  Device* Runtime::DevicePick(int ndId, SchedulePolicy sch){
    SchedulePolicy InnerScheduler;

#ifdef DEBUG
    std::cout<<"DevicePick: Scheduler "<<Scheduler<<", node "<<ndId<<std::endl;
#endif

    if (sch == UNKNOWN){
      InnerScheduler = this->Scheduler;
    }else{
      InnerScheduler = sch;
    }

    //int CTM[][3] = {22, 21, 36, 22, 18, 18, 32, 27, 43, 7, 10, 4, 29, 27, 35, 26, 17, 24, 14, 25, 30, 29, 23, 36, 15, 21, 8, 13, 16, 33};

    Node * nd = global_graph.GetNode(ndId);
    Device * dev = NULL;

    if(nd->Loc() != -1){
      std::cout<<"Nd has loc value:"<<nd->GetId()<<" with "<<nd->Loc()<<std::endl;
      assert(running_history.find(nd->Loc()) != running_history.end());
      dev = TaihuLight[running_history[nd->Loc()]];
      return dev;
    }

    switch(InnerScheduler){
    case UNKNOWN:
      break;

    /** Actually this is a very basic logic, try to use the EFT way.
     */
    case PRIORITY:{
      float exeTime = -1.0;
      for (auto& it: TaihuLight){
	if(nd->GetRatio((it.second)->GetType()) < ZERO_NEGATIVE){
	  //The task is not executable on the device
	  continue;
	}

	//TODO: speed ratio?
	if(CalcMemDmd(nd, it.second) <= (it.second)->GetFreeRAM() + ZERO_POSITIVE){
	  //TODO: It is not suitable to call CalcExecutionTime() directly
	  float tmpExeTime = CalcExecutionTime(*nd, *(it.second));
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
    }
      break;
    case RR:
      for (int i = RRCounter+1; i<TaihuLight.size(); i++){
	Device * tmpDev = TaihuLight[i];
	if(nd->GetRatio(tmpDev->GetType()) < ZERO_NEGATIVE){
	  //The task is not executable on the device
	  continue;
	}
	if ( CalcMemDmd(nd, tmpDev) <= tmpDev->GetFreeRAM() + ZERO_POSITIVE){
	  dev = tmpDev;
	  RRCounter = i;
	  break;
	}
      }
      if (dev == NULL){// Haven't found a good device
	for (int i = 0; i<=RRCounter; i++){
	  Device * tmpDev = TaihuLight[i];
	  if(nd->GetRatio(tmpDev->GetType()) < ZERO_NEGATIVE){
	    //The task is not executable on the device
	    continue;
	  }
	  if ( CalcMemDmd(nd, tmpDev) <= tmpDev->GetFreeRAM() + ZERO_POSITIVE){
	    dev = tmpDev;
	    RRCounter = i;
	    break;
	  }
	}
      }
      break;

    case FCFS:
    case SJF:
    case HEFT:
    case CPOP:
    case HSIP:
    case PEFT:
    case DONFL:
    case DONFL2:
    case ADONL:{
      /** For tasks on critical path, assign them to the fastest device.
       */
      float dv = nd->GetPriorityCPOP() - this->absCP;
      if(InnerScheduler == CPOP && dv >= ZERO_NEGATIVE && dv <= ZERO_POSITIVE){
	dev = TaihuLight[this->max_computeDevId];
	if(CalcMemDmd(nd, dev) > dev->GetFreeRAM() + ZERO_POSITIVE || nd->GetRatio(dev->GetType()) < ZERO_NEGATIVE){
	  for (auto& it: TaihuLight){
	    if(nd->GetRatio((it.second)->GetType()) < ZERO_NEGATIVE){
	      //The task is not executable on the device
	      continue;
	    }

	    if(CalcMemDmd(nd, it.second) < (it.second)->GetFreeRAM() + ZERO_NEGATIVE){
	      dev = it.second;
	      break;
	    }
	  }
	}

#ifdef DEBUG
	std::cout<<"[Info] Node "<<ndId<<" is a CP task, so pick device "<<this->max_computeDevId<<std::endl;
#endif
	break;
      }

      /** Traverse all the devices,
	  pick the one with the min EFT.
       */
      float mean_load = CalcMeanLoad();
      float min_OEFT = -1;
      // 1.Traverse all the devices
      for (auto& it: TaihuLight){
	if( CalcMemDmd(nd, it.second) > (it.second)->GetFreeRAM() + ZERO_POSITIVE ){
	  // The free memory of the device is too little, skip
	  continue;
	}
#ifdef DEBUG
	std::cout<<"lb_balance: "<<load_balance_threshold<<", mean load: "<<mean_load<<", dev load:"<<(it.second)->GetLoad()<<std::endl;
#endif // DEBUG
	if(this->load_balance_threshold && (it.second)->GetLoad() > std::max(mean_load, float(this->load_balance_threshold))){
#ifdef DEBUG
	  std::cout<<"a load balance pick"<<std::endl;
#endif // DEBUG
	  continue;
	}

	/*
	if(this->load_time >= ZERO_POSITIVE && ((it.second)->GetAvaTime() - global_timer) >= this->load_time){
	  continue;
	  }*/

	if(nd->GetRatio((it.second)->GetType()) < ZERO_NEGATIVE){
	  //The task is not executable on the device
	  continue;
	}

	float w = nd->GetCompDmd() / (it.second)->GetCompPower() / nd->GetRatio((it.second)->GetType());

	// 2. Calculate EST by traversing all the pred(ndId)
	float EST = 0;
	float tmpEST;
	/* The logic below is different from that in CalcTransmissionTime()
	   since CalcTransmissionTime() considers the communication between
	   devices instead of tasks.
	 */
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
	    float ava_time;//The link available time
	    ct = TaihuLightNetwork.GetBw((it.second)->GetId(), predNd->GetOccupied()); //bandwith
	    if (ct <= BW_ZERO){//get bandwith between nodes
	      ct = TaihuLightNetwork.GetBw((it.second)->GetLocation(), TaihuLight[predNd->GetOccupied()]->GetLocation(), true);
	      ava_time = std::max((float)0.0, TaihuLightNetwork.GetConAvaTime((it.second)->GetLocation(), TaihuLight[predNd->GetOccupied()]->GetLocation(), true) - this->global_timer);
	    }else{
	      ava_time = std::max((float)0.0, TaihuLightNetwork.GetConAvaTime((it.second)->GetId(), predNd->GetOccupied()) - this->global_timer);
	    }
	    ct = CommunicationDataSize(pred, ndId) / ct;

	    ct = std::max(ct, this->min_transmission_time);

	    /** Note: since the scheduled tasks are ready tasks,
		tmpEST = global_timer + ct
	    */
	    tmpEST = std::max(predNd->GetAFT(), global_timer) + ct;
	    if(Scheduler == DONFL || Scheduler == DONFL2 || Scheduler == ADONL){
	      tmpEST += ava_time;
	    }
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
	}else{ //HSIP and others
	  OEFT = EFT;
	}

	if ( (min_OEFT < 0) || (min_OEFT > OEFT) ){
	  min_OEFT = OEFT;
	  dev = it.second;
	}
      }
#if 0
      if(dev != NULL){
	std::cout<<"ld: "<<dev->GetLoad()<<" "<<mean_load<<" t: "<<(dev->GetAvaTime() - global_timer)<<std::endl;
      }
#endif
    }
      break;

    case DONF:
    case DONF2:
    case ADON:
    case DONFM:{
      /** Traverse all the devices,
	  pick the one with the min EFT.
      */
      float min_EFT = -1;
      // 1.Traverse all the devices
      for (auto& it: TaihuLight){
	if( CalcMemDmd(nd, it.second) > (it.second)->GetFreeRAM() + ZERO_POSITIVE ){
	  // The free memory of the device is too little, skip
	  continue;
	}

	if(nd->GetRatio((it.second)->GetType()) < ZERO_NEGATIVE){
	  //The task is not executable on the device
	  continue;
	}

	float w = nd->GetCompDmd() / (it.second)->GetCompPower() / nd->GetRatio((it.second)->GetType());
	// Add the memory allocation and access time
	w += (std::max(nd->GetDataDmd(), nd->GetDataGenerate()) + std::max(CalcMemDmd(nd, it.second), 0.0f)) / (it.second)->GetBw();

	// 2. Calculate EST by traversing all the pred(ndId)
	float ct;
	if(this->with_conflicts){
	  ct = CalcTransmissionTime(*nd, *(it.second), true, false);
	}else{
	  ct = CalcTransmissionTime(*nd, *(it.second), false, false);
	}

	// Since all the scheduled tasks' preds has been finished, we don't consider their AFT here.
	float EST = std::max((it.second)->GetAvaTime(), global_timer + ct);

	float EFT = EST + w;

	if ( (min_EFT < 0) || (min_EFT > EFT) ){
	  min_EFT = EFT;
	  dev = it.second;
	}

      }

    }
      break;

    case MULTILEVEL:
      break;

    case DATACENTRIC:{
      /** Pick the device according to the data location
	  or the min data transformation time.
       */
      float comtime, caltime, tmpweight, maxweight = 0;
      int ndidx;
      for (auto it : nd->input) {
	if ((tmpweight = CommunicationDataSize(it, ndId)) > maxweight ){
	  maxweight = tmpweight;
	  ndidx = it;
	}
      }
      comtime = maxweight / TaihuLightNetwork.GetMeanBW();
      caltime = nd->GetCompDmd() / GetMeanCP();

      if (comtime > (caltime * this->DCRatio)){// pick according to data location
	// Increase the counter and get the device
	dc_valid_counter++;
	dev = TaihuLight[global_graph.GetNode(ndidx)->GetOccupied()];
      }else{// pick according to EFT
	float min_OEFT = -1;
	for (auto& it: TaihuLight){
	  if( CalcMemDmd(nd, it.second) > (it.second)->GetFreeRAM() + ZERO_POSITIVE ){
	    // The free memory of the device is too little, skip
	    continue;
	  }

	  if(nd->GetRatio((it.second)->GetType()) < ZERO_NEGATIVE){
	    //The task is not executable on the device
	    continue;
	  }
	  float w = nd->GetCompDmd() / (it.second)->GetCompPower() / nd->GetRatio((it.second)->GetType());

	  // 2. Calculate EST by traversing all the pred(ndId)
	  float EST = 0;
	  float tmpEST;
	  // TODO: The logic below can be replaced by CalcTransmissionTime()
	  for (auto& pred : nd->input){
	    Node* predNd = global_graph.GetNode(pred);
	    if (predNd->GetOccupied() == it.first){
	      /** Execute on the same device
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
	  EST = std::max( EST, (it.second)->GetAvaTime() );

	  if ( (min_OEFT < 0) || (min_OEFT > EST + w) ){
	    min_OEFT = EST + w;
	    dev = it.second;
	  }
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

  /** Calculate mean load of all devices.
   */
  float Runtime::CalcMeanLoad(){
    float meanLoad = 0.0;
    int deviceNum = 1;
    for (auto& it: TaihuLight){
      meanLoad += ((it.second)->GetLoad() - meanLoad) / deviceNum;
      deviceNum ++;
    }
    return meanLoad;
  }

  /** Calculate the nearest time that a new decision can be made.
   */
  float Runtime::CalcNearestFinishTime(float start){
    float NearestTime = -0.1;
    std::map<int, float>::iterator ite;

    // 1. Search the execution_queue
    for (ite = execution_queue.begin(); ite != execution_queue.end(); ite++){
      if(ite->second <= start){
	continue;
      }
      if (NearestTime > ite->second || NearestTime < ZERO_NEGATIVE){
	NearestTime = ite->second;
      }
    }

    // 2. Search the block_free_queue
    for (auto& ite: block_free_queue){
      if(ite.second < start){
	continue;
      }
      if (NearestTime > ite.second  ||  NearestTime < ZERO_NEGATIVE){
	NearestTime = ite.second;
      }
    }

    // 3. If scheduler_ava_time is larger than global_timer, consider it as well
    if (this->scheduler_ava_time >= global_timer + ZERO_POSITIVE &&
	this->scheduler_ava_time > start){
      if (NearestTime > this->scheduler_ava_time  ||
	  NearestTime < ZERO_NEGATIVE){
	NearestTime = this->scheduler_ava_time;
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
  float Runtime::CalcTransmissionTime(Node nd, Device dev, bool withConflicts, bool setAvaTime){
    float data_transmission_time=0.0;

    //1. Calculate total_data_output
    float total_data_output = 0.0;
    for (std::set<int>::iterator iter = nd.input.begin(); iter != nd.input.end(); iter ++){
      Node* input_nd = global_graph.GetNode(*iter);
      if(input_nd->GetDataGenerate() > ZERO_POSITIVE){
	total_data_output += input_nd->GetDataGenerate();
      }else{
	total_data_output += std::max(input_nd->GetDataDmd(), (float)0.0);
      }
    }

    //2. Calculate data_trans_ratio
    float data_trans_ratio = 1.0;
    if(nd.GetDataConsume() > ZERO_POSITIVE){
      if( total_data_output > nd.GetDataConsume() ){
	data_trans_ratio = std::max(nd.GetDataConsume(), (float)0.0) / total_data_output;
      }
    }else{
      if( total_data_output > nd.GetDataDmd() ){
	data_trans_ratio = std::max(nd.GetDataDmd(), (float)0.0) / total_data_output;
      }
    }

    //3. Walk through all the input nodes of nd, and calc the transmission time
    std::map<int, float> TransSize;
    for (std::set<int>::iterator iter = nd.input.begin(); iter != nd.input.end(); iter ++){
      Node* input_nd = global_graph.GetNode(*iter);
      int input_dev_id = input_nd->GetOccupied();

      // On the same device, ignore the data transmission time
      // The input node is an entry node with entry task duplication, ignore the transmission time
      if (input_dev_id == dev.GetId() || (input_nd->GetInNum() == 0 && this->ETD)){
	continue;
      }

      // Calculate the total transmission data size from input_dev_id to dev
      if(TransSize.find(input_dev_id) == TransSize.end()){
	// There's not an entry, create one
	TransSize[input_dev_id] = 0.0;
      }
      if ( (global_graph.GetComCost( *iter, nd.GetId() )) >= ZERO_POSITIVE ) {
	// The edge has a weight
	TransSize[input_dev_id] += global_graph.GetComCost( *iter, nd.GetId() );
      }else{
	// The edge doesn't have a weight
	if(input_nd->GetDataGenerate() > ZERO_POSITIVE){
	  TransSize[input_dev_id] += std::max(input_nd->GetDataGenerate(), (float)0.0) * data_trans_ratio;
	}else{
	  TransSize[input_dev_id] += std::max(input_nd->GetDataDmd(), (float)0.0) * data_trans_ratio;
	}
      }
    }

    float network_bandwidth = 0.0;
    for(auto& dataSz : TransSize){
      int input_dev_id = dataSz.first;
      float transmission_size = dataSz.second;

      // The quatity of time from now to the link avaliable time
      float ava_time = 0.0;
      // Get the bandwith between the two devices
      if ((network_bandwidth = TaihuLightNetwork.GetBw(dev.GetId(), input_dev_id)) <= BW_ZERO){
	network_bandwidth = TaihuLightNetwork.GetBw(dev.GetLocation(), TaihuLight[input_dev_id]->GetLocation(),  true);
	ava_time = std::max(ava_time, TaihuLightNetwork.GetConAvaTime(dev.GetLocation(), TaihuLight[input_dev_id]->GetLocation(),  true) - this->global_timer);
      }else{//The devices are on the same node.
	ava_time = std::max(ava_time, TaihuLightNetwork.GetConAvaTime(dev.GetId(), input_dev_id) - this->global_timer);
      }

      float ct; // Communication time
      ct = transmission_size / network_bandwidth;
      ct = std::max(ct, this->min_transmission_time);

      if(withConflicts){
	data_transmission_time = std::max(ct + ava_time, data_transmission_time);
      }else{
	data_transmission_time = std::max(ct, data_transmission_time);
      }

      if(withConflicts && setAvaTime){
	if ((TaihuLightNetwork.GetBw(dev.GetId(), input_dev_id)) <= BW_ZERO){
	  TaihuLightNetwork.IncConAvaTime(dev.GetLocation(), TaihuLight[input_dev_id]->GetLocation(), ct, this->global_timer, true);

	}else{//The devices are on the same node.
	  TaihuLightNetwork.IncConAvaTime(dev.GetId(), input_dev_id, ct, this->global_timer);
	}
      }
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
    assert(nd.GetRatio(dev.GetType()) > ZERO_POSITIVE);
    calc_time = nd.GetCompDmd() / dev.GetCompPower() / nd.GetRatio(dev.GetType());

    //2. data access time
    data_time = (std::max(nd.GetDataDmd(), nd.GetDataGenerate()) +
		 std::max(nd.MemAlloc(), 0.0f)) / dev.GetBw();

    data_time = std::max(data_time, (float)0.0);

    //return std::max(calc_time, data_time);
    return std::max(calc_time + data_time, 0.0f);
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
	if(input_nd->GetDataGenerate() > ZERO_POSITIVE){
	  total_data_output += input_nd->GetDataGenerate();
	}else{
	  total_data_output += std::max(input_nd->GetDataDmd(), (float)0.0);
	}
      }

      // Get how much of the output data should be transfered.
      if(nd->GetDataConsume() > ZERO_POSITIVE){
	if( total_data_output > nd->GetDataConsume() ){
	  data_trans_ratio = std::max(nd->GetDataConsume(), (float)0.0) / total_data_output;
	}
      }else{
	if (total_data_output > nd->GetDataDmd()){
	  data_trans_ratio = std::max(nd->GetDataDmd(), (float)0.0) / total_data_output;
	}
      }

      if(global_graph.GetNode(predId)->GetDataGenerate() > ZERO_POSITIVE){
	dataSize = std::max(global_graph.GetNode(predId)->GetDataGenerate(), (float)0.0) * data_trans_ratio;
      }else{
	dataSize = std::max(global_graph.GetNode(predId)->GetDataDmd(), (float)0.0) * data_trans_ratio;
      }
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
      /** 4. Execution queue nodes
       */
      for (auto& it : execution_queue) {
	std::cout<<"| Node "<<it.first<<", finish time:"<<it.second<<std::endl;
      }
      std::cout<< RED <<"******************"<< RESET <<std::endl;
    }

    return dead_loop;
  }

  /** Change the scheduling policy during running time.
   */
  void Runtime::SetScheduler(SchedulePolicy sch){
    this->Scheduler = sch;
  }

  /** Get mean computation power.
   */
  float Runtime::GetMeanCP(){
    return this->mean_computing_power;
  }

  /** Calculate mean wait time of all tasks.
   */
  float Runtime::GetMeanWaitTime(){
    float meanwaittime = 0;
    int numtasks = 0;
    for (auto it : idset) {
      numtasks++;
      meanwaittime += (global_graph.GetNode(it)->GetWaitTime() - meanwaittime) / numtasks;
    }

    return meanwaittime;
  }

  /** Get max parallel value.
   */
  int Runtime::GetMaxParallel(){
    return this->max_parallel;
  }

  /** Set scheduling mean cost.
   */
  void Runtime::SetSchedulerCost(float sc){
    assert(sc >= 0.0);
    scheduler_mean_cost = sc;
  }

  /** Set alpha value of ADON and DONF2 policies.
   */
  void Runtime::SetAlpha(float alpha){
    assert(alpha >= ZERO_NEGATIVE);
    this->alpha_DON = alpha;
#ifdef DEBUG
    std::cout<<"Set alpha as: "<<alpha<<std::endl;
    std::cout<<"Check alpha: "<<this->alpha_DON<<std::endl;
#endif
  }

  /** Set load balance threshold value.
   */
  void Runtime::SetLoadBalanceThreshold(int threshold){
    assert(threshold >= 0);
    this->load_balance_threshold = threshold;
  }

  /** Set mem_full_threshold
   */
  void Runtime::SetMemFull(float full){
    assert(full > ZERO_POSITIVE && full < 1.0 - ZERO_POSITIVE);
    this->mem_full_threshold = full;
    std::cout<<"Set mem full: "<<this->mem_full_threshold<<std::endl;
  }

  /** Set dev_full_threshold
   */
  void Runtime::SetDevFull(float full){
    assert(full > ZERO_POSITIVE && full < 1.0 - ZERO_POSITIVE);
    this->dev_full_threshold = full;
  }

  /** Output SchedulePolicy as enum items.
   */
  std::ostream& operator<<(std::ostream& out, const SchedulePolicy value){
    static std::map<SchedulePolicy, std::string> strings;
    if (strings.size() == 0){
#define INSERT_ELEMENT(p) strings[p] = #p
      INSERT_ELEMENT(UNKNOWN);
      INSERT_ELEMENT(FCFS);
      INSERT_ELEMENT(SJF);
      INSERT_ELEMENT(RR);
      INSERT_ELEMENT(PRIORITY);
      INSERT_ELEMENT(HEFT);
      INSERT_ELEMENT(CPOP);
      INSERT_ELEMENT(PEFT);
      INSERT_ELEMENT(HSIP);
      INSERT_ELEMENT(DONF);
      INSERT_ELEMENT(DONF2);
      INSERT_ELEMENT(ADON);
      INSERT_ELEMENT(DONFM);
      INSERT_ELEMENT(DONFL);
      INSERT_ELEMENT(DONFL2);
      INSERT_ELEMENT(ADONL);
      INSERT_ELEMENT(MULTILEVEL);
      INSERT_ELEMENT(DATACENTRIC);
#undef INSERT_ELEMENT
    }

    return out << strings[value];
  }

  /** Output the simlulation report.
   */
  void Runtime::SimulationReport(){
    std::cout<<"max_cpath_cc: "<<this->max_cpath_cc<<", max_capth_cc_mem: "<<this->max_cpath_cc_mem<<", serial:"<<(global_graph.GetTotalCost()/this->max_devCompute)<<", with mem:"<<(global_graph.GetTotalCost()/this->max_devCompute + global_graph.TotalMemAcce()/TaihuLight[this->max_computeDevId]->GetBw())<<std::endl;
    float speedup = this->total_step * std::max(this->max_cpath_cc, (global_graph.GetTotalCost()/this->max_devCompute))/this->global_timer;
    float speedup_mem_accounted = this->total_step * std::max(this->max_cpath_cc_mem, (global_graph.GetTotalCost()/this->max_devCompute + global_graph.TotalMemAcce()/TaihuLight[this->max_computeDevId]->GetBw())) / this->global_timer;
    std::cout<<"-------- Simulation Report --------"<<std::endl;
    std::cout<<" Graph file: "<<graph_file_name<<std::endl;
    std::cout<<" Cluster: "<<cluster_file_name<<std::endl;
    std::cout<<" Scheduling Policy: "<<Scheduler<<std::endl;
    std::cout<<" DC Ratio: "<<DCRatio<<std::endl;
    std::cout<<" Alpha: "<<alpha_DON<<std::endl;
    std::cout<<" With Conflicts: "<<with_conflicts<<std::endl;
    std::cout<<" Mem full threshold: "<<mem_full_threshold<<std::endl;
    std::cout<<" Dev full threshold: "<<dev_full_threshold<<std::endl;
    std::cout<<" Total nodes: "<<task_total<<std::endl;
    std::cout<<" Global timer: "<<global_timer<<std::endl;
    std::cout<<" Max parallelism: "<<this->max_parallel<<std::endl;
    std::cout<<" Mean wait time: "<<GetMeanWaitTime()<<std::endl;
    std::cout<<" Cpath cost summary: "<<this->max_cpath_cc<<std::endl;
    std::cout<<" Scheduler cost: "<<this->scheduler_mean_cost<<std::endl;
    // Note: This speedup value maybe not so accurate!
    std::cout<<" Speedup: "<<speedup<<", "<<speedup_mem_accounted<<"(with mem)"<<std::endl;
    std::cout<<" Efficiency: "<<speedup / this->deviceNum<<std::endl;
    std::cout<<" SLR: "<<(this->global_timer/this->max_cpath_cc)<<std::endl;
    std::cout<<" SLR_its: "<<(this->global_timer/this->max_cpath_cc/this->total_step)<<std::endl;
    std::cout<<" Min free RAM size: "<<this->min_free_mem<<std::endl;

    int devId, tasks;
    float occupyTime, dataTransTime, totalRAM, freeRAM;
    Cluster::iterator it = TaihuLight.begin();
    for(; it != TaihuLight.end(); it++){
      devId = it->first;
      occupyTime = (it->second)->GetRunTime();
      dataTransTime = (it->second)->GetTransTime();
      tasks = (it->second)->GetTasks();
      totalRAM = (it->second)->GetRAM();
      freeRAM = (it->second)->GetFreeRAM();
      assert(devId >= 0);
      assert(occupyTime >= 0.0);

      std::cout<<" Device id:"<<devId<<"  occupied time:"<<occupyTime<<"  proportion:"<<occupyTime/global_timer<<"  data transfer time:"<<dataTransTime<<", finished number of tasks:"<<tasks<<", total RAM:"<<totalRAM<<", free RAM:"<<freeRAM<<std::endl;
    }
    std::cout<<"Total tasks:"<<this->task_total<<"\n\tDONF hit times:"<<this->task_hit_counter<<" propertion:"<<float(this->task_hit_counter)/this->task_total<<"\n\tDatacentric hit times:"<<this->dev_hit_counter<<" propertion:"<<float(this->dev_hit_counter)/this->task_total<<"\n\tDatacentric valid times:"<<this->dc_valid_counter<<" propertion:"<<float(this->dc_valid_counter)/this->task_total<<std::endl;

    std::cout<<"\nTiming info:"<<std::endl;
    std::cout<<"\tGraph init time: "<<this->graph_init_time<<" s"<<std::endl;
    std::cout<<"\tCluster init time: "<<this->cluster_init_time<<" s"<<std::endl;
    std::cout<<"\tCalcOCT time: "<<this->oct_time<<" s"<<std::endl;
    std::cout<<"\tRank OCT time: "<<this->rankoct_time<<" s"<<std::endl;
    std::cout<<"\tRank u time: "<<this->rank_u_time<<" s"<<std::endl;
    std::cout<<"\tRank d time: "<<this->rank_d_time<<" s"<<std::endl;
    std::cout<<"\tNDON time: "<<this->ndon_time<<" s"<<std::endl;
    std::cout<<"\tTask pick time: "<<this->task_pick_time<<" s"<<std::endl;
    std::cout<<"\tDevice pick time: "<<this->device_pick_time<<" s"<<std::endl;
    std::cout<<"-----------------------------------"<<std::endl;

    std::cout<<"-------Running History-------"<<std::endl;
    FILE* fp = fopen("dev_mapping", "w+");
    if(fp == NULL){
      exit(-1);
    }else{
      for(auto i : running_history){
	//std::cout<<i.first<<","<<i.second<<std::endl;
	fprintf(fp, "%d,%d\n", i.first, i.second);
      }
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

  /** Return TaihuLightNetwork.
      Just for testing.
  */
  Connections Runtime::GetConnections(){
    return TaihuLightNetwork;
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
    assert(BlockSize >= 0);
    assert(ReferNum >= 0); // The sink node has ReferNum=0

    // At least 1KB for a block size, to avoid error.
    BlockSize = std::max(BlockSize, 1);

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
