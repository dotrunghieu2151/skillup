#include "Core/Queue/MpmcQueue.hpp"
#include <gtest/gtest.h>

class McmpQueueTest : public ::testing::Test {
public:
protected:
  void SetUp() override {};
};

TEST_F(McmpQueueTest, SingleThread) {}
