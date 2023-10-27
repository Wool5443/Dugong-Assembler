#include <string.h>
#include "Assembler.hpp"
#include "Utils.hpp"

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Please, give input and output files.\n");

        return ERROR_BAD_FILE;
    }

    const char* codeFilePath = argv[1];
    const char* byteCodeFilePath = argv[2];
    char* listingFilePath = (char*)calloc(strlen(byteCodeFilePath) + sizeof("_listing.txt"), 1);
    strcpy(listingFilePath, byteCodeFilePath);
    strcat(listingFilePath, "_listing.txt");

    ErrorCode compileError = Compile(codeFilePath, byteCodeFilePath, listingFilePath);

    free(listingFilePath);

    if (compileError)
    {
        printf("COMPILE ERROR %s!!!\n", ERROR_CODE_NAMES[compileError]);
        return compileError;
    }

    return EVERYTHING_FINE;
}