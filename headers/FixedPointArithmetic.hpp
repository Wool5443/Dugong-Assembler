#ifndef FIXED_POINT_ARITHMETIC_HPP
#define FIXED_POINT_ARITHMETIC_HPP

#include "Stack.hpp"

static const unsigned int DIGITS_AFTER_POINT = 3;

static const unsigned int _getNumberMultiplier()
{
   unsigned int multiplier = 1;
   for (unsigned int i = 1; i < DIGITS_AFTER_POINT; i++)
      multiplier *= 10;
   return multiplier;
}

static const unsigned int NUMBER_MULTIPLIER = _getNumberMultiplier();

StackElementOption FixedPointAdd(StackElement_t a, StackElement_t b);

StackElementOption FixedPointSubtract(StackElement_t a, StackElement_t b);

StackElementOption FixedPointMultiply(StackElement_t a, StackElement_t b);

StackElementOption FixedPointDivide(StackElement_t a, StackElement_t b);

StackElementOption FixedPointSin(StackElement_t a);

StackElementOption FixedPointCos(StackElement_t a);

#endif
