#include "string_util.h"
#include <ctype.h>
#include "error.h"

bool is_hex_digit(char c)
{
    return isdigit(c) || 'a' <= c && c <= 'f' || 'A' <= c && c <= 'F';
}

bool is_name_char(int ch)
{
	return isdigit(ch) || isalpha(ch) || ch == '_';
}

int is_symbol(int s1, int s2, int s3) {
	static const char *SYMBOLS[] = { "+=", "-=", "*=", "/=", "|=", "&=", "%=", "^=", "<=", ">=", "==", "!=", "&&", "||", "<<", ">>",
		">>=", "<<=", "//", "/*", "*/", "...", "--", "++", ":=" };
 
	int s[] = { s1, s2, s3 };

	for (int i = 0; i < 25; ++i) {
		int *p = s;
		int j;
		for (j = 0; SYMBOLS[i][j]; ++j, ++p)
			if (*p != SYMBOLS[i][j]) {
				j = -1;
				break;
			}
		if (j != -1)
			return j;
	}
	return 0;
}

struct parse_string_literal_result_s parse_string_literal(const char *s, const char *e)
{
    struct parse_string_literal_result_s result;
    result.ret = 0;
    for (const char *c = s; c != e; ++c)
    {
        if (*c == '\\')
        {
            char nxt = *(c + 1);
            if (nxt == 'x')
            {
                if (c + 2 == e || c + 3 == e)
                {
                    result.ret = TINY_INVALID_STRING_X_NO_FOLLOWING_HEX_DIGITS;
                    result.column = c - s;
                    return result;
                }
                c += 3;
            }
            else
            {
                c += 1;
            }
        }
    }
    return result;
}

bool strsecmp(const char *s, const char *e, const char *t)
{
    const char *i;
    for (i = s; i != e && *t; ++i, ++t)
        if (*i != *t) return false;
    return i == e && !*t;
}
