#include <string.h>
#include <ctype.h>
#include "Assembler.hpp"
#include "OneginFunctions.hpp"
#include "Stack.hpp"
#include "SPUsettings.ini"

enum ArgType
{
    ImmediateNumberArg = 1,
    RegisterArg = 2,
};

enum Command
{
    #define DEF_COMMAND(name, num, ...) \
        CMD_ ## name = num,

    #include "Commands.h"

    #undef DEF_COMMAND
};

struct ArgResult
{
    union DoubleStackEl
    {
        double immed;
        uint64_t regNum;
    } value;
    ArgType argType;
    ErrorCode error;
};

struct Label
{
    size_t codePosition;
    const char* label;
};

static ErrorCode _proccessLine(StackElement_t* codeArray, size_t* codePosition, const String* curLine, FILE* listingFile);

static ArgResult _getArg(const String* curLine, int commandLength);

uint64_t _translateCommandToBinFormat(Command command, ArgType argType);

ErrorCode Compile(const char* codeFilePath, const char* byteCodeFilePath, const char* listingFilePath)
{
    MyAssertSoft(codeFilePath, ERROR_NULLPTR);

    FILE* byteCodeFile = fopen(byteCodeFilePath, "wb");
    MyAssertSoft(byteCodeFile, ERROR_BAD_FILE);

    FILE* listingFile = fopen(listingFilePath, "w");
    MyAssertSoft(listingFile, ERROR_BAD_FILE);

    Text code = CreateText(codeFilePath, '\n');

    double* codeArray = (double*)calloc(code.numberOfLines * 2, sizeof(*codeArray));

    Label labelArray[MAX_LABELS] = {};

    fprintf(listingFile, "codePosition:   binary: %16scommand:%8sarg:\n", "", "");

    size_t codePosition = 0;
    for (size_t lineIndex = 0; lineIndex < code.numberOfLines; lineIndex++)
    {
        const String* curLine = &code.lines[lineIndex];
    
        ErrorCode proccessError = _proccessLine(codeArray, &codePosition, curLine, listingFile);
        
        if (proccessError)
        {
            DestroyText(&code);
            return proccessError;
        }
    }

    fwrite(codeArray, codePosition, sizeof(*codeArray), byteCodeFile);

    fclose(byteCodeFile);
    fclose(listingFile);
    DestroyText(&code);
    free(codeArray);

    return EVERYTHING_FINE;
}

// AUTO GENERATED!!!!!! CHANGE Commands.gen IF NEEDED!!!!!
static ErrorCode _proccessLine(StackElement_t* codeArray, size_t* codePosition, const String* curLine, FILE* listingFile)
{
    char* endLinePtr = (char*)strchr(curLine->text, '\n');
    if (endLinePtr)
        *endLinePtr = 0;

    char* commentPtr = (char*)strchr(curLine->text, ';');
    if (commentPtr)
        *commentPtr = 0;

    if (StringIsEmptyChars(curLine))
        return EVERYTHING_FINE;

    char command[10] = "";
    int commandLength = 0;

    if (sscanf(curLine->text, "%4s%n", command, &commandLength) != 1)
        return ERROR_SYNTAX;

    #define DEF_COMMAND(name, num, hasArg, ...)                                               \
    if (strcasecmp(command, #name) == 0)                                                      \
    {                                                                                         \
        fprintf(listingFile, "            [0x%zX]\t", *codePosition);                         \
        if (hasArg)                                                                           \
        {                                                                                     \
            ArgResult arg = _getArg(curLine, commandLength);                                  \
            RETURN_ERROR(arg.error);                                                          \
                                                                                              \
            uint64_t cmdBin = _translateCommandToBinFormat(CMD_ ## name, arg.argType);        \
            *((uint64_t*)codeArray + (*codePosition)++) = cmdBin;                             \
                                                                                              \
            switch (arg.argType)                                                              \
            {                                                                                 \
                case ImmediateNumberArg:                                                      \
                    codeArray[(*codePosition)++] = arg.value.immed;                           \
                    fprintf(listingFile, "0x%-4lX0x%-20lX%-8s    %lg", cmdBin,                \
                                                              *(uint64_t*)&arg.value.immed,   \
                                                              command, arg.value.immed);      \
                    break;                                                                    \
                case RegisterArg:                                                             \
                    *((uint64_t*)codeArray + (*codePosition)++) = arg.value.regNum;           \
                    fprintf(listingFile, "0x%-4lX0x%-20lX%-8s    %llu", cmdBin,               \
                                                                        arg.value.regNum,     \
                                                                        command,              \
                                                                        arg.value.regNum);    \
                    break;                                                                    \
                default:                                                                      \
                    return ERROR_SYNTAX;                                                      \
            }                                                                                 \
        }                                                                                     \
        else                                                                                  \
        {                                                                                     \
            *((uint64_t*)codeArray + (*codePosition)++) = num;                                \
            fprintf(listingFile, "0x%-4X%22s%s", num, "", command);                          \
        }                                                                                     \
        putc('\n', listingFile);                                                              \
    }                                                                                         \
    else 

    #include "Commands.gen"

    /*else*/ return ERROR_SYNTAX;

    #undef DEF_COMMAND

    return EVERYTHING_FINE;
}

static ArgResult _getArg(const String* curLine, int commandLength)
{
    ArgResult reg = {};

    if (sscanf(curLine->text + commandLength + 1, "%lg", &reg.value.immed) == 1)
    {
        reg.argType = ImmediateNumberArg;
        reg.error = EVERYTHING_FINE;
        return reg;
    }

    char regType = 0;
    if (sscanf(curLine->text + commandLength + 1, "r%cx", &regType) != 1)
    {
        reg.error = ERROR_SYNTAX;
        return reg;
    }

    reg.argType = RegisterArg;
    reg.value.regNum = regType - 'a' + 1;
    reg.error = EVERYTHING_FINE;

    return reg;
}

uint64_t _translateCommandToBinFormat(Command command, ArgType argType)
{
    return ((uint64_t)command | (argType << BITS_FOR_COMMAND));
}
