#include "scanner.h"
#include <stdlib.h>

tiny_lex_token_t tiny_scanner_next(tiny_scanner_t *scanner)
{
    // 如果指向当前 token 的指针没有下一个元素，那么通过 reader 读取
    if (list_next(scanner->cur) == &scanner->tokens)
    {
        tiny_scanner_token_t *token = malloc(sizeof(tiny_scanner_token_t));
        list_init(&token->list);
        scanner->reader(scanner->ctx, &token->token);
        list_add_before(&scanner->tokens, &token->list);
    }

    scanner->cur = list_next(scanner->cur);
    return tiny_scanner_now(scanner)->token;
}

tiny_lex_token_t tiny_scanner_peek(tiny_scanner_t *scanner)
{
    // 如果指向当前 token 的指针没有下一个元素，那么通过 reader 读取
    if (list_next(scanner->cur) == &scanner->tokens)
    {
        tiny_scanner_token_t *token = malloc(sizeof(tiny_scanner_token_t));
        list_init(&token->list);
        scanner->reader(scanner->ctx, &token->token);
        list_add_before(&scanner->tokens, &token->list);
    }

    return le2scannertoken(list_next(scanner->cur), list)->token;
}

tiny_scanner_token_t *tiny_scanner_now(tiny_scanner_t *scanner)
{
    return le2scannertoken(scanner->cur, list);
}

void tiny_scanner_reset(tiny_scanner_t *scanner, tiny_scanner_token_t *token)
{
    scanner->cur = &token->list;
}

int tiny_scanner_diff(tiny_scanner_t *scanner, tiny_scanner_token_t *token)
{
    int dist = 0;
    list_entry_t *entry = &token->list;
    while (entry != scanner->cur && entry->next != &token->list)
    {
        entry = entry->next;
        dist++;
    }
    return dist;
}

void tiny_scanner_begin(tiny_scanner_t *scanner, void *ctx, void (*reader)(void *ctx, tiny_lex_token_t *token))
{
    list_init(&scanner->tokens);
    scanner->cur = &scanner->tokens;
    scanner->ctx = ctx;
    scanner->reader = reader;
}
