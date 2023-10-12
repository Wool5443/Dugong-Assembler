#include "Assembler.hpp"

int main()
{
    const char* dugFile = "ggg.dug";
    const char* byteCodeFile = "main.bin";

    ErrorCode compileError = Compile(dugFile, byteCodeFile);

    if (compileError)
        fprintf(stderr, "ERROR!!! %s\n", ERROR_CODE_NAMES[compileError]);
    
    return compileError;
}