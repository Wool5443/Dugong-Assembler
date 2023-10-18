#include <string.h>
#include <ctype.h>
#include "Assembler.hpp"
#include "OneginFunctions.hpp"
#include "Stack.hpp"
#include "SPUsettings.ini"

enum ArgType
{
    ImmediateNumberArg = 1,
    RegisterArg        = 2,
    LabelArg           = 3,
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
    union DoubleStackEl
    {
        double   immed;
        uint64_t regNum;
        size_t   codePosition;
    } value;
    ArgType argType;
    ErrorCode error;
};

struct CodePositionResult
{
    uint64_t codePosition;
    ErrorCode error;
};

struct Label
{
    size_t codePosition;
    const char* label;
};

const size_t LABEL_NOT_FOUND = (size_t)-1;

#define ON_SECOND_RUN(...) if (isSecondRun) __VA_ARGS__

static ErrorCode _proccessLine(StackElement_t* codeArray, size_t* codePosition,
                               Label labelArray[], size_t* freeLabelCell, String* curLine, FILE* listingFile,
                               bool isSecondRun);

static ErrorCode _insertLabel(Label* labelArray, size_t* freeLabelCell, const String* curLine, const char* labelEnd,
                              size_t codePosition);

static ArgResult _getArg(const String* curLine, int commandLength, const Label labelArray[]);

static size_t _getLabelCodePosition(const Label labelArray[], const char* label);

static uint64_t _translateCommandToBinFormat(Command command, ArgType argType);

ErrorCode Compile(const char* codeFilePath, const char* byteCodeFilePath, const char* listingFilePath)
{
    MyAssertSoft(codeFilePath, ERROR_NULLPTR);

    FILE* byteCodeFile = fopen(byteCodeFilePath, "wb");
    MyAssertSoft(byteCodeFile, ERROR_BAD_FILE);

    FILE* listingFile  = fopen(listingFilePath,   "w");
    MyAssertSoft(listingFile,  ERROR_BAD_FILE);

    Text code = CreateText(codeFilePath, '\n');

    double* codeArray = (double*)calloc(code.numberOfLines * 2, sizeof(*codeArray));

    Label  labelArray[MAX_LABELS] = {};
    size_t freeLabelCell = 0;

    fprintf(listingFile, "codePosition:\t\tbinary:\t%17scommand:%8sarg:\n", "", "");

    size_t codePosition = 0;
    for (size_t lineIndex = 0; lineIndex < code.numberOfLines; lineIndex++)
    {
        const String* curLine   = &code.lines[lineIndex];

        ErrorCode proccessError = _proccessLine(codeArray, &codePosition, labelArray, &freeLabelCell,
                                               (String*)curLine, listingFile, false);

        if (proccessError)
        {
            DestroyText(&code);
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
            DestroyText(&code);
            return proccessError;
        }
    }

    fwrite(codeArray, codePosition, sizeof(*codeArray), byteCodeFile);

    fclose(byteCodeFile);
    fclose(listingFile);
    DestroyText(&code);
    free(codeArray);

    for (size_t i = 0; i < MAX_LABELS; i++)
        free((void*)labelArray[i].label);

    return EVERYTHING_FINE;
}

// AUTO GENERATED!!!!!! CHANGE Commands.gen IF NEEDED!!!!!
static ErrorCode _proccessLine(StackElement_t* codeArray, size_t* codePosition,
                               Label labelArray[], size_t* freeLabelCell, String* curLine, FILE* listingFile,
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

    char command[10] = "";
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
        ON_SECOND_RUN(fprintf(listingFile, "\t\t\t[0x%zX]\t\t", *codePosition));                \
        if (hasArg)                                                                           \
        {                                                                                     \
            ArgResult arg = _getArg(curLine, commandLength, labelArray);                      \
            RETURN_ERROR(arg.error);                                                          \
                                                                                              \
            uint64_t cmdBin = _translateCommandToBinFormat(CMD_ ## name, arg.argType);        \
            *((uint64_t*)codeArray + (*codePosition)++) = cmdBin;                             \
                                                                                              \
            switch (arg.argType)                                                              \
            {                                                                                 \
                case ImmediateNumberArg:                                                      \
                    codeArray[(*codePosition)++] = arg.value.immed;                           \
                    ON_SECOND_RUN(fprintf(listingFile, "0x%-4lX0x%-20lX%-10s    %lg", cmdBin,  \
                                                        *(uint64_t*)&arg.value.immed,         \
                                                        command, arg.value.immed));           \
                    break;                                                                    \
                case RegisterArg:                                                             \
                    *((uint64_t*)codeArray + (*codePosition)++) = arg.value.regNum;           \
                    ON_SECOND_RUN(fprintf(listingFile, "0x%-4lX0x%-20lX%-10s    %lu", cmdBin,  \
                                                        arg.value.regNum,                     \
                                                        command,                              \
                                                        arg.value.regNum));                   \
                    break;                                                                    \
                case LabelArg:                                                             \
                    *((uint64_t*)codeArray + (*codePosition)++) = arg.value.codePosition;     \
                    ON_SECOND_RUN(fprintf(listingFile, "0x%-4lX0x%-20lX%-10s    %lu", cmdBin,  \
                                                        arg.value.codePosition,               \
                                                        command,                              \
                                                        arg.value.codePosition));             \
                    break;                                                                    \
                default:                                                                      \
                    return ERROR_SYNTAX;                                                      \
            }                                                                                 \
        }                                                                                     \
        else                                                                                  \
        {                                                                                     \
            *((uint64_t*)codeArray + (*codePosition)++) = num;                                \
            ON_SECOND_RUN(fprintf(listingFile, "0x%-4X%22s%s", num, "", command));            \
        }                                                                                     \
        ON_SECOND_RUN(if (commentPtr) fprintf(listingFile, "\t%s", commentPtr + 1));          \
        ON_SECOND_RUN(putc('\n', listingFile));                                               \
    }                                                                                         \
    else 

    #include "Commands.gen"

    /*else*/ return ERROR_SYNTAX;

    #undef DEF_COMMAND

    return EVERYTHING_FINE;
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

static size_t _getLabelCodePosition(const Label labelArray[], const char* label)
{
    for (size_t i = 0; i < MAX_LABELS; i++)
        if (strncmp(labelArray[i].label, label, MAX_LABEL_SIZE) == 0)
            return labelArray[i].codePosition;
    return LABEL_NOT_FOUND;
}

static ArgResult _getArg(const String* curLine, int commandLength, const Label labelArray[])
{
    ArgResult reg = {};

    const char* argStrPtr = curLine->text + commandLength + 1;

    if (sscanf(argStrPtr, "%lg", &reg.value.immed) == 1)
    {
        reg.argType = ImmediateNumberArg;
        reg.error = EVERYTHING_FINE;

        return reg;
    }

    char regType = 0;
    if (sscanf(argStrPtr, "r%cx", &regType) == 1)
    {
        reg.argType = RegisterArg;
        reg.value.regNum = regType - 'a' + 1;
        reg.error = EVERYTHING_FINE;

        return reg;
    }

    char label[MAX_LABEL_SIZE + 1] = "";
    if (sscanf(argStrPtr, "%32s", label) == 1)
    {
        size_t codePosition = _getLabelCodePosition(labelArray, label);

        reg.argType = LabelArg;
        reg.value.codePosition = codePosition;
        reg.error = EVERYTHING_FINE;

        return reg;
    }

    reg.error = ERROR_SYNTAX;

    return reg;
}

static uint64_t _translateCommandToBinFormat(Command command, ArgType argType)
{
    return ((uint64_t)command | (argType << BITS_FOR_COMMAND));
}
