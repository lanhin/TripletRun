// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#ifndef TRIPLET_GRAPH_H_
#define TRIPLET_GRAPH_H_

#include <set>
#include <map>

namespace triplet{
  enum NodeStatus {
    INIT = 1,
    READY,
    RUNNING,
    FINISHED
  };

  class Node{
  public:
    Node();
    Node(int id, float compDmd, float dataDmd, float dataConsume, float dataGenerate);
    ~Node();

    typedef std::set<int> nodeset;

    /** Set the id of the node.
     */
    void SetId(int id);

    /** Record which device is occupied by the node.
     */
    void SetOccupied(int id);

    /** Set the computation demand of the node.
     */
    void SetCompDmd(float demand);

    /** Set the data demand of the node.
     */
    void SetDataDmd(float demand);

    /** Set occw of the node.
     */
    void SetOCCW(float occw_value);

    /** Add an edge to the node: an pred node.
     */
    void AddInput(int inNode);

    /** Add an edge to the node: an succ node.
     */
    void AddOutput(int outNode);

    /** Set rank_OCT of the node,
	which is used in PEFT policy.
     */
    void SetRankOCT(float rank);

    /** Set rank_u of the node,
	which is used in HSIP policy.
    */
    void SetRank_u_HSIP(float rank);

    /** Set rank_u of the node,
	which is used in HEFT policy.
    */
    void SetRank_u_HEFT(float rank);

    /** Set actual finish time (AFT) of the node.
     */
    void SetAFT(float aft);

    /** Get the id of the node.
     */
    int GetId();

    /** Get the occupied device's id by the node.
     */
    int GetOccupied();

    /** Get computation demand of the node.
     */
    float GetCompDmd();

    /** Get data demand of the node.
     */
    float GetDataDmd();

    /** Get data consume of the node.
     */
    float GetDataConsume();

    /** Get data generate of the node.
     */
    float GetDataGenerate();

    /** Get in-degree of the node:
	the number of pred nodes.
     */
    int GetInNum();

    /** Get out-degree of the node:
	the number of the succ nodes.
     */
    int GetOutNum();

    /** Get occw of the node.
     */
    float GetOCCW();

    /** Get rank_OCT of the node,
	used in PEFT policy.
     */
    float GetRankOCT();

    /** Get rank_u of the node,
	used in HSIP policy.
    */
    float GetRank_u_HSIP();

    /** Get rank_u of the node,
	used in HEFT policy.
    */
    float GetRank_u_HEFT();


    /** Get actual finish time (AFT) of the node.
     */
    float GetAFT();

    /** Set the mean weight value.
     */
    void SetMeanWeight(float mean);

    /** Get the mean weight value.
	If a negative value (-1) is returned, it means it has not been calculated.
     */
    float GetMeanWeight();

    /** Set waiting time of the node.
     */
    void SetWaitTime(float time);

    /** Get waiting time of the node.
     */
    float GetWaitTime();

    /** Get the critical path computation cost value of this node.
     */
    float GetCpathCC();

    /** Set the critical path computation cost value of this node.
     */
    void SetCpathCC(float cc);

    /** Get the rank_d used in CPOP policy.
     */
    float GetRank_d_CPOP();

    /** Set the rank_d used in CPOP policy.
     */
    void SetRank_d_CPOP(float rankd);

    /** Set the priority of CPOP policy.
     */
    void SetPriorityCPOP(float priority);

    /** Get the priority of CPOP policy.
     */
    float GetPriorityCPOP();

    /** Set NDON value.
     */
    void SetNDON(float degree);

    /** Get NDON value.
     */
    float GetNDON();

    /** Set rank_ADON value.
     */
    void SetRank_ADON(float degree);

    /** Get rank_ADON value.
     */
    float GetRank_ADON();

    /** Get the node's level
     */
    int GetLevel();

    /** Set the node's level
     */
    void SetLevel(int lvl);

    /** Set node status.
     */
    void SetStatus(NodeStatus st);

    /** Get node status.
     */
    NodeStatus GetStatus();

    /** Check if the node is in the ready queue.
     */
    bool IsReady();

    /** Add a ratio for device type TYPE.
     */
    void AddRatio(int type, float ratio);

    /** Get speed ratio for device type TYPE.
     */
    float GetRatio(int type);

    /** The id set of the succ nodes.
     */
    nodeset output;

    /** The id set of the pred nodes.
     */
    nodeset input;
    
  protected:
    NodeStatus status; //INIT, READY, RUNNING and FINISHED
    int id_;
    int occupied_device; // the occupied device id
    int level; // The min levels away from source node
    float computing_demand;
    float data_demand;
    float data_consume;
    float data_generate;

    /** mean_weight: average execution time of the node (on all devices)
	Note: this is different from the computing_demand.
     */
    float mean_weight;
    float occw; /** occw: out-degree communication cost weight */
    float rank_OCT; // Used in PEFT as the priority.
    float rank_u_HSIP; // Used in HSIP policy
    float rank_u_HEFT; // Used in HEFT policy
    float rank_d_CPOP; // Rank_d used in CPOP policy
    float priority_CPOP; // Priority = rank_u_HEFT + rank_d_CPOP
    float AFT; // The actual finish time of this node
    float wait_time; // The waiting time of the node
    float cpath_cc; // Critical path computation cost value
    float NDON; // Normalized degree of node
    float rank_ADON; // Rank used in ADON policy

    std::map<int, float> SpeedRatio; //Speed ratios of different device type
  };

  class Graph{
  public:
    Graph();
    ~Graph();

    /** Add a node into the graph.
	id: node id
	comDmd: computation demand
	dataDmd: data demand
     */
    void AddNode(int id);
    void AddNode(int id, float comDmd, float dataDmd, float dataConsume = -1.0, float dataGenerate = -1.0);

    /** Add an edge from src to dst.
	Also give the communication cost if need.
     */
    void AddEdge(int src, int dst, float comCost = -1);

    /** Get a node pointer according to the given node id.
     */
    Node* GetNode(int id);

    /** Return the total number of edges.
     */
    int Edges();

    /** Return the total number of nodes.
     */
    int Nodes();

    /** Get communication cost between two nodes.
	Return -1 if there's no weight configure
	of the edge; return the cost otherwise.
     */
    int GetComCost(int src, int dst);

    /** Calculate Out-degree Communication Cost Weight (OCCW) of a graph node.
     */
    float OCCW(int ndId);

    /** Calculate all the OCCWs of all the nodes.
     */
    void InitAllOCCW();

    /** Calculate the priority value used in CPOP policy.
	Return the max priority value, which is the absCP in CPOP.
     */
    float CalcPriorityCPOP();

    /** Return the max node id in the graph.
	For OCT matrix construction.
     */
    int MaxNodeId();

    /** Calculate the max device compute power of node ndId.
     */
    void CalcCpathCC(int ndId, float max_devCompute, float min_execution_time);

    /** Get total computation cost.
     */
    float GetTotalCost();

    /** Set source vertex id.
     */
    void SetSourceId(int source);

    /** Get source vertex id.
     */
    int GetSourceId();

    /** Set sink vertex id.
     */
    void SetSinkId(int sink);

    /** Get sink vertex id.
     */
    int GetSinkId();

    /** Clean up the graph, destory
	everything in it.
     */
    void Clear();

    /** Report the 3 summary value.
     */
    void SummaryReport();

    typedef std::map<int,Node*> graphmap;
  protected:
    graphmap graph_;
    int numEdge;
    int numNode;
    int maxNode; //Max node id
    int sourceId; // Source vertex id
    int sinkId; // Sink vertex id

    /* TODO: Record accuracy. */
    float total_computation_cost; //Summary of computation cost
    float total_memory_cost; //Summary of memory cost
    float total_edge_weight; //Summary of edge weight

    std::map<std::pair<int ,int>, int> comCostMap;  //Communication cost map
  };
}

#endif //TRIPLET_GRAPH_H_
