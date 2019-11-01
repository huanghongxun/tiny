#include "ast.h"
#include <stdlib.h>

void tiny_ast_add_child(tiny_ast_t *ast, tiny_ast_t *child)
{
    if (!child)
        return;
    tiny_ast_t **ptr = &ast->child;
    while (*ptr)
        ptr = &((*ptr)->sibling);
    *ptr = child;
}

tiny_ast_t *tiny_make_ast(int desc)
{
    tiny_ast_t *ast = malloc(sizeof(tiny_ast_t));
    ast->child = ast->sibling = NULL;
    ast->token.s = ast->token.e = NULL;
    ast->desc = desc;
    return ast;
}

void tiny_free_ast(tiny_ast_t *ast)
{
    if (!ast)
        return;
    tiny_free_ast(ast->child);
    tiny_free_ast(ast->sibling);
    free(ast);
}

size_t tiny_ast_child_count(tiny_ast_t *ast)
{
    size_t cnt = 0;
    for (tiny_ast_t *ptr = ast->child; ptr; ptr = ptr->sibling)
        cnt++;
    return cnt;
}
