#include "Core/Queue/SpscQueue.hpp"
#include <gtest/gtest.h>

class ScspQueueTest : public ::testing::Test {
public:
  Core::SpscQueue<int, 10> queue{};

protected:
  void SetUp() override {};
};

TEST_F(ScspQueueTest, SingleThread) {
  bool pushSucc = queue.Push(1);

  EXPECT_EQ(pushSucc, true);

  int i;
  bool popSucc = queue.Pop(i);

  EXPECT_EQ(popSucc, true);
  EXPECT_EQ(i, 1);

  popSucc = queue.Pop(i);

  EXPECT_EQ(popSucc, false);
}
