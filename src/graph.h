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

  protected:
    int id_;
    float computing_demand;
    float data_demand;
    nodeset input;
    nodeset output;
  };

  class Graph{
  public:
    Graph();
    ~Graph();

    void AddNode(int id);
    void AddNode(int id, float comDmd, float dataDmd);
    void AddEdge(int src, int dst);
    Node GetNode(int id);

    typedef std::map<int,Node> graphmap;
  protected:
    graphmap graph_;
    int numEdge;
    int numNode;
  };
}

#endif //TRIPLET_GRAPH_H_
