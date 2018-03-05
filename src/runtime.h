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
  typedef std::map<int, Device*> Cluster;

  enum SchedulePolicy {
    UNKNOWN = 0,
    FCFS, // First come, first serve. Pick the first task, put it on the fastest device
    SJF, // Shortest job first, pick the shortest task and put it on the fastest free device
    RR, // Round robin, pick the first task from the ready queue and put it on the next free device
    PRIORITY, // Priority, pick the priority most task and put it on the fastest device
    HEFT,
    PEFT,
    HSIP,
    DONF, // Degree of node first
    DONF2, // 2-degree degree of node first
    MULTILEVEL, // Multi-level
    DATACENTRIC // Data-centric
    };

  // Class MemoryBlock
  class MemoryBlock{
  public:
    MemoryBlock();
    MemoryBlock(int id, int devid, int size, int refers);
    ~MemoryBlock();

    /** Decrease the refer number (by 1) and return the decreased number.
     */
    int DecRefers(int number = 1);

    /** Allocate the memory block physically
     */
    void DoAlloc(Cluster TaihuLight);

    /** Free this memory block on corresponding device
     */
    void DoFree(Cluster TaihuLight);

    /** Return the number of refers of this memory block
    */
    int GetRefers();

    /** Get device location.
     */
    int DeviceLocation();

    /** Get block size.
     */
    int GetBlockSize();

  protected:
    int BlockId; // Set it the same as node id is a good choice
    int DeviceId; // on which device this block locates
    int BlockSize;
    int ReferNum;
  };// Class MemoryBlock

  // Class Runtime
  class Runtime{
  public:
    Runtime();
    ~Runtime();

    /** Init the global graph from configure JSON file.
     */
    void InitGraph(const char * graphFile);

    /** Init the cluster "TaihuLight" from configure file.
    */
    void InitCluster(const char * clusterFile);

    /** Init the runtime data structures: pending_list and ready_queue
	and calculate the OCT, RankOCT of the graph.
     */
    void InitRuntime(SchedulePolicy sch = PEFT, float dc = 1.0);

    /** Calculate the OCT (Optimistic Cost Table) used in PEFT.
	This function cannot be moved into class Graph
	since it involves not only the graph topology but also the cluster configures.
     */
    void CalcOCT();

    /** Calculate the rank_oct used in PEFT based on OCT.
     */
    void CalcRankOCT();

    /** Calculate the computation cost mean value and standard deviation value
	of node ndId on different devices.
	For convinience, return weight_mean * sd directly.
    */
    float CalcWeightMeanSD(int ndId);

    /** Calculate rank_u, which is used in HSIP policy.
	Before call this, remember to call graph.InitAllOCCW()
     */
    void CalcRank_u();

    /** Calculate normalized degree of node.
     */
    float NDON(Node * nd, int degree = 1);

    /** The whole execution logic.
     */
    // TODO: Count the schduling time itself
    void Execute();

    /** Pick a task from ready queue according to the
	scheduling policy.
	Erase the correspinding task index from ready_queue.
    */
    int TaskPick(SchedulePolicy sch = UNKNOWN);

    /** Pick a free device according to the task requirements and
	scheduling policy.
    */
    Device* DevicePick(int ndId, SchedulePolicy sch = UNKNOWN);

    /** Calculate the nearest time that a new decision can be made.
     */
    float CalcNearestFinishTime();

    /** Calculate the data transmission time if we put nd on dev,
	return the result as a float.
     */
    float CalcTransmissionTime(Node nd, Device dev);

    /** Calculate the execution time if we put nd on dev,
	return the result as a float.
     */
    float CalcExecutionTime(Node nd, Device dev);

    /** Get the data size need to be transfered from predId to succId.
	Return it as a float value.
    */
    float CommunicationDataSize(int predId, int succId);

    /** Use entry task duplication policy on nd.
     */
    void EntryTaskDuplication(Node* nd);

    /** Detect dead loop and return the result.
	If a dead loop is detected, report it and return true, otherwise return false.
     */
    bool DeadLoopDetect();

    /** Change the scheduling policy during running time.
     */
    void SetScheduler(SchedulePolicy sch);

    /** Get mean computation power.
     */
    float GetMeanCP();

    /** Calculate mean wait time of all tasks.
     */
    float GetMeanWaitTime();

    /** Get max parallel value.
     */
    int GetMaxParallel();

    /** Set scheduling mean cost.
     */
    void SetSchedulerCost(float sc);

    /** Output the simlulation report.
     */
    void SimulationReport();

    /** Return the global_graph.
	Just for testing.
     */
    Graph GetGraph();

    /** Return TaihuLight.
	Just for testing.
     */
    Cluster GetCluster();

    /** Return TaihuLightNetwork.
	Just for testing.
     */
    Connections GetConnections();

    /** Return the execution_queue.
	Just for testing.
     */
    std::map<int, float> GetExeQueue();

    /** Return the ready_queue.
	Just for testing.
     */
    std::vector<int> GetReadyQueue();

  protected:
    bool ETD; // Entry task duplication flag
    int deviceNum; // The total number of devices in TaihuLight.
    int deviceInUse; // The number of devices in BUSY status.
    SchedulePolicy Scheduler; // define the scheduler
    Graph global_graph; // The computing graph.
    Cluster TaihuLight; // The cluster.
    Connections TaihuLightNetwork; // Network of the cluster.
    float scheduler_ava_time; // When the next scheduling could occur.
    float global_timer; // For recording global time.
    float scheduler_mean_cost; // Mean cost of scheduling.
    float mean_computing_power; // Mean computation power of all devices.
    float DCRatio; // Data centric ratio, used in devicepick
    int RRCounter; // Counter for RR policy
    int max_devId; // Max device id in the cluster
    int max_parallel; // Max parallelism
    float max_devCompute; // Max device compute power

    int task_total; // Total tasks scheduled
    int task_hit_counter; // DONF task pick hit counter
    int dev_hit_counter; // DATACENTRIC device pick hit
    int dc_valid_counter; // The times that DATACENTRIC really works

    double graph_init_time;
    double cluster_init_time;
    double oct_time;
    double rankoct_time;
    double rank_u_time;

    std::string graph_file_name;
    std::string cluster_file_name;

    float **OCT; // The Optimistic Cost Table used in PEFT

    /* TODO: move this idset into class graph? */
    std::set<int> idset;  //Node id set
    std::set<int> computerset; //computer node set
    std::vector<int> ready_queue; // nodeid
    std::map<int, int> running_history; //nodeid -> deviceid
    std::map<int, float> execution_queue; // nodeid -> execution finish time
    std::vector<std::pair<int,float>>block_free_queue; // blockid -> refer decrease time
    std::map<int, int> pending_list; // nodeid -> pending input
    std::map<int, MemoryBlock*> BlocksMap; // Memory blocks pool, blockid -> memory block
  };// Class Runtime
}

#endif //TRIPLET_RUNTIME_H_
