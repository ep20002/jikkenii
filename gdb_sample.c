#include <stdio.h>

char *s = "Hello world";
//char s[16] = "Hello world";
//char s[16] = "Hello";

int main()
{
    int i = 0;
    while (s[i] != ' ') {
        i++;
    }
    s[i] = '\n'; s[i+1] = '\0';
    printf("%s", s);
}
