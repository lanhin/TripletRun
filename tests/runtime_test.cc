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

  EXPECT_FLOAT_EQ(rt.CalcWeightMeanSD(0), 0.47140449);
  EXPECT_FLOAT_EQ(rt.CalcWeightMeanSD(0), 0.47140449);
  EXPECT_FLOAT_EQ(rt.CalcWeightMeanSD(8), 0.23570225);
}
