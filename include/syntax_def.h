#ifndef SYNTAX_DEF_H
#define SYNTAX_DEF_H

#include "trie.h"

#define TINY_DESC_ELIMINATE 0
#define TINY_DESC_UNARY 1
#define TINY_DESC_BINARY 2
#define TINY_DESC_IF 3
#define TINY_DESC_WHILE 4
#define TINY_DESC_FOR 5
#define TINY_DESC_FUNC 6
#define TINY_DESC_NUMBER 7
#define TINY_DESC_CALL 8
#define TINY_DESC_DECL 9
#define TINY_DESC_ASSIGN 10
#define TINY_DESC_ROOT 11
#define TINY_DESC_TYPE 12
#define TINY_DESC_IDENTIFIER 13
#define TINY_DESC_FORMAL_PARAMS 14
#define TINY_DESC_FORMAL_PARAM 15
#define TINY_DESC_BLOCK 16
#define TINY_DESC_STATEMENT 17
#define TINY_DESC_STRING 18
#define TINY_DESC_ACTUAL_PARAMS 19
#define TINY_DESC_RETURN 20
#define TINY_DESC_EXPR 21
#define TINY_DESC_MAIN 22
#define TINY_DESC_CHAR 23

struct trie *prepare_parsers();

#endif // SYNTAX_DEF_H