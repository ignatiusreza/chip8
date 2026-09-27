/* 
 * File:   Stack.h
 * Author: AlexisBlaze
 *
 * Created on January 12, 2011, 1:31 PM
 */

#ifndef _CHIP8_CORE_STACK_H
#define	_CHIP8_CORE_STACK_H

class Stack {
  public:
    static constexpr int CAPACITY = 16;

  private:
    // an index rather than a pointer into _values, so a copied Stack stays valid
    short _values[CAPACITY];
    int _size;

  public:
    Stack() : _size(0) {}

    bool push(short val) {
      if(overflow()) return false;

      _values[_size++] = val;
      return true;
    }

    short pop() {
      if(underflow()) return 0;

      return _values[--_size];
    }

    short peek() const {
      if(underflow()) return 0;
      return _values[_size-1];
    }

    int size() const { return _size; }
    bool empty() const { return underflow(); }

  private:
    bool overflow() const {
      return (_size == CAPACITY);
    }

    bool underflow() const {
      return (_size == 0);
    }
};

#endif	/* _CHIP8_CORE_STACK_H */
