// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "graph.h"
#include <cassert>

namespace triplet{
  // class Node
  Node::Node()
    :id_(-1){}
  Node::Node(int id, float compDmd, float dataDmd){
    id_ = id;
    computing_demand = compDmd;
    data_demand = dataDmd;
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

  void Node::SetCompDmd(float demand){
    assert(demand > 0.0);
    computing_demand = demand;
  }

  void Node::SetDataDmd(float demand){
    assert(demand > 0.0);
    data_demand = demand;
  }

  void Node::AddInput(int inNode){
    assert(inNode >= 0);
    input.insert(inNode);
  }

  void Node::AddOutput(int outNode){
    assert(outNode >= 0);
    output.insert(outNode);
  }

  int Node::GetId(){
    return id_;
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

  // class graph
  Graph::Graph(){}

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
    
    Node node;
    node.SetId(id);
    graph_[id] = node;
  }

  void Graph::AddNode(int id, float comDmd, float dataDmd){
    assert(id >= 0);
    graphmap::iterator it;
    it = graph_.find(id);
    assert(it == graph_.end()); //Error: the node has already created
    
    Node node;
    node.SetId(id);
    node.SetCompDmd(comDmd);
    node.SetDataDmd(dataDmd);
    graph_[id] = node;
  }

  void Graph::AddEdge(int src, int dst){
    assert(src >= 0);
    assert(dst >= 0);

    graphmap::iterator itSrc, itDst;
    itSrc = graph_.find(src);
    itDst = graph_.find(dst);
    assert(itSrc != graph_.end());
    assert(itDst != graph_.end());

    itSrc->second.AddOutput(dst);
    itDst->second.AddInput(src);
  }

  Node Graph::GetNode(int id){
    assert(id >= 0);
    graphmap::iterator it;
    it = graph_.find(id);
    assert(it != graph_.end()); //Error: cannot find node

    return it->second;
  }
}
