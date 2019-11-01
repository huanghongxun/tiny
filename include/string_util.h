#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <stdbool.h>
#include <stdarg.h>

/**
 * 检查是否是数字或 A-F（忽略大小写）
 */
bool is_hex_digit(char c);

/**
 * 检查是否是标识符字符
 */
bool is_name_char(int ch);

int is_symbol(int s1, int s2, int s3);

struct parse_string_literal_result_s
{
    int ret;
    int column;
};

struct parse_string_literal_result_s parse_string_literal(const char *s, const char *e);

/**
 * 检查 s[0,e) 是否和以 \0 结尾的字符串 t 一致
 */
bool strsecmp(const char *s, const char *e, const char *t);

#endif // STRING_UTIL_H