#include "syntax_def.h"
#include "parser.h"
#include <ctype.h>
#include "string_util.h"

#define DEFINE(name, descriptor, parser) \
    {                                    \
        tiny_parser_t *p = parser;       \
        trie_insert(parsers, #name, p);  \
        p->desc = descriptor;            \
    }

static bool is_identifier(const char *s, const char *e)
{
    if (!isalpha(*s) && *s != '_')
        return false;
    for (const char *i = s + 1; i != e; ++i)
        if (!isalpha(*s) && !isdigit(*s) && *s != '_')
            return false;
    return true;
}

static bool is_string(const char *s, const char *e)
{
    return s < e && s + 1 < e && *s == '"' && *(e - 1) == '"';
}

static bool is_character(const char *s, const char *e)
{
    return s < e && s + 1 < e && *s == '\'' && *(e - 1) == '\'';
}

static bool is_number(const char *s, const char *e)
{
    if (s >= e)
        return false;
    if (*s == '0' && s + 1 < e && tolower(*(s + 1)) == 'x') // 十六进制整数
    {
        s += 2;
        bool has_digit = false;
        while (s < e && is_hex_digit(*s))
            s++, has_digit = true;
        return has_digit && s >= e;
    }
    else if (*s == '0' && s + 1 < e) // 八进制整数
    {
        s++;
        bool has_digit = false;
        while (s < e && '0' <= *s && *s < '8')
            s++, has_digit = true;
        return has_digit && s >= e;
    }
    else
    {
        bool before_dot = false, dot = false, after_dot = false;
        while (s < e && isdigit(*s))
            s++, before_dot = true; // 跳过小数点前的数字位
        if (s < e && *s == '.')     // 如果数字位后是小数点，则跳过
        {
            s++; // 跳过小数点后的数字
            dot = true;
            while (s < e && isdigit(*s))
                s++, after_dot = true;
        }
        if (!before_dot && (!dot || dot && !after_dot))
            return false;
        if (s < e && tolower(*s) == 'e')
        {
            s++;
            if (s >= e)
                return false;
            if (*s == '+' || *s == '-')
                s++;
            if (s >= e)
                return false;
            bool has_digit = false;
            while (s < e && isdigit(*s))
                s++, has_digit = true;
            if (!has_digit)
                return false;
        }
        return true;
    }
}

struct trie *prepare_parsers()
{
    struct trie *parsers = trie_create();
    // root -> (func | vars)*
    DEFINE(
        root,
        TINY_DESC_ROOT,
        KLEENE_UNTIL(
            TOKEN_EOF,
            OR(
                GRAMMAR(func),
                GRAMMAR(vars))));
    // func -> type ['MAIN'] identifier '(' formal_params ')' block
    DEFINE(
        func,
        TINY_DESC_FUNC,
        SEQUENCE(
            GRAMMAR(type),
            WITH_DESC(TINY_DESC_MAIN, OPTIONAL(TOKEN_IGNORE_CASE("main"))),
            GRAMMAR(identifier),
            TOKEN("("),
            GRAMMAR(formal_params),
            TOKEN(")"),
            GRAMMAR(block)));
    // vars -> type identifier (',' identifier)* ';'
    DEFINE(
        vars,
        TINY_DESC_DECL,
        SEQUENCE(
            GRAMMAR(type),
            SEPARATION(
                TOKEN(","),
                GRAMMAR(identifier)),
            TOKEN(";")));
    // type -> 'int' | 'type'
    DEFINE(
        type,
        TINY_DESC_TYPE,
        ERROR(TINY_EXPECT_TYPE, OR(
                                    TOKEN_IGNORE_CASE("int"),
                                    TOKEN_IGNORE_CASE("real"))));
    // identifier -> (alpha | '_') (alpha | digit | '_')*
    DEFINE(
        identifier,
        TINY_DESC_IDENTIFIER,
        ERROR(TINY_EXPECT_IDENTIFIER, TOKEN_PREDICATE(is_identifier)));
    // formal_params = formal_param (',' formal_param)*
    DEFINE(
        formal_params,
        TINY_DESC_FORMAL_PARAMS,
        SEPARATION(
            TOKEN(","),
            GRAMMAR(formal_param)));
    // formal_param -> type identifier
    DEFINE(
        formal_param,
        TINY_DESC_FORMAL_PARAM,
        SEQUENCE(
            GRAMMAR(type),
            GRAMMAR(identifier)));
    // block -> 'BEGIN' statement* 'END'
    DEFINE(
        block,
        TINY_DESC_BLOCK,
        SEQUENCE(
            ERROR(TINY_EXPECT_BEGIN, TOKEN("BEGIN")),
            KLEENE_UNTIL(TOKEN("END"), GRAMMAR(statement)),
            ERROR(TINY_EXPECT_END, TOKEN("END"))));
    // statement -> block | vars | expression ';' | return | if
    DEFINE(
        statement,
        TINY_DESC_STATEMENT,
        ERROR(TINY_EXPECT_STATEMENT, OR(
                                         GRAMMAR(block),
                                         GRAMMAR(vars),
                                         SEQUENCE(GRAMMAR(expression), TOKEN(";")),
                                         GRAMMAR(return ),
                                         GRAMMAR(if))));
    DEFINE(string, TINY_DESC_STRING, TOKEN_PREDICATE(is_string));
    DEFINE(character, TINY_DESC_CHAR, TOKEN_PREDICATE(is_character));
    // Number -> NonZeroDigit Digits | NonZeroDigit Digits '.' Digits | '0x' HexDigits | '0' OctDigits
    DEFINE(number, TINY_DESC_NUMBER, TOKEN_PREDICATE(is_number));
    // if -> 'if' '(' expression ')' statement ['else' statement]
    DEFINE(
        if,
        TINY_DESC_IF,
        SEQUENCE(
            TOKEN_IGNORE_CASE("if"),
            ERROR(TINY_EXPECT_LEFT_PARENTHESIS, TOKEN("(")),
            GRAMMAR(expression),
            ERROR(TINY_EXPECT_RIGHT_PARENTHESIS, TOKEN(")")),
            GRAMMAR(statement),
            OPTIONAL(
                SEQUENCE(
                    TOKEN_IGNORE_CASE("else"),
                    GRAMMAR(statement)))));
    // return -> 'return' expression ';'
    DEFINE(
        return,
        TINY_DESC_RETURN,
        SEQUENCE(
            TOKEN_IGNORE_CASE("return"),
            GRAMMAR(expression),
            TOKEN(";")));
    // actual_params -> expression (',' expression)*
    DEFINE(
        actual_params,
        TINY_DESC_ACTUAL_PARAMS,
        SEPARATION(
            ERROR(TINY_EXPECT_COMMA, TOKEN(",")),
            GRAMMAR(expression)));
    // unit0 -> '(' expression ')' | call | number | identifier | string | character
    DEFINE(
        unit0,
        TINY_DESC_ELIMINATE,
        OR(
            SEQUENCE(
                TOKEN("("),
                GRAMMAR(expression),
                ERROR(TINY_EXPECT_RIGHT_PARENTHESIS, TOKEN(")"))),
            GRAMMAR(call),
            GRAMMAR(number),
            GRAMMAR(identifier),
            GRAMMAR(string),
            GRAMMAR(character)));
    // call -> identifier '(' actual_params ')'
    DEFINE(
        call,
        TINY_DESC_CALL,
        SEQUENCE(
            GRAMMAR(identifier),
            ERROR(TINY_MAY_FUNC_CALL, TOKEN("(")),
            GRAMMAR(actual_params),
            ERROR(TINY_EXPECT_RIGHT_PARENTHESIS, TOKEN(")"))));
    DEFINE(
        unit2,
        TINY_DESC_BINARY,
        SEPARATION(
            OR(
                TOKEN("*"),
                TOKEN("/")),
            GRAMMAR(unit0)));
    DEFINE(
        unit3,
        TINY_DESC_BINARY,
        SEPARATION(
            OR(
                TOKEN("+"),
                TOKEN("-")),
            GRAMMAR(unit2)));
    DEFINE(
        unit4,
        TINY_DESC_BINARY,
        SEPARATION(
            OR(
                TOKEN("=="),
                TOKEN("!=")),
            GRAMMAR(unit3)));
    DEFINE(
        unit5,
        TINY_DESC_ASSIGN,
        SEPARATION(
            TOKEN(":="),
            GRAMMAR(unit4)));
    DEFINE(
        expression,
        TINY_DESC_ELIMINATE,
        GRAMMAR(unit5));
    return parsers;
}
