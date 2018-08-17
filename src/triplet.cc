// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "device.h"
#include "graph.h"
#include "runtime.h"
#include "utils.h"
#include <iostream>
#include <ctime>
#include <cassert>
#include <cstring>
// TODO: conditional include?
#include <getopt.h>

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

/** Output usage of tripletrun.
 */
void Usage(){
  std::cout<<"Usage: ./triplet [options]"<<std::endl;
  std::cout<<"\tOptions:"<<std::endl;
  std::cout<<"\t-a, --alpha <value>: alpha for DONF2 and ADON policies."<<std::endl;
  std::cout<<"\t-c, --cluster <clusterfile>: set cluster file."<<std::endl;
  std::cout<<"\t-d, --dcratio <value>: Data-Centric ratio value."<<std::endl;
  std::cout<<"\t-f, --devfull <value>: dev_full_threshold"<<std::endl;
  std::cout<<"\t-g, --graph <graphfile>: set graph file."<<std::endl;
  std::cout<<"\t-h, --help: output this usage message."<<std::endl;
  std::cout<<"\t-l, --loadbalance <value>: load balance threshold task number"<<std::endl;
  std::cout<<"\t-m, --memfull <value>: mem_full_threshold"<<std::endl;
  std::cout<<"\t-s, --scheduler <policy>: set schduling policy."<<std::endl;
  std::cout<<"\t-t, --scost <value>: Scheduler cost"<<std::endl<<std::endl;

  std::cout<<"\tScheduler supported: RR, FCFS, SJF, PEFT, HSIP, DONF, DONF2, DONFM, HEFT, CPOP, DATACENTRIC, ADON"<<std::endl;
}

int main(int argc, char *argv[])
{
  DECLARE_TIMING(triplet);
  START_TIMING(triplet);
  // Start information
  std::cout<<"TripletRun 0.0.1 alpha"<<std::endl;
  time_t now = time(0);
  char* dt = ctime(&now);
  std::cout<<dt<<std::endl;

  // Command

  triplet::SchedulePolicy scheduler = triplet::HSIP;
  const char* graphfile = "graph.json";
  const char* clusterfile = "cluster.json";

  int c;
  float dcratio = 1.0;
  int lb_threshold = 0;
  float scheduler_cost = 0.0;
  float alpha = 0.5;
  float memfull_t = 0.9;
  float devfull_t = 0.2;
  bool with_conflicts = false;

  while (1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
      {"alpha", 1, 0, 'a'},
      {"help", 0, 0, 'h'},
      {"graph", 1, 0, 'g'},
      {"cluster", 1, 0, 'c'},
      {"dcratio", 1, 0, 'd'},
      {"loadbalance", 1, 0, 'l'}, // Load balance threshold task number
      {"scheduler", 1, 0, 's'},
      {"scost", 1, 0, 't'}, // Scheduler cost
      {"memfull", 1, 0, 'm'}, //mem_full_threshold
      {"devfull", 1, 0, 'f'}, //dev_full_threshold
      {"with-conflicts", 1, 0, 'w'}, // with conflicts for DONF policies
      {0, 0, 0, 0}
    };

    c = getopt_long(argc, argv, "a:c:d:f:g:hl:m:s:t:w:",
		    long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 'a':
      assert(optarg);
      alpha = atof(optarg);
      break;
    case 'c':
      assert(optarg);
      clusterfile = optarg;
      std::cout<<"Set cluster file: "<<clusterfile<<std::endl;
      break;
    case 'd':
      assert(optarg);
      dcratio = atof(optarg);
      break;
    case 'f':
      assert(optarg);
      devfull_t = atof(optarg);
      break;
    case 'g':
      assert(optarg);
      graphfile = optarg;
      std::cout<<"Set graph file: "<<graphfile<<std::endl;
      break;
    case 'h':
      Usage();
      exit(0);
      break;
    case 'l':
      assert(optarg);
      lb_threshold = atoi(optarg);
      break;
    case 'm':
      assert(optarg);
      memfull_t = atof(optarg);
      break;
    case 's':
      assert(optarg);
      if(strcmp("RR", optarg) == 0 || strcmp("rr", optarg) == 0){
	scheduler = triplet::RR;
	std::cout<<"scheduler RR"<<std::endl;
      }else if(strcmp("FCFS", optarg) == 0 || strcmp("fcfs", optarg) == 0){
	scheduler = triplet::FCFS;
	std::cout<<"scheduler FCFS"<<std::endl;
      }else if(strcmp("SJF", optarg) == 0 || strcmp("sjf", optarg) == 0){
	scheduler = triplet::SJF;
	std::cout<<"scheduler SJF"<<std::endl;
      }else if(strcmp("PEFT", optarg) == 0 || strcmp("peft", optarg) == 0){
	scheduler = triplet::PEFT;
	std::cout<<"scheduler PEFT"<<std::endl;
      }else if(strcmp("HSIP", optarg) == 0 || strcmp("hsip", optarg) == 0){
	scheduler = triplet::HSIP;
	std::cout<<"scheduler HSIP"<<std::endl;
      }else if(strcmp("DONF", optarg) == 0 || strcmp("donf", optarg) == 0){
	scheduler = triplet::DONF;
	std::cout<<"scheduler DONF"<<std::endl;
      }else if(strcmp("DONF2", optarg) == 0 || strcmp("donf2", optarg) == 0){
	scheduler = triplet::DONF2;
	std::cout<<"scheduler DONF2"<<std::endl;
      }else if(strcmp("DONFM", optarg) == 0 || strcmp("donfm", optarg) == 0){
	/** DONFM: Firstly pick tasks that tend to release memory full devices
	 */
	scheduler = triplet::DONFM;
	std::cout<<"scheduler DONFM"<<std::endl;
      }else if(strcmp("HEFT", optarg) == 0 || strcmp("heft", optarg) == 0){
	scheduler = triplet::HEFT;
	std::cout<<"scheduler HEFT"<<std::endl;
      }else if(strcmp("CPOP", optarg) == 0 || strcmp("cpop", optarg) == 0){
	scheduler = triplet::CPOP;
	std::cout<<"scheduler CPOP"<<std::endl;
      }else if(strcmp("DC", optarg) == 0 || strcmp("dc", optarg) == 0){
	scheduler = triplet::DATACENTRIC;
	std::cout<<"scheduler DATACENTRIC"<<std::endl;
      }else if(strcmp("ADON", optarg) == 0 || strcmp("adon", optarg) == 0){
	scheduler = triplet::ADON;
	std::cout<<"scheduler ADON"<<std::endl;
      }else{
	std::cout<<"Error: cannot identify scheduler "<<optarg<<std::endl;
	exit(1);
      }
      break;
    case 't':
      assert(optarg);
      scheduler_cost = atof(optarg);
      break;
    case 'w':
      assert(optarg);
      if(strcmp("True", optarg) == 0 || strcmp("true", optarg) == 0){
	with_conflicts = true;
	std::cout<<"With conflicts: "<<with_conflicts<<std::endl;
      }else if(strcmp("False", optarg) == 0 || strcmp("false", optarg) == 0){
	with_conflicts = false;
	std::cout<<"With conflicts: "<<with_conflicts<<std::endl;
      }else{
	std::cout<<"Error: cannot identify w option "<<optarg<<std::endl;
	exit(1);
      }
      break;
    default:
      std::cout<<"Option processing error!"<<std::endl;
    }
  }

  if (optind < argc) {
    std::cout<<"non-option ARGV-elements: ";
    while (optind < argc)
      std::cout<<argv[optind++];
    std::cout<<std::endl;
  }

  triplet::Runtime rt;

  DECLARE_TIMING(graph);
  DECLARE_TIMING(cluster);

  START_TIMING(graph);
  rt.InitGraph(graphfile);
  STOP_TIMING(graph);
  std::cout<<" Graph init time: "<<GET_TIMING(graph)<<" s"<<std::endl;

  START_TIMING(cluster);
  rt.InitCluster(clusterfile);
  STOP_TIMING(cluster);
  std::cout<<" Cluster init time: "<<GET_TIMING(cluster)<<" s"<<std::endl;

  rt.SetAlpha(alpha);
  rt.SetMemFull(memfull_t);
  rt.SetDevFull(devfull_t);
  rt.InitRuntime(scheduler, dcratio, with_conflicts);
  rt.SetLoadBalanceThreshold(lb_threshold);
  rt.SetSchedulerCost(scheduler_cost);
  rt.Execute();

  STOP_TIMING(triplet);
  std::cout<<"Total execution time: "<<GET_TIMING(triplet)<<std::endl;
  return 0;
}
