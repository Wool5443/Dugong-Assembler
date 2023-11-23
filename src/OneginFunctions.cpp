#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "OneginFunctions.hpp"
#include "Utils.hpp"
#include "StringFunctions.hpp"
#include "Sort.hpp"

size_t _countTokens(const char* string, char terminator);

const String* _split(const char* string, size_t numOfTokens, char terminator);

int _stringCompareStartToEnd(const void* s1, const void* s2);

int _stringCompareEndToStart(const void* s1, const void* s2);

Text CreateText(const char* path, char terminator)
{
    MyAssertHard(path, ERROR_NULLPTR);

    Text text = {};

    text.size = GetFileSize(path);
    
    char* rawText = (char*)calloc(text.size + 2, sizeof(char));

    rawText[text.size] = terminator;
    rawText[text.size + 1] = '\0';

    FILE* file = fopen(path, "rb");
    MyAssertHard(file, ERROR_BAD_FILE, );
    MyAssertHard(text.size == fread(rawText, sizeof(char), text.size, file), ERROR_BAD_FILE, );
    fclose(file); 

    text.rawText = rawText;

    text.numberOfTokens = _countTokens(rawText, terminator);

    text.tokens = _split(text.rawText, text.numberOfTokens, terminator);

    return text;
}

void DestroyText(Text* text)
{
    MyAssertHard(text, ERROR_NULLPTR, );
    free((void*)(text->tokens));
    free((void*)(text->rawText));
}

void SortTextTokens(Text* text, StringCompareMethod sortType)
{
    switch (sortType)
    {
        case START_TO_END:
            Sort((void*)(text->tokens), text->numberOfTokens, sizeof((text->tokens)[0]), _stringCompareStartToEnd);
            break;
        case END_TO_START:
            Sort((void*)(text->tokens), text->numberOfTokens, sizeof((text->tokens)[0]), _stringCompareEndToStart);
            break;
        default:
            Sort((void*)(text->tokens), text->numberOfTokens, sizeof((text->tokens)[0]), _stringCompareStartToEnd);
            break;
    }
}

void PrintRawText(const Text* text, FILE* file)
{
    fputs(text->rawText, file);
}

void PrintTextTokens(const Text* text, FILE* file, char terminator)
{
    for (size_t i = 0; i < text->numberOfTokens; i++)
    {
        const char* line = text->tokens[i].text;
        if (*line != terminator)
            StringPrint(file, line, terminator);
    }
}

int _stringCompareStartToEnd(const void* s1, const void* s2)
{
    return StringCompare((String*)s1, (String*)s2, START_TO_END, IGNORE_CASE, IGNORED_SYMBOLS);
}

int _stringCompareEndToStart(const void* s1, const void* s2)
{
    return StringCompare((String*)s1, (String*)s2, END_TO_START, IGNORE_CASE, IGNORED_SYMBOLS);
}

size_t _countTokens(const char* string, char terminator)
{
    MyAssertHard(string, ERROR_NULLPTR, );

    size_t tokens = 1;
    const char* newTokenSymbol = strchr(string, terminator);
    while (newTokenSymbol != NULL)
    {
        tokens++;
        newTokenSymbol = strchr(newTokenSymbol + 1, terminator);
    }
    return tokens;
}

const String* _split(const char* string, size_t numOfTokens, char terminator)
{
    MyAssertHard(string, ERROR_NULLPTR);

    String* textTokens = (String*)calloc(numOfTokens, sizeof(textTokens[0]));

    MyAssertHard(textTokens, ERROR_NO_MEMORY);

    const char* endCurToken = strchr(string, terminator);

    textTokens[0] = {.text = string,
                     .length = (size_t)(endCurToken - string)};

    size_t i = 1;

    while (endCurToken)
    {
        textTokens[i] = {};
        textTokens[i].text = endCurToken + 1;
        endCurToken = strchr(endCurToken + 1, terminator);
        textTokens[i].length = endCurToken ? (size_t)(endCurToken - textTokens[i].text) : 0;
        i++;
    }

    return (const String*)textTokens;
}
