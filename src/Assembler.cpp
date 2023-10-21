#include <string.h>
#include <ctype.h>
#include "Assembler.hpp"
#include "OneginFunctions.hpp"
#include "SPUsettings.ini"

#define ON_SECOND_RUN(...) if (isSecondRun) __VA_ARGS__

#define FREE_JUNK fclose(byteCodeFile); fclose(listingFile); free(codeArray); \
                  DestroyText(&code); for (size_t i = 0; i < MAX_LABELS; i++) free((void*)labelArray[i].label)

static const size_t MAX_LABELS = 32;
static const size_t MAX_LABEL_SIZE = 32;
const size_t LABEL_NOT_FOUND = (size_t)-1;

static const size_t MAX_ARGS_SIZE = sizeof(double) + 1;
static const size_t REG_NUM_BYTE  = MAX_ARGS_SIZE - 1;

enum ArgType
{
    ImmediateNumberArg = 1,
    RegisterArg        = 2,
    RAMArg             = 4,
};

enum Command
{
    #define DEF_COMMAND(name, num, ...) \
        CMD_ ## name = num,

    #include "Commands.gen"

    #undef DEF_COMMAND
};

struct ArgResult
{
    double immed;
    char regNum;
    char argType;
    ErrorCode error;
};

struct CodePositionResult
{
    size_t value;
    ErrorCode error;
};

struct Label
{
    const char* label;
    size_t codePosition;
};

static ErrorCode _proccessLine(char* codeArray, size_t* codePosition,
                               Label labelArray[], size_t* freeLabelCell,
                               String* curLine, FILE* listingFile,
                               bool isSecondRun);

static ErrorCode _insertLabel(Label* labelArray, size_t* freeLabelCell, const String* curLine, const char* labelEnd,
                              size_t codePosition);

static ArgResult _parseArg(const char* argStr, const Label labelArray[]);

static CodePositionResult _getLabelCodePosition(const Label labelArray[], const char* label);

static char _translateCommandToBinFormat(Command command, char argType);

ErrorCode Compile(const char* codeFilePath, const char* byteCodeFilePath, const char* listingFilePath)
{
    MyAssertSoft(codeFilePath, ERROR_NULLPTR);

    FILE* byteCodeFile = fopen(byteCodeFilePath, "wb");
    MyAssertSoft(byteCodeFile, ERROR_BAD_FILE);

    FILE* listingFile  = fopen(listingFilePath,   "w");
    MyAssertSoft(listingFile,  ERROR_BAD_FILE);

    Text code = CreateText(codeFilePath, '\n');

    char* codeArray = (char*)calloc(code.numberOfLines * 2, sizeof(double));

    Label  labelArray[MAX_LABELS] = {};
    size_t freeLabelCell = 0;

    fprintf(listingFile, "Code position:%20s cmd:%4s arg:%24s original:\n", "", "", "");

    size_t codePosition = 0;
    for (size_t lineIndex = 0; lineIndex < code.numberOfLines; lineIndex++)
    {
        const String* curLine   = &code.lines[lineIndex];

        ErrorCode proccessError = _proccessLine(codeArray, &codePosition, labelArray, &freeLabelCell,
                                               (String*)curLine, listingFile, false);

        if (proccessError)
        {
            FREE_JUNK;
            return proccessError;
        }
    }

    codePosition = 0;
    for (size_t lineIndex = 0; lineIndex < code.numberOfLines; lineIndex++)
    {
        const String* curLine = &code.lines[lineIndex];
    
        ErrorCode proccessError = _proccessLine(codeArray, &codePosition, labelArray, &freeLabelCell,
                                               (String*)curLine, listingFile, true);

        if (proccessError)
        {
            FREE_JUNK;
            return proccessError;
        }
    }

    fwrite(codeArray, codePosition, sizeof(*codeArray), byteCodeFile);

    FREE_JUNK;

    return EVERYTHING_FINE;
}

// AUTO GENERATED!!!!!! CHANGE Commands.gen IF NEEDED!!!!!
static ErrorCode _proccessLine(char* codeArray, size_t* codePosition,
                               Label labelArray[], size_t* freeLabelCell,
                               String* curLine, FILE* listingFile,
                               bool isSecondRun)
{
    char* endLinePtr = (char*)strchr(curLine->text, '\n');
    if (endLinePtr)
        *endLinePtr = 0;

    char* commentPtr = (char*)strchr(curLine->text, ';');
    if (commentPtr)
        *commentPtr = 0;

    if (StringIsEmptyChars(curLine))
        return EVERYTHING_FINE;

    char command[5] = "";
    int commandLength = 0;

    const char* labelEnd = strchr(curLine->text, ':');
    if (labelEnd)
    {
        if (isSecondRun)
            return EVERYTHING_FINE;

        if (labelEnd)
        {
            ErrorCode labelError = _insertLabel(labelArray, freeLabelCell, curLine, labelEnd, *codePosition);
            return labelError;
        }
    }

    if (sscanf(curLine->text, "%4s%n", command, &commandLength) != 1)
        return ERROR_SYNTAX;

    #define DEF_COMMAND(name, num, hasArg, ...)                                               \
    if (strcasecmp(command, #name) == 0)                                                      \
    {                                                                                         \
        ON_SECOND_RUN(fprintf(listingFile, "%13s [0x%016lX] %4s", "",                         \
                             (uint64_t)*codePosition, ""));                                   \
                                                                                              \
        if (hasArg)                                                                           \
        {                                                                                     \
            ArgResult arg = _parseArg(curLine->text + commandLength + 1, labelArray);           \
            RETURN_ERROR(arg.error);                                                          \
                                                                                              \
            char cmd = _translateCommandToBinFormat(CMD_ ## name, arg.argType);               \
            memcpy(codeArray + (*codePosition)++, &cmd, 1);                                   \
                                                                                              \
            if (arg.argType & ImmediateNumberArg)                                             \
            {                                                                                 \
                memcpy(codeArray + *codePosition, &arg.immed, sizeof(double));                \
                *codePosition += sizeof(double);                                              \
            }                                                                                 \
            if (arg.argType & RegisterArg)                                                    \
            {                                                                                 \
                memcpy(codeArray + *codePosition, &arg.regNum, 1);                            \
                *codePosition += 1;                                                           \
            }                                                                                 \
                                                                                              \
            ON_SECOND_RUN(fprintf(listingFile, "0x%02hX %4s", cmd, ""));                      \
            ON_SECOND_RUN(fprintf(listingFile, "0x%016lX 0x%02hhX %10s",                      \
                                  *(uint64_t*)&arg.immed, arg.regNum, ""));                   \
        }                                                                                     \
        else                                                                                  \
        {                                                                                     \
            char cmd = (char)CMD_ ## name;                                                    \
            memcpy(codeArray + (*codePosition)++, &cmd, 1);                                   \
            ON_SECOND_RUN(fprintf(listingFile, "0x%02hX %38s", cmd, ""));                     \
        }                                                                                     \
    }                                                                                         \
    else 

    #include "Commands.gen"

    /*else*/ return ERROR_SYNTAX;

    #undef DEF_COMMAND

    if (commentPtr)
        *commentPtr = ';';

    ON_SECOND_RUN(fprintf(listingFile, "%s\n", curLine->text));

    return EVERYTHING_FINE;
}

static ArgResult _parseArg(const char* argStr, const Label labelArray[])
{
    if (!argStr || !labelArray)
        return {LABEL_NOT_FOUND, ERROR_NULLPTR};

    ArgResult result = {};

    const char* bracketPtr = strchr(argStr, '[');
    if (bracketPtr)
    {
        if (!strchr(argStr, ']'))
        {
            result.error = ERROR_SYNTAX;
            return result;
        }
        argStr = bracketPtr + 1;
        result.argType |= RAMArg;
    }

    int readChars = 0;

    int regType = 0;
    if (sscanf(argStr, "r%cx%n", &regType, &readChars) == 1)
    {
        result.argType |= RegisterArg;
        result.regNum = regType -'a' + 1;
        argStr += readChars;
    }

    const char* plusPtr = strchr(argStr, '+');
    if (plusPtr)
        argStr = plusPtr + 1;

    double immed = 0;
    if (sscanf(argStr, "%lg%n", &immed, &readChars) == 1)
    {
        result.argType |= ImmediateNumberArg;
        if (result.argType & RAMArg)
        {
            uint64_t intImmed = (uint64_t)immed;
            result.immed = *(double*)&intImmed;
        }
        else
            result.immed = immed;
    }

    if (result.argType & ImmediateNumberArg && result.argType & RegisterArg && plusPtr)
    {
        result.error = ERROR_SYNTAX;

        return result;
    }

    if (result.argType == 0)
    {
        char label[MAX_LABEL_SIZE] = "";

        sscanf(argStr, "%s", label);

        CodePositionResult labelCodePostitionResult = _getLabelCodePosition(labelArray, label);

        if (labelCodePostitionResult.error)
        {
            result.error = ERROR_SYNTAX;
            return result;
        }

        result.argType = ImmediateNumberArg;
        *(uint64_t*)&result.immed = labelCodePostitionResult.value;
        result.error = EVERYTHING_FINE;
    }

    if (result.argType == 0 || result.argType == RAMArg)
        result.error = ERROR_SYNTAX;

    return result;
}

static ErrorCode _insertLabel(Label labelArray[], size_t* freeLabelCell, const String* curLine, const char* labelEnd,
                              size_t codePosition)
{
    const char* labelStart = curLine->text;

    while (isspace(*labelStart) && labelStart < labelEnd) labelStart++;

    size_t labelLength = labelEnd - labelStart;

    if (labelLength == 0 || labelLength > MAX_LABEL_SIZE)
        return ERROR_WRONG_LABEL_SIZE;

    char* label = (char*)calloc(labelLength + 1, 1);

    strncpy(label, labelStart, labelLength);
    label[labelLength] = 0;

    labelArray[*freeLabelCell].label = label;
    labelArray[*freeLabelCell].codePosition = codePosition;

    (*freeLabelCell)++;

    return EVERYTHING_FINE;
}

static CodePositionResult _getLabelCodePosition(const Label labelArray[], const char* label)
{
    if (!labelArray || !label)
        return {LABEL_NOT_FOUND, ERROR_NULLPTR};

    for (size_t i = 0; i < MAX_LABELS; i++)
        if (labelArray[i].label && strncmp(labelArray[i].label, label, MAX_LABEL_SIZE) == 0)
            return {labelArray[i].codePosition, EVERYTHING_FINE};
    return {LABEL_NOT_FOUND, EVERYTHING_FINE};
}

static char _translateCommandToBinFormat(Command command, char argType)
{
    return ((char)command | (argType << BITS_FOR_COMMAND));
}
