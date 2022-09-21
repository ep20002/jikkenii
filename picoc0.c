#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scan.h"

int token;             // 最後に読んだトークン
char name[MAX_TK_LEN]; // ...の名前
int vsuffix = 1;       // 値の一時名の接尾番号
#define MAX_TMP_LEN 16 // 値の一時名の最大長


void expect(int expected)
{
    if (token == expected) {
        token = get_next_token(name);
        return;
    }
    fprintf(stderr, "Syntax error\n");
    exit(1);
}


void num(char *ret)
{
    strcpy(ret, name);
    expect(TK_NUM);
}


void expr(char *left)
{
    num(left);
    while (token == '+' || token == '-' || token == '<') {
        int op = token;
        token = get_next_token(name);
        char right[MAX_TMP_LEN];
        num(right);
        int v = vsuffix++;
        char *f = op == '+' ? "add" : op == '-' ? "sub" : "icmp slt";
        printf("  %%t.%d = %s i32 %s, %s\n", v, f, left, right);
        // '<'の場合は真偽値(i1)をint(i32)にゼロ拡張
        if (op == '<') {
            int u = vsuffix++;
            printf("  %%t.%d = zext i1 %%t.%d to i32\n", u, v);
            v = u;
        }
        sprintf(left, "%%t.%d", v);
    }
}


void print_st(void)
{
    expect(TK_PRINT);
    char e[MAX_TMP_LEN];
    expr(e);
    int v1 = vsuffix++, v2 = vsuffix++;
    printf("  %%t.%d = getelementptr [4 x i8], [4 x i8]* @fmt_pd, i32 0, i32 0\n", v1);
    printf("  %%t.%d = call i32 (i8*, ...) @printf(i8* %%t.%d, i32 %s)\n", v2, v1, e);
    expect(';');
}


void program(void)
{
    printf("@fmt_pd = private constant [4 x i8] c\"%%d\\0A\\00\"\n");
    printf("declare i32 @printf(i8*, ...)\n\n");

    expect(TK_INT);
    if (strcmp(name, "main") != 0) {
        fprintf(stderr, "Syntax error\n");
        exit(1);
    }
    expect(TK_ID);
    expect('(');
    expect(')');
    printf("define i32 @main() {\n");

    expect('{');
    print_st();
    expect('}');

    printf("  ret i32 0\n");
    printf("}\n");
}

int main()
{
    token = get_next_token(name);
    program();
}

