//! @file

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define TESTINGs

/** @enum Color
 * @brief Represents colors for @see SetConsoleColor
 */
enum Color { RED, GREEN, WHITE };

/** @enum ErrorCode
 * @brief Represents possible error codes for @see MyAssertHard()
 */
enum ErrorCode 
{
  EVERYTHING_FINE = 0, ERROR_NULLPTR, ERROR_BAD_NUMBER, ERROR_BAD_FILE, ERROR_OVERLAP,
  ERROR_INDEX_OUT_OF_BOUNDS, ERROR_NO_MEMORY, ERROR_NO_COMPARATOR, ERROR_BAD_SIZE,
  ERROR_BAD_VALUE, ERROR_DEAD_CANARY, ERROR_BAD_HASH, ERROR_ZERO_DIVISION,
  ERROR_SYNTAX
};

static const char* ERROR_CODE_NAMES[] =
{
  "EVERYTHING_FINE", "ERROR_NULLPTR", "ERROR_BAD_NUMBER", "ERROR_BAD_FILE", "ERROR_OVERLAP",
  "ERROR_INDEX_OUT_OF_BOUNDS", "ERROR_NO_MEMORY", "ERROR_NO_COMPARATOR", "ERROR_BAD_SIZE",
  "ERROR_BAD_VALUE", "ERROR_DEAD_CANARY", "ERROR_BAD_HASH", "ERROR_ZERO_DIVISION",
  "ERROR_SYNTAX"
};

#define RETURN_ERROR(error)                                                                     \
do                                                                                              \
{                                                                                               \
   __typeof__(error) _error = error;                                                            \
   if (_error)                                                                                  \
      return _error;                                                                            \
} while (0);

#ifdef DEBUG
    #define ON_DEBUG(...) __VA_ARGS__
#else
    #define ON_DEBUG(...)
#endif

/**
 * @brief typedef for compare functions used in universal quicksort.
*/
typedef int CompareFunction_t(const void* a, const void* b);

/**
 * @brief Hard assert which tells the file, function and line where the error occurred.
 *
 * @param [in] STATEMENT - the condition to check.
 * @param [in] ERR_CODE - what can happen @see ErrorCode.
 * @param [in] EXIT_CMD - operation to perform before exiting the program.
 *
 * @note If there is nothing to perform pass nothing.
 */
#define MyAssertHard(STATEMENT, ERR_CODE, ...)                                                                      \
if (!(STATEMENT))                                                                                                   \
do {                                                                                                                \
    SetConsoleColor(stderr, RED);                                                                                   \
    fprintf(stderr, "%s in %s in %s in line: %d\n", #ERR_CODE, __FILE__, __PRETTY_FUNCTION__, __LINE__);            \
    SetConsoleColor(stderr, WHITE);                                                                                 \
    __VA_ARGS__;                                                                                                    \
    exit(ERR_CODE);                                                                                                 \
} while(0);

/**
 * @brief Transforms a given name into a string.
 * 
 * @param [in] VALUE - the thing to transform.
*/
#define ValueToString(VALUE) #VALUE

/**
 * @brief Soft assert which tells the file, function and line where the error occurred.
 *
 * @param [in] STATEMENT - the condition to check.
 * @param [in] ERR_CODE - what can happen @see ErrorCode.
 * @param [in] EXIT_CMD - operation to perform before exiting the program.
 *
 * @note If there is nothing to perform pass nothing.
 */
#define MyAssertSoft(STATEMENT, ERR_CODE, ...)                                                                      \
if (!(STATEMENT))                                                                                                   \
do {                                                                                                                \
    SetConsoleColor(stderr, RED);                                                                                   \
    fprintf(stderr, "%s in %s in %s in line: %d\n", #ERR_CODE, __FILE__, __PRETTY_FUNCTION__, __LINE__);            \
    SetConsoleColor(stderr, WHITE);                                                                                 \
    __VA_ARGS__;                                                                                                    \
    return ERR_CODE;                                                                                                \
} while(0);

/**
 * @brief Finds max of x, y.
 */
#define max(x, y)                                                                                                   \
({                                                                                                                  \
    __typeof__(x) _tx = x; __typeof__(y) _ty = y;                                                                   \
    _tx > _ty ? _tx : _ty;                                                                                          \
})

/**
 * @brief Finds min of x, y.
 */
#define min(x, y)                                                                                                   \
({                                                                                                                  \
    __typeof__(x) _tx = x; __typeof__(y) _ty = y;                                                                   \
    _tx < _ty ? _tx : _ty;                                                                                          \
})                                                                                                                  \

/**
 * @brief Struct to contain where some variable was created.
 * 
 * @var Owner::fileName - the file where the variable was created.
 * @var Owner::line - the line where it was created.
 * @var Owner::name - the name of the variable.
*/
struct Owner
{
    const char* fileName;
    size_t line;
    const char* name;
};

/**
 * @brief Tells if 2 doubles are equal.
 *
 * @param x1
 * @param x2
 *
 * @return true The numbers are equal.
 * @return false The numbers are not equal.
 */
bool IsEqual(const double x1, const double x2);

/**
 * @brief Copies <length> element from source to destination starting at zero index.
 *
 * @param destination
 * @param source
 * @param length
 */
void CopyArray(double destination[], double source[], int length);

/**
 * @brief Finds min of the given array of any type.
 * 
 * @param [in] data - the array to find min in.
 * @param [in] elementCount - length of the array.
 * @param [in] elementSize - size in bytes of the elements.
 * @param [in] compareFunction - @see CompareFunction_t - the comparator.
 * 
 * @return const void* the the smallest element.
*/
const void* MinArray(const void* data, size_t elementCount, size_t elementSize, CompareFunction_t* compareFunction);

/**
 * @brief Finds max of the given array of any type.
 * 
 * @param [in] data - the array to find max in.
 * @param [in] elementCount - length of the array.
 * @param [in] elementSize - size in bytes of the elements.
 * @param [in] compareFunction - @see CompareFunction_t - the comparator.
 * 
 * @return const void* the the largest element.
*/
const void* MaxArray(const void* data, size_t elementCount, size_t elementSize, CompareFunction_t* compareFunction);

/**
 * @brief Swaps the raw bytes of a and b.
 * 
 * @param [in] a - pointer to the 1st object.
 * @param [in] b - pointer to the 2nd object.
 * @param [in] size - size of the objects.
*/
void Swap(void* a, void* b, size_t size);

/**
 * @brief Sorts the given array according to the Compare Function.
 * Uses randomized quick sort.
 * 
 * @param [in] data - the array to sort.
 * @param [in] elementCount - length of the array.
 * @param [in] elementSize - size in bytes of the array elements.
 * @param [in] compareFunction - @see CompareFunction_t
*/
void Sort(void* data, size_t elementCount, size_t elementSize, CompareFunction_t* compareFunction);

/**
 * @brief Clears stdin.
 */
void ClearBuffer(void);

/**
 * @brief Check if the user input contains anything but scanned data.
 *
 * @return true Everything is clear.
 * @return false User entered something odd.
 */
bool CheckInput(void);

/**
 * @brief Set the color of either stderr or stdout
 *
 * @param place - stderr or stdout
 * @param color - @see Color
 */
void SetConsoleColor(FILE* place, const enum Color color);

unsigned int CalculateHash(const void *key, size_t len, unsigned int seed);

#endif