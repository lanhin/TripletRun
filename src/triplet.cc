// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "device.h"
#include "graph.h"
#include "runtime.h"
#include <iostream>

const char* StatusArray[] = {
  "FREE",
  "BUSY",
  "UNAVAILABLE"
};

/** Output a device's infomation.
 */
void ShowDeviceInfo(triplet::Device dev){
  std::cout<<"========Device info========"<<std::endl;
  std::cout<<" ID: "<<dev.GetId()<<std::endl;
  std::cout<<" Status: "<<StatusArray[dev.GetStatus()]<<std::endl;
  std::cout<<" Computing Power: "<<dev.GetCompPower()<<std::endl;
  std::cout<<" RAM(KB): "<<dev.GetRAM()<<std::endl;
  std::cout<<" Bandwidth(KB/s): "<<dev.GetBw()<<std::endl;
  std::cout<<" Location: "<<dev.GetLocation()<<std::endl;
  std::cout<<"==========================="<<std::endl;
}
 
/** Output a graph's information, especially the nodes.
 */
void ShowGraphInfo(triplet::Graph gra, std::set<int> idset){
  std::cout<<"========Graph info========="<<std::endl;
  std::cout<<" Num of nodes: "<<gra.Edges()<<std::endl;
  std::cout<<" Num of edges: "<<gra.Nodes()<<std::endl;
  for (std::set<int>::iterator iter = idset.begin(); iter != idset.end(); iter ++){
    triplet::Node* nd = gra.GetNode(*iter);
    std::cout<<std::endl<<" Node id: "<<*iter<<std::endl;
    std::cout<<" Node computing demand: "<<nd->GetCompDmd()<<std::endl;
    std::cout<<" Node data demand: "<<nd->GetDataDmd()<<std::endl;
    std::cout<<" Node occupied: "<<nd->GetOccupied()<<std::endl;
    std::cout<<" Node input: "<<nd->GetInNum()<<std::endl;
    std::cout<<" Node output: "<<nd->GetOutNum()<<std::endl;
  }
  std::cout<<"==========================="<<std::endl;
}

int main(int argc, char *argv[])
{
  // Create and init devices
  triplet::Device deva(0, 1.0, 2048, 96.0, 0);

  //ShowDeviceInfo(deva);

  // Read and init graph
  triplet::Graph gra;
  std::set<int> idset;
  gra.AddNode(0);
  gra.AddNode(1, 10.0, 2048.0);
  gra.GetNode(0)->SetCompDmd(8.8);
  gra.GetNode(0)->SetDataDmd(6.6);

  gra.AddEdge(0, 1);

  idset.insert(0);
  idset.insert(1);

  //ShowGraphInfo(gra, idset);

  // Process the graph

  triplet::Runtime rt;
  rt.InitGraph("graph.json");
  rt.InitCluster("cluster.json");
  rt.InitRuntime();
  rt.Execute();
  
  return 0;
}
