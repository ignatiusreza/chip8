/* 
 * File:   stack_test.cc
 * Author: AlexisBlaze
 *
 * Created on January 12, 2011, 11:59 PM
 */

#include <gtest/gtest.h>
#include "stack.h"

// The fixture for testing class Stack.
class StackTest : public ::testing::Test {
  protected:
    Stack s1;
};

// Tests that popping stack will not underflow.
TEST_F(StackTest, UnderflowTest) {
  EXPECT_EQ(0, s1.pop());
}

// Tests that pushing stack pass the CAPACITY will not overflow.
TEST_F(StackTest, OverflowTest) {
  int i = 0;

  for(; i < Stack::CAPACITY; i++)
    ASSERT_TRUE(s1.push(i));
  EXPECT_FALSE(s1.push(i));
}

// Tests that popping pushed value on stack return correct value.
TEST_F(StackTest, PushPopTest) {
  int i = 0;

  for(; i < Stack::CAPACITY; i++)
    ASSERT_TRUE(s1.push(i));
  ASSERT_FALSE(s1.push(i)); // overflow??

  for(i--; i >= 0; i--)
    ASSERT_EQ(i, s1.pop());
  ASSERT_EQ(0, s1.pop()); // underflow??
}

// Tests that a copied stack holds its own values.
TEST_F(StackTest, CopyTest) {
  ASSERT_TRUE(s1.push(1));
  Stack s2 = s1;
  ASSERT_TRUE(s2.push(2));

  EXPECT_EQ(1, s1.size());
  EXPECT_EQ(2, s2.size());
  EXPECT_EQ(2, s2.pop());
  EXPECT_EQ(1, s2.pop());
  EXPECT_EQ(1, s1.pop());
}

// Tests that stack can store large enough value.
TEST_F(StackTest, MaxValueTest) {
  int i = 0;

  ASSERT_TRUE(s1.push(0xfff));
  ASSERT_EQ(0xfff, s1.pop());
}
