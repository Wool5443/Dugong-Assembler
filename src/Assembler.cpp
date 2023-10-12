#include <string.h>
#include <ctype.h>
#include "Assembler.hpp"
#include "OneginFunctions.hpp"
#include "Stack.hpp"
#include "SPUcommands.settings"

enum STRING_AS_NUM_snOMMANDS
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

CommandResult _translateRawCommand(uint rawCommand);

ErrorCode _writeCommand(FILE* outFile, const String* curLine, Command command, int commandLength);

ErrorCode Compile(const char* codeFilePath, const char* byteCodeOutPath)
{
    MyAssertSoft(codeFilePath, ERROR_NULLPTR);

    FILE* outFile = fopen(byteCodeOutPath, "w");

    MyAssertSoft(outFile, ERROR_BAD_FILE);

    Text code = CreateText(codeFilePath, '\n');

    for (size_t position = 0; position < code.numberOfLines; position++)
    {
        const String* curLine = &(code.lines[position]);

        char* endLinePtr = (char*)strchr(curLine->text, '\n');
        if (endLinePtr)
            *endLinePtr = 0;

        char* commentPtr = (char*)strchr(curLine->text, ';');
        if (commentPtr)
            *commentPtr = 0;

        if (StringIsEmptyChars(curLine))
            continue;

        uint rawCommand = 0;

        int commandLength = 0;

        if (sscanf(curLine->text, "%4s%n", (char*)&rawCommand, &commandLength) != 1)
        {
            DestroyText(&code);
            return ERROR_SYNTAX;
        }



        CommandResult resCom = _translateRawCommand(rawCommand);

        if (resCom.error)
        {
            DestroyText(&code);
            return resCom.error;
        }

        Command command = resCom.command;

        ErrorCode writeError = _writeCommand(outFile, curLine, command, commandLength);
        
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

ErrorCode _writeCommand(FILE* outFile, const String* curLine, Command command, int commandLength)
{
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

                if (!StringIsEmptyChars(curLine->text + commandLength + argLength + 2, 0))
                    return ERROR_SYNTAX;
                
                fprintf(outFile, "%u %u %u\n", (uint)command, (uint)RegisterArg, regType - 'a' + 1);
            }
            else
            {
                fprintf(outFile, "%u %u " STACK_EL_SPECIFIER "\n", (uint)command, (uint)ImmediateNumberArg, (StackElement_t)arg);
            }
            break;
        }
    
    default:
        fprintf(outFile, "%u\n", (uint)command);
        break;
    }

    return EVERYTHING_FINE;
}

CommandResult _translateRawCommand(uint rawCommand)
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