// ==================
// @2017-09 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#ifndef TRIPLET_GRAPH_H_
#define TRIPLET_GRAPH_H_

#include <set>

namespace triplet{
  class Node{
  public:
    Node();
    Node(int id, float compDmd, float dataDmd);
    ~Node();

    typedef std::set<int> nodeset;

    void SetId(int id);
    void SetCompDmd(float demand);
    void SetDataDmd(float demand);
    void AddInput(int inNode);
    void AddOutput(int outNode);
    
    int GetId();
    float GetCompDmd();
    float GetDataDmd();
    int GetInNum();
    int GetOutNum();
    
    int id_;
    float computing_demand;
    float data_demand;
    nodeset input;
    nodeset output;
  };

  class Graph{
  };
}

#endif //TRIPLET_GRAPH_H_
