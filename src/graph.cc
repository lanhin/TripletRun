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
    :id_(-1), output_data_size(-1.0), rank_OCT(0){}
  Node::Node(int id, float compDmd, float dataDmd, float output){
    id_ = id;
    computing_demand = compDmd;
    data_demand = dataDmd;
    output_data_size = output;
    rank_OCT = 0;
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

  /** Set the output data size of the node.
      This is unused at present.
  */
  void Node::SetOutputSize(float output){
    assert(output >= 0.0);
    output_data_size = output;
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
  void Node::SetRank(float rank){
    rank_OCT = rank;
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

  /** Get output size of the node.
      This is not used at present.
  */
  float Node::GetOutputSize(){
    if(output_data_size < ZERO_POSITIVE)
      return data_demand;
    else
      return output_data_size;
  }

  /** Get rank_OCT of the node,
      used in PEFT policy.
  */
  float Node::GetRank(){
    return rank_OCT;
  }

  /** Get actual finish time (AFT) of the node.
   */
  float Node::GetAFT(){
    assert(this->AFT >= ZERO_NEGATIVE);
    return this->AFT;
  }

  // class graph
  Graph::Graph(){
    numEdge = 0;
    numNode = 0;
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
