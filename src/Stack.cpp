#include <time.h>
#include "Stack.hpp"

typedef unsigned int hash_t;

static const hash_t HASH_SEED = 0xBEBDA;

#define _STACK_DUMP_ERROR_DEBUG(filePath, stack, error)                                  \
do                                                                                       \
{                                                                                        \
    if (error && stack)                                                                  \
    {                                                                                    \
        Owner _caller = {__FILE__, __LINE__, __func__};                                  \                  
        FILE* _logFile = fopen(filePath, "a");                                           \
        if (_logFile)                                                                    \
            _stackDump(_logFile, stack, &_caller, error);                                \
        fclose(_logFile);                                                                \
    }                                                                                    \
} while (0);

#ifdef CANARY_PROTECTION
    static size_t _getRandomCanary()
    {
        srand((unsigned int)time(NULL));
        return ((canary_t)rand() << 32) + (canary_t)rand();
    }

    static const size_t _CANARY = _getRandomCanary();
#endif

struct Stack
{
    #ifdef CANARY_PROTECTION
    canary_t leftCanary;
    #endif

    size_t realDataSize;
    StackElement_t* data;

    Owner owner;
    size_t size;
    size_t capacity;

    #ifdef HASH_PROTECTION
    hash_t hashData;
    hash_t hashStack;
    #endif

    #ifdef CANARY_PROTECTION
    canary_t rightCanary;
    #endif
};

#ifdef CANARY_PROTECTION
static ErrorCode _checkCanary(const Stack* stack);

static canary_t* _getLeftDataCanaryPtr(const StackElement_t* data);

static canary_t* _getRightDataCanaryPtr(const StackElement_t* data, size_t realDataSize);
#endif

#ifdef HASH_PROTECTION
static ErrorCode _checkHash(Stack* stack);

static ErrorCode _reHashify(Stack* stack);

static hash_t _calculateDataHash(const Stack* stack);

static hash_t _calculateStackHash(Stack* stack);
#endif

static ErrorCode _stackRealloc(Stack* stack);

StackOption _stackInit(Owner owner)
{
    Stack* stack = (Stack*)calloc(1, sizeof(Stack));

    if (!stack)
        return {NULL, ERROR_NO_MEMORY};

    *stack = 
    {
        #ifdef CANARY_PROTECTION
            .leftCanary = _CANARY,
        #endif
        .owner = owner,
        .size = 0,
        .capacity = DEFAULT_CAPACITY
        #ifdef CANARY_PROTECTION
            ,.rightCanary = _CANARY
        #endif
    };

    size_t realDataSize = DEFAULT_CAPACITY * sizeof(StackElement_t);

    #ifdef CANARY_PROTECTION
    realDataSize += 2 * sizeof(canary_t);
    #endif

    StackElement_t* data = (StackElement_t*)calloc(realDataSize, 1);

    #ifdef CANARY_PROTECTION
    data = (StackElement_t*)((void*)data + sizeof(canary_t));
    #endif

    #ifdef CANARY_PROTECTION
    *_getLeftDataCanaryPtr(data) = _CANARY;

    *_getRightDataCanaryPtr(data, realDataSize) = _CANARY;
    #endif

    ErrorCode error = EVERYTHING_FINE;
    if (!data)
    {
        error = ERROR_NO_MEMORY;
        stack->capacity = 0;
        realDataSize = 0;
    }

    for (size_t i = 0; i < stack->capacity; i++)
        data[i] = POISON;
    
    stack->data = data;
    stack->realDataSize = realDataSize;

    #ifdef HASH_PROTECTION
    _reHashify(stack);
    #endif

    return {stack, error};
}

ErrorCode StackDestructor(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    ErrorCode error = CheckStackIntegrity(stack);

    _STACK_DUMP_ERROR_DEBUG(logFilePath, stack, error);
    RETURN_ERROR(error);

    if (stack->data == NULL)
        error = ERROR_NO_MEMORY;
    else
        #ifdef CANARY_PROTECTION
        free((void*)stack->data - sizeof(canary_t));
        #else
        free((void*)stack->data);
        #endif

    stack->size = POISON;
    stack->capacity = POISON;

    stack->data = NULL;

    stack->owner = {};

    #ifdef HASH_PROTECTION
        stack->hashData = POISON;
        stack->hashStack = POISON;
    #endif

    #ifdef CANARY_PROTECTION
        stack->leftCanary = POISON;
        stack->rightCanary = POISON;
    #endif
    
    free((void*)stack);

    return error;
}

ErrorCode CheckStackIntegrity(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    if (stack->capacity < stack->size)
        return ERROR_INDEX_OUT_OF_BOUNDS;
    if (!stack->data)
        return ERROR_NO_MEMORY;
    
    ErrorCode error = EVERYTHING_FINE;

    #ifdef CANARY_PROTECTION
        ErrorCode canaryError = _checkCanary(stack);
        _STACK_DUMP_ERROR_DEBUG(logFilePath, stack, canaryError);
        RETURN_ERROR(canaryError);
    #endif

    #ifdef HASH_PROTECTION
        error = _checkHash(stack);
        _STACK_DUMP_ERROR_DEBUG(logFilePath, stack, error);
        RETURN_ERROR(error);
    #endif
    
    return EVERYTHING_FINE;
}

ErrorCode _stackDump(FILE* where, Stack* stack, Owner* caller, ErrorCode error)
{
    fprintf(where,
            "Stack[%p] from %s(%zu) %s()\n"
            "called from %s(%zu) %s()\n"
            "Stack condition - %s\n"

            #ifdef HASH_PROTECTION
            "Data hash = %u (should be %u)\n"
            "Stack hash = %u (should be %u)\n"
            #endif

            #ifdef CANARY_PROTECTION
            "Left stack canary = %zu (should be %zu)\n"
            "Right stack canary = %zu (should be %zu)\n"
            #endif

            "{\n"
            "    size = %zu\n"
            "    capacity = %zu\n"
            "    data[%p]\n"

            #ifdef CANARY_PROTECTION
            "    Left data canary = %zu (should be %zu)\n"
            #endif
            ,
            stack, stack->owner.fileName, stack->owner.line, stack->owner.name,
            caller->fileName, caller->line, caller->name,
            ERROR_CODE_NAMES[error],

            #ifdef HASH_PROTECTION
            stack->hashData, _calculateDataHash(stack),
            stack->hashStack, _calculateStackHash(stack),
            #endif

            #ifdef CANARY_PROTECTION
            stack->leftCanary, _CANARY,
            stack->rightCanary, _CANARY,
            #endif

            stack->size,
            stack->capacity,
            stack->data

            #ifdef CANARY_PROTECTION
            , *_getLeftDataCanaryPtr(stack->data), _CANARY
            #endif
            );

    const StackElement_t* data = stack->data;
    size_t capacity = stack->capacity;

    for (size_t i = 0; i < capacity; i++)
    {
        fprintf(where, "    ");

        if (data[i] != POISON)
            fprintf(where, "*");
        else
            fprintf(where, " ");

        fprintf(where, "[%zu] = " STACK_EL_SPECIFIER "\n", i, data[i]);
        fflush(where);
    }


    #ifdef CANARY_PROTECTION
        fprintf(where, "    Right data canary = %zu (should be %zu)\n",
                *_getRightDataCanaryPtr(stack->data, stack->realDataSize), _CANARY);
                
        ErrorCode canaryError = _checkCanary(stack);

        _STACK_DUMP_ERROR_DEBUG(logFilePath, stack, canaryError);
        RETURN_ERROR(canaryError);
    #endif

    fprintf(where, "}\n\n\n");

    fflush(where);

    return EVERYTHING_FINE;
}

ErrorCode Push(Stack* stack, StackElement_t value)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    ErrorCode error = CheckStackIntegrity(stack);

    _STACK_DUMP_ERROR_DEBUG(logFilePath, stack, error);
    RETURN_ERROR(error);

    error = _stackRealloc(stack);

    _STACK_DUMP_ERROR_DEBUG(logFilePath, stack, error);
    RETURN_ERROR(error);

    stack->data[stack->size] = value;
    stack->size++;

    #ifdef CANARY_PROTECTION
        ErrorCode canaryError = _checkCanary(stack);
        _STACK_DUMP_ERROR_DEBUG(logFilePath, stack, canaryError);
        RETURN_ERROR(canaryError);
    #endif

    #ifdef HASH_PROTECTION
    _reHashify(stack);
    #endif

    return EVERYTHING_FINE;
}

StackElementOption Pop(Stack* stack)
{
    ErrorCode error = CheckStackIntegrity(stack);

    _STACK_DUMP_ERROR_DEBUG(logFilePath, stack, error);

    if (error)
        return {POISON, error};

    if (stack->size == 0)
        return {POISON, ERROR_INDEX_OUT_OF_BOUNDS};

    StackElement_t* data = stack->data;
    size_t size = stack->size - 1;

    StackElement_t value = data[size];
    
    data[size] = POISON;

    stack->size = size;

    error = _stackRealloc(stack);

    _STACK_DUMP_ERROR_DEBUG(logFilePath, stack, error);
    if (error)
        return {value, error};

    #ifdef CANARY_PROTECTION
        ErrorCode canaryError = _checkCanary(stack);
        _STACK_DUMP_ERROR_DEBUG(logFilePath, stack, canaryError);
        if (canaryError)
            return {POISON, canaryError};
    #endif

    #ifdef HASH_PROTECTION
    error = _reHashify(stack);
    #endif

    return {value, error};
}

static ErrorCode _stackRealloc(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    size_t newCapacity = 0;

    if (stack->size == stack->capacity)
        newCapacity = stack->capacity * STACK_GROW_FACTOR;
    else if (DEFAULT_CAPACITY < stack->capacity && stack->size <= stack->capacity / (STACK_GROW_FACTOR * STACK_GROW_FACTOR))
        newCapacity = stack->capacity / (STACK_GROW_FACTOR * STACK_GROW_FACTOR);

    if (newCapacity != 0)
    {
        StackElement_t* oldData = stack->data;
        size_t newDataSize = newCapacity * sizeof(StackElement_t);

        #ifdef CANARY_PROTECTION
            oldData = (StackElement_t*)((void*)oldData - sizeof(canary_t));
            newDataSize += 2 * sizeof(canary_t);

            canary_t* oldRightCanaryPtr = _getRightDataCanaryPtr(stack->data, stack->realDataSize);
            canary_t  oldRightCanary    = *oldRightCanaryPtr;

            *oldRightCanaryPtr = 0;
        #endif

        StackElement_t* newData = (StackElement_t*)realloc((void*)oldData, newDataSize);

        if (newData == NULL)
        {
            _STACK_DUMP_ERROR_DEBUG(logFilePath, stack, ERROR_NO_MEMORY);
            
            #ifdef CANARY_PROTECTION
            *oldRightCanaryPtr = oldRightCanary;
            #endif

            return ERROR_NO_MEMORY;
        }

        #ifdef CANARY_PROTECTION
        newData = (StackElement_t*)((void*)newData + sizeof(canary_t));
        #endif

        #ifdef CANARY_PROTECTION
        *_getRightDataCanaryPtr(newData, newDataSize) = oldRightCanary;
        #endif

        stack->data = newData;
        stack->realDataSize = newDataSize;
        stack->capacity = newCapacity;

        for (size_t i = stack->size; i < stack->capacity; i++)
            newData[i] = POISON;
    }

    #ifdef CANARY_PROTECTION
        ErrorCode canaryError = _checkCanary(stack);
        _STACK_DUMP_ERROR_DEBUG(logFilePath, stack, canaryError);
        RETURN_ERROR(canaryError);
    #endif

    return EVERYTHING_FINE;
}

#ifdef CANARY_PROTECTION
static ErrorCode _checkCanary(const Stack* stack)
{
    if (stack->leftCanary != _CANARY ||
               stack->rightCanary != _CANARY ||
               *_getLeftDataCanaryPtr(stack->data) != _CANARY ||)
    {
        return ERROR_DEAD_CANARY;
    }
    
    return EVERYTHING_FINE;
}

static canary_t* _getLeftDataCanaryPtr(const StackElement_t* data)
{
    return (canary_t*)((void*)data - sizeof(canary_t));
}

static canary_t* _getRightDataCanaryPtr(const StackElement_t* data, size_t realDataSize)
{
    return (canary_t*)((void*)data + realDataSize - 2 * sizeof(canary_t));
}
#endif

#ifdef HASH_PROTECTION
static ErrorCode _reHashify(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    hash_t hashData = _calculateDataHash(stack);
    hash_t hashStack = _calculateStackHash(stack);

    stack->hashData = hashData;
    stack->hashStack = hashStack;

    return EVERYTHING_FINE;
}

static hash_t _calculateDataHash(const Stack* stack)
{
    const void* data = (const void*)stack->data;

    #ifdef CANARY_PROTECTION
    data -= sizeof(canary_t);
    #endif

    return CalculateHash(data, stack->realDataSize, HASH_SEED);
}

static hash_t _calculateStackHash(Stack* stack)
{
    hash_t oldHashData = stack->hashData;
    hash_t oldHashStack = stack->hashStack;

    stack->hashData = 0;
    stack->hashStack = 0;

    hash_t stackHash = CalculateHash((const void*)stack, sizeof(*stack), HASH_SEED);

    stack->hashData = oldHashData;
    stack->hashStack = oldHashStack;

    return stackHash;
}

static ErrorCode _checkHash(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    if (stack->hashData != _calculateDataHash(stack) || stack->hashStack != _calculateStackHash(stack))
        return ERROR_BAD_HASH;
    
    return EVERYTHING_FINE;
}
#endif
