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

  void Node::SetId(int id){
    assert(id >= 0);
    id_ = id;
  }

  void Node::SetOccupied(int id){
    assert(id >= 0);
    occupied_device = id;
  }

  void Node::SetCompDmd(float demand){
    assert(demand > 0.0);
    computing_demand = demand;
  }

  void Node::SetDataDmd(float demand){
    assert(demand > 0.0);
    data_demand = demand;
  }

  void Node::SetOutputSize(float output){
    assert(output >= 0.0);
    output_data_size = output;
  }

  void Node::AddInput(int inNode){
    assert(inNode >= 0);
    input.insert(inNode);
  }

  void Node::AddOutput(int outNode){
    assert(outNode >= 0);
    output.insert(outNode);
  }

  void Node::SetRank(float rank){
    rank_OCT = rank;
  }

  void Node::SetAFT(float aft){
    assert(aft >= ZERO_NEGATIVE);

    this->AFT = aft;
  }

  int Node::GetId(){
    return id_;
  }

  int Node::GetOccupied(){
    return occupied_device;
  }

  float Node::GetCompDmd(){
    return computing_demand;
  }

  float Node::GetDataDmd(){
    return data_demand;
  }

  int Node::GetInNum(){
    return input.size();
  }

  int Node::GetOutNum(){
    return output.size();
  }

  float Node::GetOutputSize(){
    if(output_data_size < ZERO_POSITIVE)
      return data_demand;
    else
      return output_data_size;
  }

  float Node::GetRank(){
    return rank_OCT;
  }

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

  Node* Graph::GetNode(int id){
    assert(id >= 0);
    graphmap::iterator it;
    it = graph_.find(id);
    assert(it != graph_.end()); //Error: cannot find node

    return it->second;
  }

  int Graph::Edges(){
    return numEdge;
  }

  int Graph::Nodes(){
    return numNode;
  }

  int Graph::GetComCost(int src, int dst){
    // TODO: Need to check if the key exists?
    int cc = comCostMap[std::make_pair(src, dst)];

    return cc;
  }

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
