#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"
#include "lexical.h"
#include "parser.h"
#include "syntax_def.h"

#define BUF_SIZE 1024

char *read_all(FILE *stream)
{
    size_t len;
    size_t content_len = 0;
    size_t content_size = BUF_SIZE;
    char *content = malloc(content_size);
    if (!content)
    {
        perror("not enough memory");
        exit(2);
    }
    // TODO: 需要处理 buf 大小不足的问题
    while (len = fread(content + content_len, sizeof(char), BUF_SIZE, stream))
    {
        content_len += len;
        while (content_size - (content_len + 1) < BUF_SIZE)
        {
            content = realloc(content, content_size *= 2);
            if (!content)
            {
                perror("not enough memory");
                exit(2);
            }
        }
    }

    content[content_len] = '\0';
    return content;
}

static void print_token(tiny_lex_token_t token, FILE *stream)
{
    for (const char *c = token.s; c != token.e; ++c)
        fputc(*c, stream);
}

static void print_error_message(tiny_lex_token_t *token, const char *message)
{
    fprintf(stderr, "%d:%d: error: ", token->line_number, token->line_column);
    char t = *((char *)token->e);
    *((char *)token->e) = 0;
    fprintf(stderr, message, token->s);
    *((char *)token->e) = t;
    putchar('\n');
    print_token(tiny_lex_current_line(token), stderr);
    putchar('\n');
    for (int i = 0; i < token->line_column; ++i)
        putchar(' ');
    putchar('^');
    putchar('\n');
}

static void error(tiny_lex_token_t *token, int ret)
{
    if (ret == TINY_UNEXPECTED_EOF)
    {
        print_error_message(token, "Unexpected EOF");
        return;
    }
    else if (ret == TINY_UNEXPECTED_TOKEN)
    {
        print_error_message(token, "Unexpected token");
        return;
    }
    else if (ret == TINY_INVALID_STRING)
    {
        print_error_message(token, "Invalid string literal");
        return;
    }
    else if (ret == TINY_INVALID_STRING_X_NO_FOLLOWING_HEX_DIGITS)
    {
        print_error_message(token, "\\x used with no following hex digits");
        return;
    }
    else if (ret == TINY_INVALID_NUMBER)
    {
        print_error_message(token, "Invalid number literal");
        return;
    }
    else if (ret == TINY_EXPECT_SEMICOLON)
    {
        print_error_message(token, "Expect ';', but found '%s'");
        return;
    }
    else if (ret == TINY_EXPECT_LEFT_PARENTHESIS)
    {
        print_error_message(token, "Expect '(', but found '%s'");
        return;
    }
    else if (ret == TINY_EXPECT_RIGHT_PARENTHESIS)
    {
        print_error_message(token, "Expect ')', but found '%s'");
        return;
    }
    else if (ret == TINY_EXPECT_BEGIN)
    {
        print_error_message(token, "Expect 'BEGIN', but found '%s'");
        return;
    }
    else if (ret == TINY_EXPECT_END)
    {
        print_error_message(token, "Expect 'END', but found '%s'");
        return;
    }
    else if (ret == TINY_EXPECT_IDENTIFIER)
    {
        print_error_message(token, "Expect an identifier, but found '%s'");
        return;
    }
    else if (ret == TINY_EXPECT_STATEMENT)
    {
        print_error_message(token, "Expect a statement, but found '%s'");
        return;
    }
    else if (ret == TINY_EXPECT_EXPRESSION)
    {
        print_error_message(token, "Expect a statement, but found '%s'");
        return;
    }
    else if (ret == TINY_EXPECT_TYPE)
    {
        print_error_message(token, "Expect a statement, but found '%s'");
        return;
    }
    else if (ret == TINY_EXPECT_COMMA)
    {
        print_error_message(token, "Expect ',', but found '%s'");
        return;
    }
    else if (ret == TINY_EXPECT_FUNC_VARS)
    {
        print_error_message(token, "Expect function or variable declaration");
        return;
    }
    else if (ret == TINY_MAY_FUNC_CALL)
    {
        print_error_message(token, "Unexpected token '%s', maybe you want a func call?");
        return;
    }
}

void lex_reader(void *ctx, tiny_lex_token_t *token)
{
    static FILE *tokens = NULL;
    if (!tokens)
        tokens = fopen("tokens.txt", "w");

    int ret = tiny_lex_next(ctx, token);
    token->error = ret;
    if (ret >= 0)
    {
        print_token(*token, tokens);
        fputc('\n', tokens);
    }
    else if (ret == TINY_EOF)
    {
        return;
    }
    else
    {
        error(token, ret);
    }
}

void print_ast(tiny_ast_t *ast, int indent, FILE *stream)
{
    for (int i = 0; i < indent; ++i)
        fprintf(stream, "  ");
    switch (ast->desc)
    {
    case TINY_DESC_ELIMINATE:
        fprintf(stream, "-");
        break;
    case TINY_DESC_UNARY:
        fprintf(stream, "unary");
        break;
    case TINY_DESC_BINARY:
        fprintf(stream, "binary");
        break;
    case TINY_DESC_IF:
        fprintf(stream, "if");
        break;
    case TINY_DESC_WHILE:
        fprintf(stream, "while");
        break;
    case TINY_DESC_FOR:
        fprintf(stream, "for");
        break;
    case TINY_DESC_FUNC:
        fprintf(stream, "func");
        break;
    case TINY_DESC_NUMBER:
        fprintf(stream, "number");
        break;
    case TINY_DESC_CALL:
        fprintf(stream, "call");
        break;
    case TINY_DESC_DECL:
        fprintf(stream, "vars");
        break;
    case TINY_DESC_ASSIGN:
        fprintf(stream, "assignment");
        break;
    case TINY_DESC_ROOT:
        fprintf(stream, "root");
        break;
    case TINY_DESC_TYPE:
        fprintf(stream, "type ");
        break;
    case TINY_DESC_IDENTIFIER:
        fprintf(stream, "id");
        break;
    case TINY_DESC_FORMAL_PARAMS:
        fprintf(stream, "formal_params");
        break;
    case TINY_DESC_FORMAL_PARAM:
        fprintf(stream, "formal_param");
        break;
    case TINY_DESC_BLOCK:
        fprintf(stream, "block");
        break;
    case TINY_DESC_STATEMENT:
        fprintf(stream, "statement");
        break;
    case TINY_DESC_STRING:
        fprintf(stream, "string");
        break;
    case TINY_DESC_ACTUAL_PARAMS:
        fprintf(stream, "params");
        break;
    case TINY_DESC_RETURN:
        fprintf(stream, "return");
        break;
    case TINY_DESC_EXPR:
        fprintf(stream, "expression");
        break;
    case TINY_DESC_MAIN:
        fprintf(stream, "main");
        break;
    case TINY_DESC_CHAR:
        fprintf(stream, "char");
        break;
    }
    fprintf(stream, " ");
    print_token(ast->token, stream);
    fprintf(stream, "\n");
    if (ast->child)
        print_ast(ast->child, indent + 1, stream);
    if (ast->sibling)
        print_ast(ast->sibling, indent, stream);
}

int main(int argc, char **argv)
{
    if (argc <= 1)
    {
        perror("you should specify code file path");
        exit(1);
    }

    FILE *code_file = fopen(argv[1], "r");
    FILE *astfile = fopen("ast.txt", "w");
    if (!code_file)
    {
        perror("file not found");
        exit(2);
    }

    char *code = read_all(code_file);

    tiny_lex_t lex;
    tiny_lex_begin(&lex, code);

    tiny_scanner_t scanner;
    tiny_scanner_begin(&scanner, &lex, lex_reader);

    tiny_parser_ctx_t ctx;
    ctx.parsers = prepare_parsers();
    ctx.current_parser = trie_search(ctx.parsers, "root");
    tiny_parser_result_t result = tiny_syntax_parse(ctx, &scanner);
    if (result.state == 0)
    {
        print_ast(result.ast, 0, astfile);
    }
    else
    {
        error(&result.error_token, result.state);
    }
    
    return 0;
}
