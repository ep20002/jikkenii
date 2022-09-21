#include <stdio.h>
#include "scan.h"

char *printable[256] = {
    [TK_EQ]=    "TK_EQ     (==)",
    [TK_NEQ]=   "TK_NEQ    (!=)",
    [TK_LEQ]=   "TK_LEQ    (<=)",
    [TK_GEQ]=   "TK_GEQ    (>=)",
    [TK_AND]=   "TK_AND    (&&)",
    [TK_OR]=    "TK_OR     (||)",
    [TK_IF]=    "TK_IF     (if)",
    [TK_ELSE]=  "TK_ELSE   (else)",
    [TK_WHILE]= "TK_WHILE  (while)",
    [TK_PRINT]= "TK_PRINT  (print)",
    [TK_SCAN]=  "TK_SCAN  (scan)",
    [TK_INT]=   "TK_INT    (int)",
    [TK_BREAK]= "TK_BREAK  (break)",
    [TK_CONT]=  "TK_CONT   (continue)",
    [TK_RET]=   "TK_RET    (return)"
};


char *display(int token, char *name, char *buff)
{
    if (token == 0)  buff[0] = '\0';
    else if (token < 128) sprintf(buff, "%c", token);
    else if (token == TK_ID)  sprintf(buff, "TK_ID     (%s)", name);
    else if (token == TK_NUM) sprintf(buff, "TK_NUM    (%s)", name);
    else sprintf(buff, "%s", printable[token]);
    return buff;
}


int main()
{
    char name[MAX_TK_LEN + 1];
    char buff[MAX_TK_LEN + 10];
    for (;;) {
        int token = get_next_token(name);
        if (token == 0) break;
        printf("%s\n", display(token, name, buff));
    }
}
