// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "runtime.h"
#include "json/json.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>


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

      triplet::Node nd = global_graph.GetNode(id1);
      std::cout<<std::endl<<" Node id: "<<id1<<std::endl;
      std::cout<<" Node computing demand: "<<nd.GetCompDmd()<<std::endl;
      std::cout<<" Node data demand: "<<nd.GetDataDmd()<<std::endl;
      std::cout<<" Node input: "<<nd.GetInNum()<<std::endl;
      std::cout<<" Node output: "<<nd.GetOutNum()<<std::endl;
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
	  TaihuLight[devId]->SetFree();

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

  void Runtime::SimulationReport(){
    // TODO: implementation    
  }
}
