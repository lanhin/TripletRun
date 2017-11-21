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
    Node(int id, float compDmd, float dataDmd, float output=-1.0);
    ~Node();

    typedef std::set<int> nodeset;

    void SetId(int id);
    void SetOccupied(int id);
    void SetCompDmd(float demand);
    void SetDataDmd(float demand);
    void SetOutputSize(float output);
    void AddInput(int inNode);
    void AddOutput(int outNode);
    
    int GetId();
    int GetOccupied();
    float GetCompDmd();
    float GetDataDmd();
    int GetInNum();
    int GetOutNum();
    float GetOutputSize();

    nodeset output;
    nodeset input;
    
  protected:
    int id_;
    int occupied_device; // the occupied device id
    float computing_demand;
    float data_demand;
    float output_data_size;
  };

  class Graph{
  public:
    Graph();
    ~Graph();

    void AddNode(int id);
    void AddNode(int id, float comDmd, float dataDmd);
    void AddEdge(int src, int dst);
    Node* GetNode(int id);
    int Edges();
    int Nodes();
    void Clear();

    typedef std::map<int,Node*> graphmap;
  protected:
    graphmap graph_;
    int numEdge;
    int numNode;
  };
}

#endif //TRIPLET_GRAPH_H_
