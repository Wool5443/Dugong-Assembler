#include <math.h>
#include "FixedPointArithmetic.hpp"

StackElementOption FixedPointAdd(StackElement_t a, StackElement_t b)
{
    return {a + b, EVERYTHING_FINE};
}

StackElementOption FixedPointSubtract(StackElement_t a, StackElement_t b)
{
    return {a - b, EVERYTHING_FINE};
}

StackElementOption FixedPointMultiply(StackElement_t a, StackElement_t b)
{
    return {a * b / NUMBER_MULTIPLIER, EVERYTHING_FINE};
}

StackElementOption FixedPointDivide(StackElement_t a, StackElement_t b)
{
    if (b == 0)
        return {POISON, ERROR_ZERO_DIVISION};

    return {(StackElement_t)(a * NUMBER_MULTIPLIER / b), EVERYTHING_FINE};
}

StackElementOption FixedPointSin(StackElement_t a)
{
    double result = sin((double)a / NUMBER_MULTIPLIER);

    return {(StackElement_t)(result * NUMBER_MULTIPLIER), EVERYTHING_FINE};
}

StackElementOption FixedPointCos(StackElement_t a)
{
    double result = cos((double)a / NUMBER_MULTIPLIER);

    return {(StackElement_t)(result * NUMBER_MULTIPLIER), EVERYTHING_FINE};
}
