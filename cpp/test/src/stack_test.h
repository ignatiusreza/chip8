/* 
 * File:   stack_test.h
 * Author: AlexisBlaze
 *
 * Created on January 12, 2011, 11:59 PM
 */

#ifndef _CHIP8_TEST_STACK_TEST_H
#define	_CHIP8_TEST_STACK_TEST_H

#include "../../src/stack.h"

// The fixture for testing class Stack.
class StackTest : public ::testing::Test {
  protected:
    Stack s1;
};

// Tests that popping stack will not underflow.
TEST_F(StackTest, UnderflowTest) {
  EXPECT_EQ(NULL, s1.pop());
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
  ASSERT_EQ(NULL, s1.pop()); // underflow??
}

// Tests that stack can store large enough value.
TEST_F(StackTest, MaxValueTest) {
  int i = 0;

  ASSERT_TRUE(s1.push(0xfff));
  ASSERT_EQ(0xfff, s1.pop());
}

#endif	/* _CHIP8_TEST_STACK_TEST_H */
