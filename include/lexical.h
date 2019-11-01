#ifndef LEXICAL_H
#define LEXICAL_H

#include "error.h"

struct tiny_lex_s {
    const char *code; // 字符串指针
    int cur;
    int len;
    int line_number;
    int line_column;
};

struct tiny_lex_token_s {
    int error;
    const char *head, *tail;
    const char *s;
    const char *e;
    int line_number;
    int line_column;
};

typedef struct tiny_lex_s tiny_lex_t;
typedef struct tiny_lex_token_s tiny_lex_token_t;

void tiny_lex_begin(tiny_lex_t *lex, const char *code);

/**
 * @brief 读取下一个 token
 * @param lex 词法分析器
 * @param token 保存 token 的起始和终止地址
 * @return token 的长度，-1 表示读取结束
 */
int tiny_lex_next(tiny_lex_t *lex, tiny_lex_token_t *token);

tiny_lex_token_t tiny_lex_current_line(tiny_lex_token_t *token);

#endif // LEXICAL_H