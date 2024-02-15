#ifndef UTILS_H
#define UTILS_H

// This has become considerally bigger than my initial plans as I have worked my way through the code. This might end up becoming a custom string class (yikes)

// the tests for this class to make sure it all works are here: https://github.com/DrewBrandt/CPPUtils
//  let me (Drew Brandt) know if there are errors

char *newstr(char *str, int len);
char *newstr(char *str);

// without Arduino or the std C++ library, float and int to string conversions have to be done manually... the base algorithm is courtesy of ChatGPT
void revstr(char *str, int len); // reverse string - used in the int function
char *newstr(long long num, int base = 10);
char *newstr(double num, int decimals = 2); // base 10 floats only ftlog
char *newstr(int num, int base = 10);

// This is getting tedious
// but also a little bit cool writing all the very basic functions that everyone takes for granted.
char *concat(char *str, char *str2);
int strlen(char *str);
bool streql(char *a, char *b);
#endif