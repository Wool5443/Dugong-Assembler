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

    const char* codeFilePath = "ggg.dug";
    const char* byteCodeFilePath = "byteCode.bin";
    const char* listingFilePath = "listing.txt";

    ErrorCode compileError = Compile(codeFilePath, byteCodeFilePath, listingFilePath);

    if (compileError)
    {
        printf("COMPILE ERROR %s!!!\n", ERROR_CODE_NAMES[compileError]);
        return compileError;
    }

    return EVERYTHING_FINE;
}