#include "parser.h"
#include <ctype.h>
#include <stdarg.h>
#include "string_util.h"
#include <stdlib.h>
#include <assert.h>

#define STATE_SUCCESS 0
#define STATE_ERROR 1

tiny_parser_result_t tiny_syntax_parse(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    return ctx.current_parser->parser(ctx, scanner);
}

tiny_parser_t *tiny_make_parser()
{
    tiny_parser_t *parser = malloc(sizeof(tiny_parser_t));
    parser->parser = NULL;
    parser->sibling = parser->child = NULL;
    parser->token = NULL;
    parser->predicate = NULL;
    parser->desc = 0;
    parser->error = 0;
    return parser;
}

static tiny_parser_ctx_t make_context(struct trie *parsers, tiny_parser_t *current_parser)
{
    tiny_parser_ctx_t ctx = {
        .parsers = parsers,
        .current_parser = current_parser};
    return ctx;
}

static tiny_parser_result_t make_success_result(tiny_ast_t *ast)
{
    tiny_parser_result_t result;
    result.ast = ast;
    result.state = STATE_SUCCESS;
    result.required_token = NULL;
    return result;
}

static tiny_parser_result_t make_failure_result(tiny_lex_token_t token, const char *required_token, int error)
{
    assert(error != 0);
    tiny_parser_result_t result = {
        .ast = NULL,
        .state = error,
        .fatal = false,
        .error_token = token,
        .required_token = required_token};
    return result;
}

static tiny_parser_result_t parser_kleene(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_ast_t *ast = tiny_make_ast(ctx.current_parser->desc);

    while (true)
    {
        tiny_scanner_token_t *save = tiny_scanner_now(scanner);
        tiny_parser_result_t next = tiny_syntax_parse(
            make_context(ctx.parsers, ctx.current_parser->child),
            scanner);

        if (next.state == STATE_SUCCESS)
        {
            // 解析成功，添加子 AST
            tiny_ast_add_child(ast, next.ast);
        }
        else
        {
            if (next.fatal)
                return next;
            tiny_scanner_reset(scanner, save);
            return make_success_result(ast);
        }
    }
}

tiny_parser_t *tiny_make_parser_kleene(tiny_parser_t *single)
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_kleene;
    ret->child = single;
    return ret;
}

static tiny_parser_result_t parser_kleene_until(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_ast_t *ast = tiny_make_ast(ctx.current_parser->desc);
    tiny_parser_result_t one = make_success_result(NULL);

    while (true)
    {
        tiny_scanner_token_t *save = tiny_scanner_now(scanner);
        tiny_parser_result_t next = tiny_syntax_parse(
            make_context(ctx.parsers, ctx.current_parser->child),
            scanner);

        if (next.state == STATE_SUCCESS)
        {
            // 解析成功，添加子 AST
            tiny_ast_add_child(ast, next.ast);
        }
        else
        {
            if (next.fatal)
                return next;
            one = next;
            tiny_scanner_reset(scanner, save);
            break;
        }
    }

    // 检查 terminator 是否存在
    {
        tiny_scanner_token_t *save = tiny_scanner_now(scanner);
        tiny_parser_result_t next = tiny_syntax_parse(
            make_context(ctx.parsers, ctx.current_parser->child->sibling),
            scanner);

        if (next.state == STATE_SUCCESS)
        {
            tiny_free_ast(next.ast);
            tiny_scanner_reset(scanner, save);
            return make_success_result(ast);
        }
        else
        {
            if (next.fatal)
                return next;
            if (one.state != STATE_SUCCESS)
                return one;
            else
                return next;
        }
    }
}

tiny_parser_t *tiny_make_parser_kleene_until(tiny_parser_t *terminator, tiny_parser_t *replica)
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_kleene_until;
    ret->child = replica;
    replica->sibling = terminator;
    return ret;
}

static tiny_parser_result_t parser_or(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_parser_result_t one;
    int diff = -1;
    tiny_scanner_token_t *save = tiny_scanner_now(scanner);
    for (tiny_parser_t *cld = ctx.current_parser->child; cld; cld = cld->sibling)
    {
        tiny_parser_result_t next = tiny_syntax_parse(
            make_context(ctx.parsers, cld),
            scanner);

        if (next.state == STATE_SUCCESS)
        {
            if (ctx.current_parser->desc != 0 && next.ast)
                next.ast->desc = ctx.current_parser->desc;
            return next;
        }
        else
        {
            if (next.fatal)
                return next;
            int mydiff = tiny_scanner_diff(scanner, save);
            if (mydiff > diff)
            {
                diff = mydiff;
                one = next;
            }
            tiny_scanner_reset(scanner, save);
        }
    }
    return one;
}

tiny_parser_t *tiny_make_parser_or(int n, ...)
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_or;
    tiny_parser_t **ptr = &ret->child;

    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i)
    {
        *ptr = va_arg(ap, tiny_parser_t *);
        ptr = &((*ptr)->sibling);
    }
    return ret;
}

static tiny_parser_result_t parser_sequence(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_ast_t *ast = tiny_make_ast(ctx.current_parser->desc);

    for (tiny_parser_t *cld = ctx.current_parser->child; cld; cld = cld->sibling)
    {
        tiny_parser_result_t next = tiny_syntax_parse(
            make_context(ctx.parsers, cld),
            scanner);

        if (next.state == STATE_SUCCESS)
        {
            tiny_ast_add_child(ast, next.ast);
        }
        else
        {
            tiny_free_ast(ast);
            return next;
        }
    }
    if (tiny_ast_child_count(ast) <= 1)
    {
        if (ast->desc != 0 && ast->child)
            ast->child->desc = ast->desc;
        return make_success_result(ast->child);
    }
    else
    {
        return make_success_result(ast);
    }
}

tiny_parser_t *tiny_make_parser_sequence(int n, ...)
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_sequence;
    tiny_parser_t **ptr = &ret->child;

    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i)
    {
        *ptr = va_arg(ap, tiny_parser_t *);
        ptr = &((*ptr)->sibling);
    }
    return ret;
}

static tiny_parser_result_t parser_grammar(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_parser_ctx_t subctx = {
        .parsers = ctx.parsers,
        .current_parser = trie_search(ctx.parsers, ctx.current_parser->token)};

    assert(subctx.current_parser != NULL);
    return tiny_syntax_parse(subctx, scanner);
}

tiny_parser_t *tiny_make_parser_grammar(const char *name)
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_grammar;
    ret->token = name;
    return ret;
}

static tiny_parser_result_t parser_optional(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_scanner_token_t *save = tiny_scanner_now(scanner);
    tiny_ast_t *ast = tiny_make_ast(ctx.current_parser->desc);
    tiny_parser_result_t result = tiny_syntax_parse(
        make_context(ctx.parsers, ctx.current_parser->child),
        scanner);

    if (result.state == STATE_SUCCESS)
        tiny_ast_add_child(ast, result.ast);
    else if (result.fatal)
        return result;
    else
        tiny_scanner_reset(scanner, save);

    return make_success_result(ast);
}

tiny_parser_t *tiny_make_parser_optional(tiny_parser_t *optional)
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_optional;
    ret->child = optional;
    return ret;
}

static tiny_parser_result_t parser_token(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_lex_token_t token = tiny_scanner_next(scanner);
    if (token.error != 0)
        if (token.error == TINY_EOF)
        {
            return make_failure_result(token, ctx.current_parser->token, TINY_UNEXPECTED_EOF);
        }
        else
        {
            tiny_parser_result_t result = make_failure_result(token, ctx.current_parser->token, token.error);
            result.fatal = token.error != TINY_EOF;
            return result;
        }

    if (strsecmp(token.s, token.e, ctx.current_parser->token))
    {
        tiny_ast_t *ast = tiny_make_ast(ctx.current_parser->desc);
        ast->token = token;
        return make_success_result(ast);
    }
    else
    {
        return make_failure_result(token, ctx.current_parser->token, TINY_UNEXPECTED_TOKEN);
    }
}

tiny_parser_t *tiny_make_parser_token(const char *name)
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_token;
    ret->token = name;
    return ret;
}

static tiny_parser_result_t parser_token_eof(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_lex_token_t token = tiny_scanner_next(scanner);
    if (token.error == TINY_EOF)
    {
        return make_success_result(NULL);
    }
    else if (token.error != 0)
    {
        tiny_parser_result_t result = make_failure_result(token, ctx.current_parser->token, token.error);
        result.fatal = token.error != TINY_EOF;
        return result;
    }
    else
    {
        return make_failure_result(token, ctx.current_parser->token, TINY_EOF);
    }
}

tiny_parser_t *tiny_make_parser_token_eof()
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_token_eof;
    return ret;
}

static tiny_parser_result_t parser_token_ignore_case(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_lex_token_t token = tiny_scanner_next(scanner);
    if (token.error)
    {
        tiny_parser_result_t result = make_failure_result(token, ctx.current_parser->token, token.error);
        result.fatal = token.error != TINY_EOF;;
        return result;
    }
    const char *s = token.s, *e = token.e, *t = ctx.current_parser->token;
    const char *i;
    for (i = s; i != e && *t; ++i, ++t)
        if (isalpha(*i) && tolower(*i) != tolower(*t) || !isalpha(*i) && *i != *t)
            break;
    if (i == e && !*t)
    {
        tiny_ast_t *ast = tiny_make_ast(ctx.current_parser->desc);
        ast->token = token;
        return make_success_result(ast);
    }
    else
    {
        return make_failure_result(token, ctx.current_parser->token, TINY_UNEXPECTED_TOKEN);
    }
}

tiny_parser_t *tiny_make_parser_token_ignore_case(const char *name)
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_token_ignore_case;
    ret->token = name;
    return ret;
}

static tiny_parser_result_t parser_token_predicate(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_lex_token_t token = tiny_scanner_next(scanner);
    if (token.error)
    {
        tiny_parser_result_t result = make_failure_result(token, ctx.current_parser->token, token.error);
        result.fatal = token.error != TINY_EOF;
        return result;
    }

    if (ctx.current_parser->predicate(token.s, token.e))
    {
        tiny_ast_t *ast = tiny_make_ast(ctx.current_parser->desc);
        ast->token = token;
        return make_success_result(ast);
    }
    else
    {
        return make_failure_result(token, ctx.current_parser->token, TINY_UNEXPECTED_TOKEN);
    }
}

tiny_parser_t *tiny_make_parser_token_predicate(bool (*predicate)(const char *, const char *e))
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_token_predicate;
    ret->predicate = predicate;
    return ret;
}

static tiny_parser_result_t parser_separation(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_ast_t *ast = tiny_make_ast(ctx.current_parser->desc);
    bool first = true;

    while (true)
    {
        {
            tiny_scanner_token_t *save = tiny_scanner_now(scanner);
            tiny_parser_result_t next = tiny_syntax_parse(
                make_context(ctx.parsers, ctx.current_parser->child),
                scanner);

            if (next.state == STATE_SUCCESS)
            {
                // 解析成功，添加子 AST
                tiny_ast_add_child(ast, next.ast);
            }
            else
            {
                if (next.fatal)
                    return next;
                if (first)
                {
                    tiny_scanner_reset(scanner, save);
                    return make_success_result(ast);
                }
                else
                {
                    tiny_free_ast(ast);
                    return next;
                }
            }
        }

        {
            tiny_scanner_token_t *save = tiny_scanner_now(scanner);
            tiny_parser_result_t next = tiny_syntax_parse(
                make_context(ctx.parsers, ctx.current_parser->child->sibling),
                scanner);

            if (next.state == STATE_SUCCESS)
            {
                tiny_ast_add_child(ast, next.ast);
            }
            else
            {
                if (next.fatal)
                    return next;
                tiny_scanner_reset(scanner, save);
                return make_success_result(ast);
            }
        }

        first = false;
    }
}

tiny_parser_t *tiny_make_parser_separation(tiny_parser_t *separator, tiny_parser_t *replica)
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_separation;
    ret->child = replica;
    replica->sibling = separator;
    return ret;
}

static tiny_parser_result_t parser_eliminate(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_parser_result_t result = tiny_syntax_parse(
        make_context(ctx.parsers, ctx.current_parser->child),
        scanner);
    if (result.ast)
    {
        tiny_free_ast(result.ast);
        result.ast = NULL;
    }
    return result;
}

tiny_parser_t *tiny_make_parser_eliminate(tiny_parser_t *parser)
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_eliminate;
    ret->child = parser;
    return ret;
}

static tiny_parser_result_t parser_with_desc(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_parser_result_t result = tiny_syntax_parse(
        make_context(ctx.parsers, ctx.current_parser->child),
        scanner);
    if (result.ast)
        result.ast->desc = ctx.current_parser->desc;
    return result;
}

tiny_parser_t *tiny_make_parser_with_desc(int desc, tiny_parser_t *parser)
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_with_desc;
    ret->desc = desc;
    ret->child = parser;
    return ret;
}

static tiny_parser_result_t parser_error(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_parser_result_t result = tiny_syntax_parse(
        make_context(ctx.parsers, ctx.current_parser->child),
        scanner);
    if (result.state == STATE_ERROR)
    {
        assert(ctx.current_parser->error != 0);
        result.state = ctx.current_parser->error;
    }

    return result;
}

tiny_parser_t *tiny_make_parser_error(int error, tiny_parser_t *parser)
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_error;
    ret->error = error;
    ret->child = parser;
    return ret;
}

static tiny_parser_result_t parser_fatal(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner)
{
    tiny_parser_result_t result = tiny_syntax_parse(
        make_context(ctx.parsers, ctx.current_parser->child),
        scanner);
    if (result.state != STATE_SUCCESS)
        result.fatal = true;

    if (result.state == STATE_ERROR)
    {
        assert(ctx.current_parser->error != 0);
        result.state = ctx.current_parser->error;
    }

    return result;
}

tiny_parser_t *tiny_make_parser_fatal(int error, tiny_parser_t *parser)
{
    tiny_parser_t *ret = tiny_make_parser();
    ret->parser = parser_fatal;
    ret->error = error;
    ret->child = parser;
    return ret;
}
