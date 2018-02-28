// ==================
// @2018-01 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include <gtest/gtest.h>

#include "src/runtime.h"

TEST(RuntimeTest, BasicOperations){

  triplet::Runtime rt;
  rt.InitGraph("graph_test.json");
  rt.InitCluster("cluster_test.json");

  EXPECT_FLOAT_EQ(rt.CalcWeightMeanSD(0), 0.62853936);
  EXPECT_FLOAT_EQ(rt.CalcWeightMeanSD(0), 0.62853936); // sqrt(2) * 4 / 9
  EXPECT_FLOAT_EQ(rt.CalcWeightMeanSD(8), 0.15713484); // sqrt(2) / 9

  EXPECT_FLOAT_EQ(rt.CalcWeightMeanSD(1), 0.35355338); // sqrt(2) / 4
  EXPECT_FLOAT_EQ(rt.CalcWeightMeanSD(2), 0.15713484); // sqrt(2) / 9
  EXPECT_FLOAT_EQ(rt.CalcWeightMeanSD(3), 0.15713484); // sqrt(2) / 9
  EXPECT_FLOAT_EQ(rt.CalcWeightMeanSD(4), 0.039283708); // sqrt(2) / 36
  EXPECT_FLOAT_EQ(rt.CalcWeightMeanSD(5), 0.15713484); // sqrt(2) / 9

  triplet::Graph gr = rt.GetGraph();
  EXPECT_EQ(gr.Edges(), 9);

  rt.InitRuntime(triplet::HSIP);

  EXPECT_FLOAT_EQ(gr.GetNode(0)->GetRank_u_HSIP(), 32.1785113);
  EXPECT_FLOAT_EQ(gr.GetNode(1)->GetRank_u_HSIP(), 16.54997194);
  EXPECT_FLOAT_EQ(gr.GetNode(2)->GetRank_u_HSIP(), 15.35355339);
  EXPECT_FLOAT_EQ(gr.GetNode(3)->GetRank_u_HSIP(), 4.31426968);
  EXPECT_FLOAT_EQ(gr.GetNode(4)->GetRank_u_HSIP(), 5.19641855);
  EXPECT_FLOAT_EQ(gr.GetNode(5)->GetRank_u_HSIP(), 4.31426968);
  EXPECT_FLOAT_EQ(gr.GetNode(8)->GetRank_u_HSIP(), 0.15713484);

  rt.EntryTaskDuplication(gr.GetNode(0));

  EXPECT_EQ(gr.GetNode(0)->GetOccupied(), 0);

  triplet::Cluster sugon = rt.GetCluster();
  EXPECT_FLOAT_EQ(sugon[0]->GetAvaTime(), 1.25);
  EXPECT_FLOAT_EQ(sugon[1]->GetAvaTime(), 2.5);
  EXPECT_FLOAT_EQ(sugon[3]->GetAvaTime(), 1.6666667);

  triplet::Connections mellanox = rt.GetConnections();
  EXPECT_FLOAT_EQ(mellanox.GetMeanBW(), 12);

  std::map<int, float> eq = rt.GetExeQueue();
  EXPECT_EQ(eq.size(), 1);
  EXPECT_FLOAT_EQ(eq[0], 1.25);

  std::vector<int> rq = rt.GetReadyQueue();
  EXPECT_EQ(rq.size(), 0);

  EXPECT_FLOAT_EQ(rt.GetMeanCP(), 15.666667);
  rt.SetScheduler(triplet::DATACENTRIC);

  EXPECT_EQ(rt.DevicePick(0)->GetId(), 0);

  EXPECT_EQ(rt.DevicePick(2)->GetId(), 0);

  gr.GetNode(0)->SetOccupied(3);
  EXPECT_EQ(rt.DevicePick(2)->GetId(), 3);

  EXPECT_EQ(rt.DevicePick(1)->GetId(), 0);
}


TEST(RuntimeTest, MeanWaitTime){
  triplet::Runtime rt;
  rt.InitGraph("graph_test.json");
  rt.InitCluster("cluster_test.json");
  rt.InitRuntime(triplet::PEFT);
  rt.Execute();

  triplet::Graph gr = rt.GetGraph();
  EXPECT_FLOAT_EQ(gr.GetNode(0)->GetWaitTime(), 0);
  EXPECT_FLOAT_EQ(gr.GetNode(1)->GetWaitTime(), 0);
  EXPECT_FLOAT_EQ(gr.GetNode(2)->GetWaitTime(), 0.9375);
  EXPECT_FLOAT_EQ(gr.GetNode(3)->GetWaitTime(), 0.625);
  EXPECT_FLOAT_EQ(gr.GetNode(4)->GetWaitTime(), 0.3125);
  EXPECT_FLOAT_EQ(gr.GetNode(5)->GetWaitTime(), 0.625);
  EXPECT_FLOAT_EQ(gr.GetNode(8)->GetWaitTime(), 0.3125);

  EXPECT_EQ(rt.GetMaxParallel(), 3);
}

TEST(RuntimeTest, SchedulerCost){
  triplet::Runtime rt;
  rt.InitGraph("graph_test.json");
  rt.InitCluster("cluster_test.json");
  rt.InitRuntime(triplet::HSIP);
  rt.SetSchedulerCost(1.0);
  rt.Execute();
  
  triplet::Cluster sugon = rt.GetCluster();
  EXPECT_FLOAT_EQ(sugon[0]->GetAvaTime(), 6.875);
  EXPECT_FLOAT_EQ(sugon[1]->GetAvaTime(), 0);
  EXPECT_FLOAT_EQ(sugon[3]->GetAvaTime(), 0);
  

}
