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

  int i;
  bool popSucc = queue.Pop(i);
}
