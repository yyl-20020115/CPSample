#include "strutil.h"
#include "common.h"

void str_trim_crlf(char *str)
{
    char *p = &str[strlen(str)-1];
    while(*p == '\r' || *p == '\n')
        *p-- = '\0';
}

void str_split(const char *str , char *left, char *right, char c)
{
    char *p = strchr(str, c);
    if (p == NULL)
        strcpy(left, str);
    else
    {
        strncpy(left, str, p-str);
        strcpy(right, p+1);
    }
}

int str_all_space(const char *str)
{
    while (*str)
    {
        if (!isspace(*str))
            return 0;
        ++str;
    }
    return 1;
}

void str_upper(char *str)
{
    while (*str)
    {
        *str = toupper(*str);
        ++str;
    }
}


unsigned int str_octal_to_uint(const char *str)
{
    unsigned int result = 0;
    int seen_non_zero_digit = 0;

    while (*str)
    {
        int digit = *str;
        if (!isdigit(digit) || digit > '7')
            break;

        if (digit != '0')
            seen_non_zero_digit = 1;

        if (seen_non_zero_digit)
        {
            result <<= 3;
            result += (digit - '0');
        }
        ++str;
    }
    return result;
}

