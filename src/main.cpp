#include "Assembler.hpp"
#include "Utils.hpp"

int main(int argc, const char* argv[])
{
    // if (argc < 2)
    // {
    //     SetConsoleColor(stderr, COLOR_RED);
    //     fprintf(stderr, "Please, give input and output files.\n");
    //     SetConsoleColor(stderr, COLOR_WHITE);

    //     return ERROR_BAD_FILE;
    // }

    const char* outFile = "byteCode.bin";

    if (argc == 3)
        outFile = argv[2];
    
    ErrorCode binaryCompileError = Compile("ggg.dug", outFile, BINARY_COMPILATION);
    // ErrorCode binaryCompileError = Compile(argv[1], outFile, BINARY_COMPILATION);

    if (binaryCompileError)
    {
        fprintf(stderr, "BINARY COMPILE ERROR!!! %s\n", ERROR_CODE_NAMES[binaryCompileError]);
        return binaryCompileError;
    }

    ErrorCode draftCompileError = Compile("ggg.dug", "draftCompilation.draft", DRAFT_COMPILATION);
    // ErrorCode draftCompileError = Compile(argv[1], "draftCompilation.draft", DRAFT_COMPILATION);

    if (draftCompileError)
    {
        fprintf(stderr, "DRAFT COMPILE ERROR!!! %s\n", ERROR_CODE_NAMES[draftCompileError]);
        return draftCompileError;
    }
}