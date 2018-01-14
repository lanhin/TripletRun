#include <gtest/gtest.h>

#include "src/device.h"

TEST(DeviceTest, ITS){
  triplet::Device* dev = new triplet::Device(0, 10, 10, 10, 0);

  dev->NewSlot(0.0, 1.02311);

  dev->NewSlot(3.012, 3.407);

  dev->ShowSlot();

  EXPECT_EQ(dev->FindSlot(0.02, 1.0), 0.0);

  dev->UpdateSlot(0.2, 0.1);
  dev->ShowSlot();

  dev->UpdateSlot(0.25, 0.25);
  dev->ShowSlot();

  dev->UpdateSlot(0.801, 1.1, 0.3);
  dev->ShowSlot();

  dev->UpdateSlot(0.5, 2.6);
  dev->ShowSlot();

  dev->NewSlot(4.127, 4.80736);
  dev->ShowSlot();

  float ts = dev->FindSlot(4.3, 0.5);
  EXPECT_FLOAT_EQ(4.127, ts);

  dev->UpdateSlot(std::max(ts, (float)4.3), 0.5);
  dev->ShowSlot();
}
