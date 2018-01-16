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
  class Node{
  public:
    Node();
    Node(int id, float compDmd, float dataDmd);
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
    void SetRank_u(float rank);

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
    float GetRank_u();

    /** Get actual finish time (AFT) of the node.
     */
    float GetAFT();

    /** The id set of the succ nodes.
     */
    nodeset output;

    /** The id set of the pred nodes.
     */
    nodeset input;
    
  protected:
    int id_;
    int occupied_device; // the occupied device id
    float computing_demand;
    float data_demand;

    float occw; /** occw: out-degree communication cost weight */
    float rank_OCT; // Used in PEFT as the priority.
    float rank_u; // Used in HSIP policy
    float AFT; // The actual finish time of this node
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
    void AddNode(int id, float comDmd, float dataDmd);

    /** Add an edge from src to dst.
	Also give the communication cost if need.
     */
    void AddEdge(int src, int dst, int comCost = -1);

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

    /** Clean up the graph, destory
	everything in it.
     */
    void Clear();

    typedef std::map<int,Node*> graphmap;
  protected:
    graphmap graph_;
    std::map<std::pair<int ,int>, int> comCostMap;
    int numEdge;
    int numNode;
  };
}

#endif //TRIPLET_GRAPH_H_
