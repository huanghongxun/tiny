#ifndef AST_H
#define AST_H

#include "lexical.h"
#include <stddef.h>

struct tiny_ast_s {
    int desc;
    tiny_lex_token_t token;

    struct tiny_ast_s *child;
    struct tiny_ast_s *sibling;
};

typedef struct tiny_ast_s tiny_ast_t;

tiny_ast_t *tiny_make_ast(int desc);

void tiny_free_ast(tiny_ast_t *ast);

void tiny_ast_add_child(tiny_ast_t *ast, tiny_ast_t *child);

size_t tiny_ast_child_count(tiny_ast_t *ast);

#endif // AST_H