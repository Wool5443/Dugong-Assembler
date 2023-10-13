#include <string.h>
#include <ctype.h>
#include "Assembler.hpp"
#include "OneginFunctions.hpp"
#include "Stack.hpp"
#include "SPUcommands.settings"

enum STRING_AS_NUM_COMMANDS
{
    push_sn = 1752397168,
    pop_sn  = 7368560,
    in_sn   = 28265,
    out_sn  = 7632239,
    hlt_sn  = 7629928,
    add_sn  = 6579297,
    sub_sn  = 6452595,
    mul_sn  = 7107949,
    div_sn  = 7760228,
    sqrt_sn = 1953657203,
    sin_sn  = 7235955,
    cos_sn  = 7565155,
};

enum ArgType
{
    ImmediateNumberArg = 1,
    RegisterArg = 2,
    ImmediateAndRegisterArg = 3,
};

struct CommandResult
{
    Command command;
    ErrorCode error;
};

static const uint64_t BITS_FOR_COMMAND = 4;

CommandResult _translateRawCommand(uint64_t rawCommand);

ErrorCode _writeCommand(FILE* outFile, const String* curLine, CompilationMode compilationMode);

uint64_t _translateArgCommandToBinFormat(Command command, ArgType argType);

ErrorCode Compile(const char* codeFilePath, const char* byteCodeOutPath, CompilationMode compilationMode)
{
    MyAssertSoft(codeFilePath, ERROR_NULLPTR);

    FILE* outFile = {};

    if (compilationMode == DRAFT_COMPILATION)
        outFile = fopen(byteCodeOutPath, "w");
    else if (compilationMode == BINARY_COMPILATION)
        outFile = fopen(byteCodeOutPath, "wb");

    MyAssertSoft(outFile, ERROR_BAD_FILE);

    Text code = CreateText(codeFilePath, '\n');

    uint64_t* codeArray = (uint64_t*)calloc(code.numberOfLines, sizeof(*codeArray));

    for (size_t position = 0; position < code.numberOfLines; position++)
    {
        const String* curLine = &code.lines[position];
    
        ErrorCode writeError = _writeCommand(outFile, curLine, compilationMode);
        
        if (writeError)
        {
            DestroyText(&code);
            return writeError;
        }
    }

    fclose(outFile);
    DestroyText(&code);

    return EVERYTHING_FINE;
}

ErrorCode _writeCommand(FILE* outFile, const String* curLine, CompilationMode compilationMode)
{
    char* endLinePtr = (char*)strchr(curLine->text, '\n');
    if (endLinePtr)
        *endLinePtr = 0;

    char* commentPtr = (char*)strchr(curLine->text, ';');
    if (commentPtr)
        *commentPtr = 0;

    if (StringIsEmptyChars(curLine))
        return EVERYTHING_FINE;

    uint64_t rawCommand = 0;

    int commandLength = 0;

    if (sscanf(curLine->text, "%4s%n", (char*)&rawCommand, &commandLength) != 1)
        return ERROR_SYNTAX;

    CommandResult resCommand = _translateRawCommand(rawCommand);

    if (resCommand.error)
        return resCommand.error;

    Command command = resCommand.command;

    switch (command)
    {
    case Push_c:
    case Pop_c:
    {
        double arg = 0;

        if (sscanf(curLine->text + commandLength + 1, "%lg", &arg) == 0)
        {
            char regType = 0;
            int argLength = 0;

            if (sscanf(curLine->text + commandLength + 1, "r%c%n", &regType, &argLength) != 1)
                return ERROR_SYNTAX;

            if (!StringIsEmptyChars(curLine->text + commandLength + argLength + 2, '\0'))
                return ERROR_SYNTAX;

            uint64_t regNumber = regType - 'a' + 1;
            
            if (compilationMode == DRAFT_COMPILATION)
                fprintf(outFile, "%u %u %u%30s %s\n", (uint64_t)command,
                                                      (uint64_t)RegisterArg, regNumber, COMMAND_NAMES[command],
                                                      REGISTERS_NAMES[regNumber]);
            else if (compilationMode == BINARY_COMPILATION)
            {
                uint64_t commandWithArg = _translateArgCommandToBinFormat(command, RegisterArg);
                fwrite(&commandWithArg, sizeof(commandWithArg), 1, outFile);
                fwrite(&regNumber, sizeof(regNumber), 1, outFile);

                fprintf(stderr, "Command: %llu\n", commandWithArg);
                fprintf(stderr, "Arg: %llu\n", regNumber);
            }
            else
                return ERROR_SYNTAX;
        }
        else
        {
            if (compilationMode == DRAFT_COMPILATION)
                fprintf(outFile, "%llu %llu %lg%30s %lg\n", (uint64_t)command, (uint64_t)ImmediateNumberArg, arg, COMMAND_NAMES[command], arg);
            else if (compilationMode == BINARY_COMPILATION)
            {
                uint64_t commandWithArg = _translateArgCommandToBinFormat(command, ImmediateNumberArg);
                fwrite(&commandWithArg, sizeof(commandWithArg), 1, outFile);
                fwrite(&arg, sizeof(arg), 1, outFile);

                fprintf(stderr, "Command: %llu\n", commandWithArg);
                fprintf(stderr, "Arg: %lg\n", arg);
            }
            else
                return ERROR_SYNTAX;
        }
        break;
    }
    
    default:
        if (compilationMode == DRAFT_COMPILATION)
            fprintf(outFile, "%llu%30s\n", (uint64_t)command, COMMAND_NAMES[command]);
        else if (compilationMode == BINARY_COMPILATION)
        {
            uint64_t goodSizeCommand = (uint64_t)command;
            fwrite(&goodSizeCommand, sizeof(goodSizeCommand), 1, outFile);

            fprintf(stderr, "No arg command: %llu\n", goodSizeCommand);
        }
        else
            return ERROR_SYNTAX;
        break;
    }

    return EVERYTHING_FINE;
}

uint64_t _translateArgCommandToBinFormat(Command command, ArgType argType)
{
    return ((uint64_t)command | (argType << BITS_FOR_COMMAND));
}

CommandResult _translateRawCommand(uint64_t rawCommand)
{
    Command command = {};

    switch (rawCommand)
    {
    case push_sn:
        command = Push_c;
        break;

    case pop_sn:
        command = Pop_c;
        break;
        
    case in_sn:
        command = In_c;
        break;

    case out_sn:
        command = Out_c;
        break;

    case add_sn:
        command = Add_c;
        break;

    case sub_sn:
        command = Subtract_c;
        break;

    case mul_sn:
        command = Multiply_c;
        break;

    case div_sn:
        command = Divide_c;
        break;

    case sqrt_sn:
        command = Sqrt_c;
        break;

    case sin_sn:
        command = Sin_c;
        break;

    case cos_sn:
        command = Cos_c;
        break;

    case hlt_sn:
        command = Halt_c;
        break;

    default:
        return {Halt_c, ERROR_SYNTAX};
        break;
    }

    return {command, EVERYTHING_FINE};
}