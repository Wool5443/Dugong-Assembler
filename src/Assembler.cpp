#include <string.h>
#include <ctype.h>
#include "Assembler.hpp"
#include "OneginFunctions.hpp"

#define ON_SECOND_RUN(...) if (isSecondRun) __VA_ARGS__

#define FREE_JUNK fclose(binaryFile); fclose(listingFile); free(codeArray); \
                  DestroyText(&code); for (size_t i = 0; i < MAX_LABELS; i++) free((void*)labelArray[i].label)

static const size_t MAX_LABELS = 128;
static const size_t MAX_LABEL_SIZE = 48;
static const size_t MAX_COMMAND_LENGTH = 4;
const size_t LABEL_NOT_FOUND = (size_t)-1;

static const size_t MAX_ARGS_SIZE = sizeof(double) + 1;
static const size_t REG_NUM_BYTE  = MAX_ARGS_SIZE - 1;

typedef unsigned char byte;
#include "SPUsettings.ini"

enum Command
{
    #define DEF_COMMAND(name, num, ...) \
        CMD_ ## name = num,

    #include "Commands.gen"

    #undef DEF_COMMAND
};

struct Arg
{
    double immed;
    byte regNum;
    byte argType;
};

struct ArgResult
{
    Arg value;
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
    double codePosition;
};

static ErrorCode _proccessToken(byte* codeArray, size_t* codePosition,
                               Label labelArray[], size_t* freeLabelCell,
                               String* curToken, FILE* listingFile,
                               bool isSecondRun);

static ErrorCode _insertLabel(Label* labelArray, size_t* freeLabelCell, const String* curToken,
                              const char* labelEnd, size_t codePosition);

static ArgResult _parseArg(const char* argStr, const Label labelArray[], bool isSecondRun);

static ArgResult _parseReg(const char** argStr);

static ArgResult _parseImmed(const char** argStr);

static ArgResult _parseLabel(const char** argStr, bool isSecondRun, const Label labelArray[]);

static ArgResult _parseImmedLabel(const char** argStr, const Label labelArray[], bool isSecondRun);

static CodePositionResult _getLabelCodePosition(const Label labelArray[], const char* label);

static byte _translateCommandToBinFormat(Command command, byte argType);

// too many args to be a separate function
#define RUN_COMPILATION(isSecondRun)                                                                            \
    codePosition = 0;                                                                                           \
    for (size_t tokenIndex = 0; tokenIndex < code.numberOfTokens; tokenIndex++)                                 \
    {                                                                                                           \
        const String* curToken = &code.tokens[tokenIndex];                                                      \
                                                                                                                \
        ErrorCode proccessError = _proccessToken(codeArray, &codePosition, labelArray, &freeLabelCell,          \
                                               (String*)curToken, listingFile, isSecondRun);                    \
                                                                                                                \
        if (proccessError)                                                                                      \
        {                                                                                                       \
            SetConsoleColor(stdout, COLOR_RED);                                                                 \
            printf("%s in line #%zu: \"%s\"\n", ERROR_CODE_NAMES[proccessError], tokenIndex, curToken->text);   \
            SetConsoleColor(stdout, COLOR_WHITE);                                                               \
            FREE_JUNK;                                                                                          \
            return proccessError;                                                                               \
        }                                                                                                       \
    }

ErrorCode Compile(const char* codeFilePath, const char* binaryFilePath, const char* listingFilePath)
{
    MyAssertSoft(codeFilePath, ERROR_NULLPTR);
    MyAssertSoft(binaryFilePath, ERROR_NULLPTR);
    MyAssertSoft(listingFilePath, ERROR_NULLPTR);

    FILE* binaryFile = fopen(binaryFilePath, "wb");
    MyAssertSoft(binaryFile, ERROR_BAD_FILE);

    FILE* listingFile  = fopen(listingFilePath,  "w");
    MyAssertSoft(listingFile, ERROR_BAD_FILE);

    Text code = CreateText(codeFilePath, '\n');

    byte* codeArray = (byte*)calloc(code.numberOfTokens, sizeof(double) + 2);

    Label  labelArray[MAX_LABELS] = {};
    size_t freeLabelCell = 0;

    size_t codePosition = 0;
    RUN_COMPILATION(false);

    fprintf(listingFile, "Code position:%20s cmd:%4s arg:%24s original:\n", "", "", "");

    RUN_COMPILATION(true);

    fprintf(listingFile, "\nLabel array:\n");
    for (size_t i = 0; i < freeLabelCell; i++)
    {
        fprintf(listingFile, "[%zu]\n", i);
        fprintf(listingFile, "{\n%4scodePosition = %lg\n", "", labelArray[i].codePosition);
        fprintf(listingFile, "%4slabel = %s\n}\n", "", labelArray[i].label);
    }

    fwrite(codeArray, codePosition, sizeof(*codeArray), binaryFile);

    FREE_JUNK;

    return EVERYTHING_FINE;
}

static ErrorCode _proccessToken(byte* codeArray, size_t* codePosition,
                               Label labelArray[], size_t* freeLabelCell,
                               String* curToken, FILE* listingFile,
                               bool isSecondRun)
{
    ((char*)curToken->text)[curToken->length] = '\0';

    char* commentPtr = (char*)strchr(curToken->text, ';');
    if (commentPtr)
        *commentPtr = '\0';

    if (StringIsEmptyChars(curToken->text, '\0'))
        return EVERYTHING_FINE;

    const char* labelEnd = strchr(curToken->text, ':');
    if (labelEnd)
    {
        if (isSecondRun)
        {
            if (commentPtr)
                *commentPtr = ';';
            fprintf(listingFile, "%82s%s\n", "", curToken->text);
            if (commentPtr)
                *commentPtr = '\0';
            return EVERYTHING_FINE;
        }

        if (labelEnd)
        {
            ErrorCode labelError = _insertLabel(labelArray, freeLabelCell, curToken, labelEnd, *codePosition);
            return labelError;
        }
    }

    char command[MAX_COMMAND_LENGTH + 1] = "";
    int commandLength = 0;

    if (sscanf(curToken->text, "%4s%n", command, &commandLength) != 1)
        return ERROR_SYNTAX;

    #define DEF_COMMAND(name, num, hasArg, ...)                                             \
    if (strcasecmp(command, #name) == 0)                                                    \
    {                                                                                       \
        ON_SECOND_RUN(fprintf(listingFile, "%13s [0x%016lX] %4s", "",                       \
                             *codePosition, ""));                                           \
                                                                                            \
        if (hasArg)                                                                         \
        {                                                                                   \
            ArgResult argRes = _parseArg(curToken->text + commandLength + 1,                 \
                                         labelArray, isSecondRun);                          \
            RETURN_ERROR(argRes.error);                                                     \
                                                                                            \
            Arg arg = argRes.value;                                                         \
                                                                                            \
            byte cmd = _translateCommandToBinFormat(CMD_ ## name, arg.argType);             \
            memcpy(codeArray + (*codePosition)++, &cmd, 1);                                 \
                                                                                            \
            if (arg.argType & ImmediateNumberArg)                                           \
            {                                                                               \
                memcpy(codeArray + *codePosition, &arg.immed, sizeof(double));              \
                *codePosition += sizeof(double);                                            \
            }                                                                               \
            if (arg.argType & RegisterArg)                                                  \
            {                                                                               \
                memcpy(codeArray + *codePosition, &arg.regNum, 1);                          \
                *codePosition += 1;                                                         \
            }                                                                               \
                                                                                            \
            ON_SECOND_RUN(fprintf(listingFile, "0x%02hX %4s", cmd, ""));                    \
            ON_SECOND_RUN(fprintf(listingFile, "0x%016lX 0x%02hhX %10s",                    \
                                  *(uint64_t*)&arg.immed, arg.regNum, ""));                 \
        }                                                                                   \
        else                                                                                \
        {                                                                                   \
            byte cmd = (byte)CMD_ ## name;                                                  \
            memcpy(codeArray + (*codePosition)++, &cmd, 1);                                 \
            ON_SECOND_RUN(fprintf(listingFile, "0x%02hX %38s", cmd, ""));                   \
        }                                                                                   \
    }                                                                                       \
    else 

    #include "Commands.gen"

    /*else*/ return ERROR_SYNTAX;

    #undef DEF_COMMAND

    if (commentPtr)
        *commentPtr = ';';

    ON_SECOND_RUN(fprintf(listingFile, "%s\n", curToken->text));

    return EVERYTHING_FINE;
}

static ArgResult _parseArg(const char* argStr, const Label labelArray[], bool isSecondRun)
{
    MyAssertSoftResult(argStr,     {}, ERROR_NULLPTR);
    MyAssertSoftResult(labelArray, {}, ERROR_NULLPTR);

    const char* bracketPtr = strchr(argStr, '[');
    char* backBracketPtr   = (char*)strchr(argStr, ']');
    if (bracketPtr || backBracketPtr)
    {
        if (!(bracketPtr && backBracketPtr))
            return {{}, ERROR_SYNTAX};

        *backBracketPtr = '\0';
        
        argStr = bracketPtr + 1;
    }

    char* plusPtr = (char*)strchr(argStr, '+');
    if (plusPtr)
        *plusPtr = '\0';

    ArgResult argRes = {};

    {
        ArgResult regRes = _parseReg(&argStr);
        if (!regRes.error)
            argRes = regRes;
        else
        {
            argRes = _parseImmedLabel(&argStr, labelArray, isSecondRun);
            RETURN_ERROR_RESULT(argRes, {});
        }
    }

    if (plusPtr)
    {
        *plusPtr = '+';
        argStr = plusPtr + 1;
        ArgResult immOrLabelRes = _parseImmedLabel(&argStr, labelArray, isSecondRun);

        RETURN_ERROR_RESULT(immOrLabelRes, {});

        argRes.value.immed += immOrLabelRes.value.immed;
    }

    if (backBracketPtr)
    {
        argRes.value.argType |= RAMArg;
        *backBracketPtr = ']';
        if (!StringIsEmptyChars(backBracketPtr + 1, 0))
            return {{}, ERROR_SYNTAX};
    }

    if (argRes.value.argType == 0 || argRes.value.argType == RAMArg)
        return {{}, ERROR_SYNTAX};

    argRes.error = EVERYTHING_FINE;

    return argRes;
}

static ArgResult _parseReg(const char** argStr)
{
    ArgResult argRes = {};
    int regType = 0, readChars = 0;

    if (sscanf(*argStr, "r%c%n", (char*)&regType, &readChars) == 1 && (*argStr)[readChars] == 'x' &&
                                                                        StringIsEmptyChars(*argStr + readChars + 1, '\0'))
    {
        readChars += 1;
        argRes.value.argType |= RegisterArg;
        argRes.value.regNum   = regType -'a' + 1;
        argRes.error          = EVERYTHING_FINE;

        *argStr += readChars;

        return argRes;
    }
    else
        return {{}, ERROR_SYNTAX};
}

static ArgResult _parseImmed(const char** argStr)
{
    ArgResult argRes = {};

    int readChars = 0;

    double immed = 0;

    if (sscanf(*argStr, "%lg%n", &immed, &readChars) == 1 && StringIsEmptyChars(*argStr + readChars, '\0'))
    {
        argRes.value.argType |= ImmediateNumberArg;
        argRes.value.immed   += immed;
        argRes.error          = EVERYTHING_FINE;

        argStr += readChars;

        return argRes;
    }
    else
        return {{}, ERROR_SYNTAX};
}

static ArgResult _parseLabel(const char** argStr, bool isSecondRun, const Label labelArray[])
{
    ArgResult argRes = {};

    int readChars = 0;

    char label[MAX_LABEL_SIZE] = "";

    sscanf(*argStr, "%s%n", label, &readChars);

    CodePositionResult labelCodePostitionResult = _getLabelCodePosition(labelArray, label);

    RETURN_ERROR_RESULT(labelCodePostitionResult, {});

    if (labelCodePostitionResult.value != LABEL_NOT_FOUND)
    {
        argRes.value.argType |= ImmediateNumberArg;
        argRes.value.immed    = labelCodePostitionResult.value;
        argRes.error          = EVERYTHING_FINE;

        *argStr += readChars;

        return argRes;
    }

    if (isSecondRun)
        return {{}, ERROR_SYNTAX};

    argRes.value.argType = ImmediateNumberArg;
    argRes.value.immed   = LABEL_NOT_FOUND;

    return argRes;
}

static ArgResult _parseImmedLabel(const char** argStr, const Label labelArray[], bool isSecondRun)
{
    ArgResult immRes = _parseImmed(argStr);

    if (!immRes.error)
        return immRes;
    else
    {
        ArgResult labelRes = _parseLabel(argStr, isSecondRun, labelArray);

        if (!labelRes.error)
            return labelRes;
        else
            return {{}, ERROR_SYNTAX};
    }
}

static ErrorCode _insertLabel(Label labelArray[], size_t* freeLabelCell, const String* curToken, const char* labelEnd,
                              size_t codePosition)
{
    MyAssertSoft(labelArray, ERROR_NULLPTR);
    MyAssertSoft(freeLabelCell, ERROR_NULLPTR);
    MyAssertSoft(curToken, ERROR_NULLPTR);
    MyAssertSoft(labelEnd, ERROR_NULLPTR);

    if (*freeLabelCell >= MAX_LABELS)
        return ERROR_TOO_MANY_LABELS;

    const char* labelStart = curToken->text;

    while (isspace(*labelStart) && labelStart < labelEnd) labelStart++;

    size_t labelLength = labelEnd - labelStart;

    if (labelLength == 0 || labelLength > MAX_LABEL_SIZE)
        return ERROR_WRONG_LABEL_SIZE;

    char* label = (char*)calloc(labelLength + 1, 1);

    strncpy(label, labelStart, labelLength);
    label[labelLength] = '\0';

    labelArray[*freeLabelCell].label = label;
    labelArray[*freeLabelCell].codePosition = codePosition;

    (*freeLabelCell)++;

    return EVERYTHING_FINE;
}

static CodePositionResult _getLabelCodePosition(const Label labelArray[], const char* label)
{
    MyAssertSoftResult(labelArray, LABEL_NOT_FOUND, ERROR_NULLPTR);
    MyAssertSoftResult(label,      LABEL_NOT_FOUND, ERROR_NULLPTR);

    for (size_t i = 0; i < MAX_LABELS; i++)
        if (labelArray[i].label && strncmp(labelArray[i].label, label, MAX_LABEL_SIZE) == 0)
            return {labelArray[i].codePosition, EVERYTHING_FINE};

    return {LABEL_NOT_FOUND, EVERYTHING_FINE};
}

static byte _translateCommandToBinFormat(Command command, byte argType)
{
    return ((byte)command | (argType << BITS_FOR_COMMAND));
}
