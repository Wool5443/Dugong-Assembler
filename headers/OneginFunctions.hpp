//! @file

#ifndef ONEGIN_FUNCTIONS_HPP
#define ONEGIN_FUNCTIONS_HPP

#include <stddef.h>
#include "StringFunctions.hpp"

/**
 * @brief Characters to ignore in texts.
*/
static const char* const IGNORED_SYMBOLS = " ,.;:'\"-!?`~()[]{}";

/** @struct Text
 * @brief Text struct which contains text itself and tokens of the text.
 * 
 * @var Text::rawText - text.
 * @var Text::tokens - const pointers to tokens in rawText.
 * @var Text::size - number of bytes in text.
 * @var Text::numberOfTokens - the length of tokens.
*/
struct Text
{
    const char* rawText;
    const String* tokens;
    size_t size;
    size_t numberOfTokens;
};

/**
 * @brief Creates a Text member and reads its contents from file.
 * 
 * @param [in] path - the path to a file.
 * 
 * @return Text.
*/
Text CreateText(const char* path, char terminator);

/**
 * @brief Frees all text's memory.
 * 
 * @param [in] text - pointer to the text to destroy.
*/
void DestroyText(Text* text);

/**
 * @brief Sorts text's tokens. Uses quick sort.
 * 
 * @param [in] text - the text tokens of which are to be sorted.
 * @param [in] sortType - the way to compare tokens.
*/
void SortTextTokens(Text* text, StringCompareMethod sortType);

/**
 * @brief Prints raw text form text to a file.
 * 
 * @param [in] text - what to write.
 * @param [in, out] file - where to write.
*/
void PrintRawText(const Text* text, FILE* file);

/**
 * @brief Prints text's line to file, ignoring \\n's.
 * 
 * @param[in] text - what to write.
 * @param [in, out] file - where to write.
*/
void PrintTextTokens(const Text* text, FILE* file, char terminator);

#endif
