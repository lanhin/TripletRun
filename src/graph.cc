// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include "graph.h"
#include <cassert>

namespace triplet{
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
}
