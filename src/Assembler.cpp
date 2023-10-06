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

ErrorCode _writeCommand(FILE* outFile, String* curLine, Command command, uint commandLength);

ErrorCode Compile(const char* codeFilePath, const char* byteCodeOutPath)
{
    MyAssertSoft(codeFilePath, ERROR_NULLPTR);

    FILE* outFile = fopen(byteCodeOutPath, "w");

    MyAssertSoft(outFile, ERROR_BAD_FILE);

    Text code = CreateText(codeFilePath, '\n');

    for (size_t ip = 0; ip < code.numberOfLines; ip++)
    {
        String curLine = code.lines[ip];

        if (*curLine.text == '\n')
            continue;

        *(char*)strchr(curLine.text, '\n') = 0;

        char* commentPtr = (char*)strchr(curLine.text, ';');
        *commentPtr = 0;

        uint rawCommand = 0;

        uint commandLength = 0;

        if (sscanf(curLine.text, "%4s%n", (char*)&rawCommand, &commandLength) != 2)
        {
            DestroyText(&code);
            return ERROR_SYNTAX;
        }

        CommandResult resCom = _translateRawCommand(rawCommand);

        RETURN_ERROR(resCom.error);

        Command command = resCom.command;

        RETURN_ERROR(_writeCommand(outFile, &curLine, command, commandLength));
    }

    return EVERYTHING_FINE;
}

ErrorCode _writeCommand(FILE* outFile, String* curLine, Command command, uint commandLength)
{
    switch (command)
    {
    case Push_c:
    case Pop_c:
        double arg = {};
        if (sscanf(curLine->text + commandLength, "%lg", &arg) == 0)
        {
            char regType = 0;
            uint readChars = 0;
            if (sscanf(curLine->text + commandLength, "r%cx%n", &regType, &readChars) != 2 ||
                    !isspace(curLine->text[commandLength + readChars]))
                return ERROR_SYNTAX;
            
            fprintf(outFile, "%u %u " STACK_EL_SPECIFIER "\n", (uint)command, (uint)RegisterArg, regType - 'a' + 1);
        }
        break;
    
    default:
        break;
    }
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