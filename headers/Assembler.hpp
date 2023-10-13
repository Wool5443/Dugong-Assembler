#include "Utils.hpp"

typedef unsigned int uint;

enum CompilationMode
{
    DRAFT_COMPILATION,
    BINARY_COMPILATION,
};

ErrorCode Compile(const char* codeFilePath, const char* byteCodeOutPath, CompilationMode compilationMode);
