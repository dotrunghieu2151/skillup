#include "Core/Queue/MpmcQueue.hpp"
#include <gtest/gtest.h>

class McmpQueueTest : public ::testing::Test {
public:
  Core::McmpQueue<int, 10> queue{};

protected:
  void SetUp() override {};
};

TEST_F(McmpQueueTest, SingleThread) {
  bool pushSucc = queue.Push(1);

  EXPECT_EQ(pushSucc, true);

  int i;
  bool popSucc = queue.Pop(i);

  EXPECT_EQ(popSucc, true);
  EXPECT_EQ(i, 1);

  popSucc = queue.Pop(i);

  EXPECT_EQ(popSucc, false);
}
