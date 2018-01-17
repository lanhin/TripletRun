// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "graph.h"
#include "constants.h"
#include <cassert>
#include <iostream>

namespace triplet{
  // class Node
  Node::Node()
    :id_(-1), rank_OCT(0), mean_weight(-1.0) {}
  Node::Node(int id, float compDmd, float dataDmd){
    id_ = id;
    computing_demand = compDmd;
    data_demand = dataDmd;
    rank_OCT = 0;
    mean_weight = -1.0;
  }

  Node::~Node(){
    input.clear();
    output.clear();
    assert(input.empty());
    assert(output.empty());
  }

  /** Set the id of the node.
   */
  void Node::SetId(int id){
    assert(id >= 0);
    id_ = id;
  }

  /** Record which device is occupied by the node.
   */
  void Node::SetOccupied(int id){
    assert(id >= 0);
    occupied_device = id;
  }

  /** Set the computation demand of the node.
   */
  void Node::SetCompDmd(float demand){
    assert(demand > 0.0);
    computing_demand = demand;
  }

  /** Set the data demand of the node.
   */
  void Node::SetDataDmd(float demand){
    assert(demand > 0.0);
    data_demand = demand;
  }

  /** Set occw of the node.
  */
  void Node::SetOCCW(float occw_value){
    assert(occw_value >= 0.0);
    this->occw = occw_value;
  }

  /** Add an edge to the node: an pred node.
   */
  void Node::AddInput(int inNode){
    assert(inNode >= 0);
    input.insert(inNode);
  }

  /** Add an edge to the node: an succ node.
   */
  void Node::AddOutput(int outNode){
    assert(outNode >= 0);
    output.insert(outNode);
  }

  /** Set rank_OCT of the node,
      which is used in PEFT policy.
  */
  void Node::SetRankOCT(float rank){
    rank_OCT = rank;
  }

  /** Set rank_u of the node,
      which is used in HSIP policy.
  */
  void Node::SetRank_u(float rank){
    rank_u = rank;
  }

  /** Set actual finish time (AFT) of the node.
   */
  void Node::SetAFT(float aft){
    assert(aft >= ZERO_NEGATIVE);

    this->AFT = aft;
  }

  /** Get the id of the node.
   */
  int Node::GetId(){
    return id_;
  }

  /** Get the occupied device's id by the node.
   */
  int Node::GetOccupied(){
    return occupied_device;
  }

  /** Get computation demand of the node.
   */
  float Node::GetCompDmd(){
    return computing_demand;
  }

  /** Get data demand of the node.
   */
  float Node::GetDataDmd(){
    return data_demand;
  }

  /** Get in-degree of the node:
      the number of pred nodes.
  */
  int Node::GetInNum(){
    return input.size();
  }

  /** Get out-degree of the node:
      the number of the succ nodes.
  */
  int Node::GetOutNum(){
    return output.size();
  }

  /** Get occw of the node.
  */
  float Node::GetOCCW(){
    return this->occw;
  }

  /** Get rank_OCT of the node,
      used in PEFT policy.
  */
  float Node::GetRankOCT(){
    return rank_OCT;
  }

  /** Get rank_u of the node,
      used in HSIP policy.
  */
  float Node::GetRank_u(){
    return rank_u;
  }

  /** Get actual finish time (AFT) of the node.
   */
  float Node::GetAFT(){
    assert(this->AFT >= ZERO_NEGATIVE);
    return this->AFT;
  }

  /** Set the mean weight value.
   */
  void Node::SetMeanWeight(float mean){
    this->mean_weight = mean;
  }

  /** Get the mean weight value.
      If a negative value (-1) is returned, it means it has not been calculated.
   */
  float Node::GetMeanWeight(){
    return this->mean_weight;
  }


  // class graph
  Graph::Graph(){
    numEdge = 0;
    numNode = 0;
    maxNode = -1;
  }

  Graph::~Graph(){
    graph_.clear();
    numEdge = 0;
    numNode = 0;
    assert(graph_.empty());
  }

  /** Add a node into the graph.
      id: node id
  */
  void Graph::AddNode(int id){
    assert(id >= 0);
    graphmap::iterator it;
    it = graph_.find(id);
    assert(it == graph_.end()); //Error: the node has already created

    Node * node = new Node();
    node->SetId(id);
    graph_[id] = node;
    numNode++;
    maxNode = std::max(maxNode, id);
  }

  /** Add a node into the graph.
      id: node id
      comDmd: computation demand
      dataDmd: data demand
  */
  void Graph::AddNode(int id, float comDmd, float dataDmd){
    assert(id >= 0);
    graphmap::iterator it;
    it = graph_.find(id);
    assert(it == graph_.end()); //Error: the node has already created
    
    Node * node = new Node();
    node->SetId(id);
    node->SetCompDmd(comDmd);
    node->SetDataDmd(dataDmd);
    graph_[id] = node;
    numNode++;
    maxNode = std::max(maxNode, id);
  }

  /** Add an edge from src to dst.
      comCost: communication cost
  */
  void Graph::AddEdge(int src, int dst, int comCost){
    assert(src >= 0);
    assert(dst >= 0);

    graphmap::iterator itSrc, itDst;
    itSrc = graph_.find(src);
    itDst = graph_.find(dst);
    assert(itSrc != graph_.end());
    assert(itDst != graph_.end());

    itSrc->second->AddOutput(dst);
    itDst->second->AddInput(src);

    // Record the communication cost
    comCostMap[std::make_pair(src, dst)] = comCost;

    numEdge++;
  }

  /** Get a node pointer according to the given node id.
   */
  Node* Graph::GetNode(int id){
    assert(id >= 0);
    graphmap::iterator it;
    it = graph_.find(id);
    assert(it != graph_.end()); //Error: cannot find node

    return it->second;
  }

  /** Return the total number of edges.
   */
  int Graph::Edges(){
    return numEdge;
  }

  /** Return the total number of nodes.
   */
  int Graph::Nodes(){
    return numNode;
  }

  /** Get communication cost between two nodes.
      Return -1 if there's no weight configure
      of the edge; return the cost otherwise.
  */
  int Graph::GetComCost(int src, int dst){
    // TODO: Need to check if the key exists?
    int cc = comCostMap[std::make_pair(src, dst)];

    return cc;
  }

  /** Calculate Out-degree Communication Cost Weight (OCCW) of a graph node.
   */
  float Graph::OCCW(int ndId){
    Node* nd = GetNode(ndId);

    float occw = 0;
    for (auto& it : nd->output){
      if ( GetComCost(ndId, it) >= 0 ){
	occw += GetComCost(ndId, it);
      }else{
	Node* ndOut = GetNode(it);
	float total_output = 0;
	float data_trans_ratio = 1.0;
	for (auto& yait : ndOut->input) {
	  total_output += GetNode(yait)->GetDataDmd();
	}
	if (total_output > ndOut->GetDataDmd()){
	  data_trans_ratio = ndOut->GetDataDmd() / total_output;
	}
	occw += nd->GetDataDmd() * data_trans_ratio;
      }
    }

    return occw;
  }

  /** Calculate all the OCCWs of all the nodes.
   */
  void Graph::InitAllOCCW(){
    for (auto& it : graph_) {
      it.second->SetOCCW(OCCW(it.first));
    }
  }

  /** Return the max node id in the graph.
      For OCT matrix construction.
  */
  int Graph::MaxNodeId(){
    return maxNode;
  }

  /** Clean up the graph, destory
      everything in it.
  */
  void Graph::Clear(){
    graphmap::iterator it = graph_.begin();
    for (; it != graph_.end(); it++){
      //std::cout<<"Deleting "<<it->second->GetId()<<std::endl;
      delete it->second;
    }
    graph_.clear();
    numEdge = 0;
    numNode = 0;
  }
}
