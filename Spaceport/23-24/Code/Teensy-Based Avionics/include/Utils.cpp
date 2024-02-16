

#include "Utils.h"

char *newstr(char *str, int len) // careful with this...
{
    char *ret = new char[len + 1];
    for (int i = 0; i < len; i++)
        ret[i] = str[i];
    ret[len] = '\0';
    return ret;
}

char *newstr(char *str)
{
    int i = strlen(str);
    char *ret = new char[i + 1];
    for (int j = 0; j <= i; j++)
        ret[j] = str[j];
    return ret;
}

void revstr(char *str, int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}
char *newstr(int num, int base)
{
    return newstr((long long)num, base);
}
char *newstr(long long num, int base)
{
    char *str = new char[21]; // 19 characters for max size of long + null + negative sign
    int i = 0;
    bool isNegative = false;

    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    if (num < 0 && base == 10)
    {
        isNegative = true;
        num = -num;
    }

    while (num != 0)
    {
        long long rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    if (isNegative)
    {
        str[i++] = '-';
    }

    str[i] = '\0';

    revstr(str, i);
    return str;
}

char *newstr(double num, int decimals)
{
    long long ipart = (long long)num;
    double fpart = num - (double)ipart;
    if (fpart < 0)
        fpart = -fpart;
    char *istr = newstr(ipart);
    if (decimals != 0)
    {
        long long dec = 1;
        for (int i = 0; i < decimals; i++)
            dec *= 10;
        fpart = fpart * dec;
        char *fstr = newstr((long long)fpart);
        char *dstr = concat(".", fstr);
        char *str = concat(istr, dstr);
        delete[] fstr;
        delete[] istr;
        delete[] dstr;
        return str;
    }
    return istr;
}

int strlen(char *str)
{
    int i = 0;
    while (str[++i] != '\0')
        ;
    return i;
}
char *concat(char *str1, char *str2)
{
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    char *nstr = new char[len1 + len2 + 1];
    for (int i = 0; i < len1; ++i)
        nstr[i] = str1[i];

    for (int i = 0; i < len2; ++i)
        nstr[len1 + i] = str2[i];

    nstr[len1 + len2] = '\0';
    return nstr;
}

bool streql(char *a, char *b)
{ // courtesy of ChatGPT
    for (; *a && *b; ++a, ++b)
    {
        if (*a != *b)
            return false;
    }
    return *a == *b;
}