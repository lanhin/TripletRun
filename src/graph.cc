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
    :status(INIT), id_(-1), level(-1), rank_OCT(0),rank_u_HSIP(-1), rank_u_HEFT(-1), mean_weight(-1.0), wait_time(0.0), cpath_cc(0.0), NDON(0.0), rank_d_CPOP(-1), priority_CPOP(0), rank_ADON(-1) {}
  Node::Node(int id, float compDmd, float dataDmd, float dataConsume, float dataGenerate){
    status = INIT;
    id_ = id;
    level = -1;
    computing_demand = compDmd;
    data_demand = dataDmd;
    data_consume = dataConsume;
    data_generate = dataGenerate;
    rank_OCT = 0;
    rank_u_HSIP = -1;
    rank_u_HEFT = -1;
    rank_d_CPOP = -1;
    mean_weight = -1.0;
    wait_time = 0.0;
    cpath_cc = 0.0;
    NDON = 0.0;
    rank_ADON = -1;
    priority_CPOP = 0.0;
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
  void Node::SetRank_u_HSIP(float rank){
    rank_u_HSIP = rank;
  }

  /** Set rank_u of the node,
      which is used in HEFT policy.
  */
  void Node::SetRank_u_HEFT(float rank){
    rank_u_HEFT = rank;
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

  /** Get data consume of the node.
   */
  float Node::GetDataConsume(){
    return this->data_consume;
  }

  /** Get data generate of the node.
   */
  float Node::GetDataGenerate(){
    return this->data_generate;
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
  float Node::GetRank_u_HSIP(){
    return rank_u_HSIP;
  }

  /** Get rank_u of the node,
      used in HEFT policy.
  */
  float Node::GetRank_u_HEFT(){
    return rank_u_HEFT;
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

  /** Set waiting time of the node.
   */
  void Node::SetWaitTime(float time){
    this->wait_time = time;
  }

  /** Get waiting time of the node.
   */
  float Node::GetWaitTime(){
    return this->wait_time;
  }

  /** Get the critical path computation cost value of this node.
   */
  float Node::GetCpathCC(){
    return this->cpath_cc;
  }

  /** Set the critical path computation cost value of this node.
   */
  void Node::SetCpathCC(float cc){
    assert(cc >= ZERO_NEGATIVE);
    this->cpath_cc = cc;
  }

  /** Get the rank_d used in CPOP policy.
   */
  float Node::GetRank_d_CPOP(){
    return this->rank_d_CPOP;
  }

  /** Set the rank_d used in CPOP policy.
   */
  void Node::SetRank_d_CPOP(float rankd){
    assert(rankd > ZERO_NEGATIVE);
    this->rank_d_CPOP = rankd;
  }


  /** Set the priority of CPOP policy.
   */
  void Node::SetPriorityCPOP(float priority){
    assert(priority > ZERO_NEGATIVE);
    this->priority_CPOP = priority;
  }

  /** Get the priority of CPOP policy.
   */
  float Node::GetPriorityCPOP(){
    return this->priority_CPOP;
  }

  /** Set NDON value.
   */
  void Node::SetNDON(float degree){
    assert(degree >= 0.0);
    this->NDON = degree;
  }

  /** Get NDON value.
   */
  float Node::GetNDON(){
    return this->NDON;
  }


  /** Set rank_ADON value.
   */
  void Node::SetRank_ADON(float degree){
    assert(degree >= 0.0);
    this->rank_ADON = degree;
  }

  /** Get rank_ADON value.
   */
  float Node::GetRank_ADON(){
    return this->rank_ADON;
  }

  /** Get the node's level
   */
  int Node::GetLevel(){
    return this->level;
  }

  /** Set the node's level
   */
  void Node::SetLevel(int lvl){
    assert(lvl >= 0);
    if(this->level < 0){//init state
      this->level = lvl;
    }else{
      this->level = std::min(lvl, this->level);
    }
  }

  /** Set node status.
   */
  void Node::SetStatus(NodeStatus st){
    this->status = st;
  }

  /** Get node status.
      Only for debug at present.
   */
  NodeStatus Node::GetStatus(){
    return this->status;
  }

  /** Check if the node is in the ready queue.
   */
  bool Node::IsReady(){
    //std::cout<<"Status:"<<this->status<<" is ready:"<<(this->status == READY)<<std::endl;
    return (this->status == READY);
  }


  // class graph
  Graph::Graph(){
    numEdge = 0;
    numNode = 0;
    maxNode = -1;
    sourceId = -1;
    sinkId = -1;

    total_computation_cost = 0.0;
    total_memory_cost = 0.0;
    total_edge_weight = 0.0;
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
  void Graph::AddNode(int id, float comDmd, float dataDmd, float dataConsume, float dataGenerate){
    assert(id >= 0);
    assert(comDmd > 0.0);
    graphmap::iterator it;
    it = graph_.find(id);
    assert(it == graph_.end()); //Error: the node has already created
    
    Node * node = new Node(id, comDmd, dataDmd, dataConsume, dataGenerate);
    //node->SetId(id);
    //node->SetCompDmd(comDmd);
    //node->SetDataDmd(dataDmd);
    graph_[id] = node;
    numNode++;
    maxNode = std::max(maxNode, id);

    // Record the total computation and memory cost
    total_computation_cost += comDmd;
    total_memory_cost += dataDmd;
  }

  /** Add an edge from src to dst.
      comCost: communication cost
  */
  void Graph::AddEdge(int src, int dst, float comCost){
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

    if (comCost >= ZERO_NEGATIVE){
      total_edge_weight += comCost;
    }

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
	  total_output += std::max(GetNode(yait)->GetDataDmd(), (float)0.0);
	}
	if (total_output > ndOut->GetDataDmd()){
	  data_trans_ratio = std::max(ndOut->GetDataDmd(), (float)0.0) / total_output;
	}
	occw += std::max(nd->GetDataDmd(), (float)0.0) * data_trans_ratio;
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

  /** Calculate the priority value used in CPOP policy.
      Return the max priority value, which is the absCP in CPOP.
   */
  float Graph::CalcPriorityCPOP(){
    float tmpPriority, maxPriority = 0;
    for (auto& it : graph_) {
      tmpPriority = it.second->GetRank_u_HEFT() + it.second->GetRank_d_CPOP();
      it.second->SetPriorityCPOP(tmpPriority);
      maxPriority = std::max(maxPriority, tmpPriority);
    }

    return maxPriority;
  }


  /** Return the max node id in the graph.
      For OCT matrix construction.
  */
  int Graph::MaxNodeId(){
    return maxNode;
  }

  /** Calculate the max device compute power of node ndId.
   */
  void Graph::CalcCpathCC(int ndId, float max_devCompute, float min_execution_time){
    assert(max_devCompute > 0.0);
    Node* nd = GetNode(ndId);

    float max_cc = 0.0;
    for (auto& it : nd->input) {
      max_cc = std::max(max_cc, GetNode(it)->GetCpathCC());
    }
    max_cc += std::max(nd->GetCompDmd() / max_devCompute, min_execution_time);

    nd->SetCpathCC(max_cc);
  }

  /** Get total computation cost.
   */
  float Graph::GetTotalCost(){
    return this->total_computation_cost;
  }

  /** Set source vertex id.
   */
  void Graph::SetSourceId(int source){
    assert(source >= 0);
    this->sourceId = source;
  }

  /** Get source vertex id.
   */
  int Graph::GetSourceId(){
    return this->sourceId;
  }

  /** Set sink vertex id.
   */
  void Graph::SetSinkId(int sink){
    assert(sink >= 0);
    this->sinkId = sink;
  }

  /** Get sink vertex id.
   */
  int Graph::GetSinkId(){
    return this->sinkId;
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

  /** Report the 3 summary value.
   */
  void Graph::SummaryReport(){
    std::cout<<"-------- Graph information --------"<<std::endl;
    std::cout<<" Total computation cost: "<<this->total_computation_cost<<std::endl;
    std::cout<<" Total memory cost: "<<this->total_memory_cost<<std::endl;
    std::cout<<" Total communication cost: "<<this->total_edge_weight<<std::endl;
    std::cout<<" Total nodes: "<<this->numNode<<std::endl;
    std::cout<<" Total edges: "<<this->numEdge<<std::endl;
    std::cout<<" Max node id: "<<this->maxNode<<std::endl;
    std::cout<<"-----------------------------------"<<std::endl;
  }

}
