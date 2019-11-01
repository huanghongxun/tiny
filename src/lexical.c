#include "lexical.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "string_util.h"

void tiny_lex_begin(tiny_lex_t *lex, const char *code)
{
    lex->code = code;
    lex->cur = 0;
    lex->len = strlen(code);
    lex->line_number = 1;
    lex->line_column = 0;
}

static int tiny_lex_next_char(tiny_lex_t *lex)
{
    int ch = lex->cur < lex->len ? (lex->line_column++, lex->code[lex->cur++]) : TINY_EOF;
    if (ch == '\n')
        lex->line_number++, lex->line_column = 0;
    return ch;
}

static int tiny_lex_peek_char(tiny_lex_t *lex, int offset)
{
    return lex->cur + offset < lex->len ? lex->code[lex->cur + offset] : TINY_EOF;
}

tiny_lex_token_t tiny_lex_current_line(tiny_lex_token_t *token)
{
    tiny_lex_token_t line;
    const char *s = token->s, *e = token->e;
    while (s > token->head && *(s - 1) != '\n')
        s--;
    while (e + 1 < token->tail && *(e + 1) != '\n')
        e++;
    line.s = s;
    line.e = e;
    return line;
}

int tiny_lex_next(tiny_lex_t *lex, tiny_lex_token_t *token)
{
    token->head = lex->code;
    token->tail = lex->code + lex->len;
    token->error = 0;

    int c = tiny_lex_next_char(lex);
    while (isspace(c))
        c = tiny_lex_next_char(lex);
    if (c == TINY_EOF)
        return TINY_EOF;

    token->line_column = lex->line_column;
    token->line_number = lex->line_number;

    if (c != TINY_EOF)
    {
        token->e = token->s = lex->code + (lex->cur - 1);
        if (c == '"' || c == '\'')
        {
            int cur = c;
            bool escape = false;
            while (c = tiny_lex_next_char(lex), true)
            {
                if (c == TINY_EOF)
                {
                    return TINY_UNEXPECTED_EOF;
                }

                if (!escape)
                {
                    if (c == cur)
                        break;
                    else if (c == '\\')
                        escape = true;
                }
                else
                {
                    escape = false;
                }
            }

            struct parse_string_literal_result_s ret = parse_string_literal(token->s, lex->code + lex->cur);
            if (ret.ret != 0)
            {
                // 更新 token 的错误点
                token->line_column += ret.column;
                token->line_number = lex->line_number;
                return ret.ret;
            }
        }
        else if (isdigit(c))
        {
            if (tiny_lex_peek_char(lex, 0) == 'x' || tiny_lex_peek_char(lex, 0) == 'X') // 16 进制
            {
                if (c != '0')
                {
                    // 更新 token 的错误点
                    token->line_column = lex->line_column;
                    token->line_number = lex->line_number;
                    return TINY_INVALID_NUMBER;
                }
                tiny_lex_next_char(lex);
                while (c = tiny_lex_peek_char(lex, 0), is_hex_digit(c))
                    tiny_lex_next_char(lex);
            }
            else
            {
                while (c = tiny_lex_peek_char(lex, 0), isdigit(c) || c == '.')
                    tiny_lex_next_char(lex);

                if (c == 'e') // 科学表示法
                {
                    tiny_lex_next_char(lex);                                  // 跳过 'e'
                    if (c = tiny_lex_peek_char(lex, 0), c == '+' || c == '-') // 跳过正负号
                        tiny_lex_next_char(lex);
                    if (!isdigit(c = tiny_lex_peek_char(lex, 0))) // 检查阶数是否合法
                    {
                        // 更新 token 的错误点
                        token->line_column = lex->line_column;
                        token->line_number = lex->line_number;
                        return c == TINY_EOF ? TINY_UNEXPECTED_EOF : TINY_UNEXPECTED_TOKEN;
                    }
                    while (c = tiny_lex_peek_char(lex, 0), isdigit(c)) // 提取阶数
                        tiny_lex_next_char(lex);
                }
                else // 普通数字
                {
                    while (c = tiny_lex_peek_char(lex, 0), isdigit(c))
                        tiny_lex_next_char(lex);
                }
            }
        }
        else if (!is_name_char(c)) // 符号
        {
            int len = is_symbol(c, tiny_lex_peek_char(lex, 0), tiny_lex_peek_char(lex, 1));
            while (--len > 0)
                tiny_lex_next_char(lex);
        }
        else // 标识符
        {
            while (c = tiny_lex_peek_char(lex, 0), is_name_char(c))
                tiny_lex_next_char(lex);
        }
    }

    token->e = lex->code + lex->cur;
    if (strsecmp(token->s, token->e, "/*")) // 多行注释
    {
        while (tiny_lex_peek_char(lex, 0) != TINY_EOF && tiny_lex_peek_char(lex, 1) != TINY_EOF &&
               (tiny_lex_peek_char(lex, 0) != '*' || tiny_lex_peek_char(lex, 1) != '/'))
        {
            tiny_lex_next_char(lex);
        }

        if (tiny_lex_peek_char(lex, 0) == '*')
        {
            tiny_lex_next_char(lex);
            tiny_lex_next_char(lex);
        }
        else
        {
            // 更新 token 的错误点
            token->line_column = lex->line_column;
            token->line_number = lex->line_number;
            return TINY_UNEXPECTED_EOF;
        }

        return tiny_lex_next(lex, token);
    }

    if (strsecmp(token->s, token->e, "//")) // 单行注释
    {
        while (c != TINY_EOF && c != '\n')
            c = tiny_lex_next_char(lex);

        return tiny_lex_next(lex, token);
    }

    return 0;
}
