#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "scanner.h"
#include "trie.h"
#include "ast.h"
#include "defs.h"

// 克林闭包，表示 replica*
#define KLEENE(replica) tiny_make_parser_kleene(replica)
// 克林闭包，匹配到 terminator，表示 replica*
#define KLEENE_UNTIL(terminator, replica) tiny_make_parser_kleene_until(terminator, replica)
// 或，表示 a | b | c | ... | ...
#define OR(...) tiny_make_parser_or(PP_NARG(__VA_ARGS__), __VA_ARGS__)
// 连接，表示 a b c ...
#define SEQUENCE(...) tiny_make_parser_sequence(PP_NARG(__VA_ARGS__), __VA_ARGS__)
// 表示引用产生式，如引用特定的产生式 S
#define GRAMMAR(name) tiny_make_parser_grammar(#name)
// 可选，表示 [optional]
#define OPTIONAL(optional) tiny_make_parser_optional(optional)
// 匹配字符，EBNF 中表示匹配一个 token 'token'
#define TOKEN(token) tiny_make_parser_token(token)
// 匹配字符且忽略大小写，EBNF 中表示匹配一个 token 'token'
#define TOKEN_IGNORE_CASE(token) tiny_make_parser_token_ignore_case(token)
// 匹配 EOF
#define TOKEN_EOF tiny_make_parser_token_eof()
// 匹配指定字符
#define TOKEN_PREDICATE(predicate) tiny_make_parser_token_predicate(predicate)
// 表示 EBNF 中类似 a (b a)*，即 a 之间用 b 隔开
#define SEPARATION(separator, replica) tiny_make_parser_separation(separator, replica)
// 表示丢弃产生式 parser 所产生的语法树
#define ELIMINATE(parser) tiny_make_parser_eliminate(parser)
// 表示修改产生式 parser 的语法树所表示的语义标记
#define WITH_DESC(desc, parser) tiny_make_parser_with_desc(desc, parser)
// 表示修改产生式 parser 失败码
#define ERROR(error, parser) tiny_make_parser_error(error, parser)
// 表示表达式失败时直接导致整个 AST 解析失败
#define FATAL(error, parser) tiny_make_parser_fatal(error, parser)

struct tiny_parser_token_seq_s {
    tiny_lex_token_t token;

    struct tiny_parser_token_seq_s *next;
};

struct tiny_parser_s;

struct tiny_parser_result_s {
    tiny_ast_t *ast;
    int state;
    bool fatal;
    tiny_lex_token_t error_token;
    const char *required_token;
};

struct tiny_parser_ctx_s {
    struct trie *parsers;
    struct tiny_parser_s *current_parser;
};

struct tiny_parser_s {
    struct tiny_parser_result_s (*parser)(struct tiny_parser_ctx_s, tiny_scanner_t *);
    struct tiny_parser_s *sibling;
    struct tiny_parser_s *child;
    const char *token;
    int desc;
    int error;
    bool (*predicate)(const char *s, const char *e);
};

typedef struct tiny_parser_result_s tiny_parser_result_t;
typedef struct tiny_parser_ctx_s tiny_parser_ctx_t;
typedef struct tiny_parser_s tiny_parser_t;

tiny_parser_t *tiny_make_parser();
tiny_parser_t *tiny_make_parser_kleene(tiny_parser_t *replica);
tiny_parser_t *tiny_make_parser_kleene_until(tiny_parser_t *terminator, tiny_parser_t *replica);
tiny_parser_t *tiny_make_parser_or(int n, ...);
tiny_parser_t *tiny_make_parser_sequence(int n, ...);
tiny_parser_t *tiny_make_parser_grammar(const char *name);
tiny_parser_t *tiny_make_parser_optional(tiny_parser_t *optional);
tiny_parser_t *tiny_make_parser_token(const char *name);
tiny_parser_t *tiny_make_parser_token_eof();
tiny_parser_t *tiny_make_parser_token_ignore_case(const char *name);
tiny_parser_t *tiny_make_parser_token_predicate(bool (*predicate)(const char *, const char *));
tiny_parser_t *tiny_make_parser_separation(tiny_parser_t *separator, tiny_parser_t *replica);
tiny_parser_t *tiny_make_parser_eliminate(tiny_parser_t *parser);
tiny_parser_t *tiny_make_parser_with_desc(int desc, tiny_parser_t *parser);
tiny_parser_t *tiny_make_parser_error(int error, tiny_parser_t *parser);
tiny_parser_t *tiny_make_parser_fatal(int error, tiny_parser_t *parser);

void tiny_syntax_next_token(tiny_parser_ctx_t *machine, tiny_lex_token_t token);

tiny_parser_result_t tiny_syntax_parse(tiny_parser_ctx_t ctx, tiny_scanner_t *scanner);

#endif // PARSER_H