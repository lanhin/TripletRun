// ==================
// @2018-01 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

#include <gtest/gtest.h>

#include "src/graph.h"

TEST(GraphTest, OCCW){
  triplet::Graph* graph = new triplet::Graph();

  // 1st level
  graph->AddNode(0, 20, 20);

  // 2nd level
  graph->AddNode(1, 15, 15);
  graph->AddNode(2, 10, 10);

  graph->AddEdge(0, 1, 5);
  graph->AddEdge(0, 2);

  // 3rd level
  graph->AddNode(3, 10, 10);
  graph->AddNode(4, 5, 5);
  graph->AddNode(5, 10, 10);

  graph->AddEdge(1, 3, 8);
  graph->AddEdge(1, 4);
  graph->AddEdge(2, 4, 5);
  graph->AddEdge(2, 5, 5);

  // 4th level
  graph->AddNode(8, 10, 10);

  graph->AddEdge(3, 8);
  graph->AddEdge(4, 8, 5);
  graph->AddEdge(5, 8);

  EXPECT_EQ(graph->Edges(), 9);
  EXPECT_EQ(graph->Nodes(), 7);

  graph->InitAllOCCW();

  EXPECT_FLOAT_EQ(graph->GetNode(0)->GetOCCW(), 15);
  EXPECT_FLOAT_EQ(graph->GetNode(1)->GetOCCW(), 11);
  EXPECT_FLOAT_EQ(graph->GetNode(2)->GetOCCW(), 10);
  EXPECT_FLOAT_EQ(graph->GetNode(3)->GetOCCW(), 4);
  EXPECT_FLOAT_EQ(graph->GetNode(4)->GetOCCW(), 5);
  EXPECT_FLOAT_EQ(graph->GetNode(5)->GetOCCW(), 4);
  EXPECT_FLOAT_EQ(graph->GetNode(8)->GetOCCW(), 0);
}
