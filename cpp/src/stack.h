/* 
 * File:   Stack.h
 * Author: AlexisBlaze
 *
 * Created on January 12, 2011, 1:31 PM
 */

#ifndef _CHIP8_LIB_STACK_H
#define	_CHIP8_LIB_STACK_H

class Stack {
  public:
    static const int CAPACITY = 16;

  private:
    short _values[CAPACITY];
    short *_tos;
    short *_limit;

  public:
    Stack() {
      _tos = _values;
      _limit = (_tos + CAPACITY);
    }

    bool push(short val) {
      if(overflow()) return false;

      *_tos++ = val;
      return true;
    }

    short pop() {
      if(underflow()) return NULL;

      return *(--_tos);
    }

    short peek() {
      if(underflow()) return NULL;
      return *(_tos-1);
    }

  private:
    bool overflow() {
      return (_tos == _limit);
    }

    bool underflow() {
      return (_tos == _values);
    }
};

#endif	/* _STACK_H */
