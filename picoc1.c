#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scan.h"

int token;             // 最後に読んだトークン
char name[MAX_TK_LEN]; // ...の名前
int vsuffix = 1;       // 値の一時名の接尾番号
int lsuffix = 1;       // ラベルの接尾番号
#define MAX_TMP_LEN 16 // 値の一時名の最大長
void statement(void);


void expect(int expected)
{
    if (token == expected) {
        token = get_next_token(name);
        return;
    }
    fprintf(stderr, "Syntax error\n");
    exit(1);
}


void id(char *ret)
{
    int v = vsuffix++;
    printf("  %%t.%d = load i32, i32* %%%s\n", v, name);
    expect(TK_ID);
    sprintf(ret, "%%t.%d", v);
}


void num(char *ret)
{
    strcpy(ret, name);
    expect(TK_NUM);
}


void factor(char *ret)
{
    switch (token) {
        case TK_ID:
            id(ret);
            break;
        case TK_NUM:
            num(ret);
            break;
        default:
            fprintf(stderr, "Syntax error\n");
            exit(1);
    }
}


void expr(char *left)
{
    factor(left);
    while (token == '+' || token == '-' || token == '<') {
        int op = token;
        token = get_next_token(name);
        char right[MAX_TMP_LEN];
        factor(right);
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


void assign_st(void)
{
    char x[MAX_TK_LEN];
    strcpy(x, name);
    expect(TK_ID);
    expect('=');
    char e[MAX_TMP_LEN];
    expr(e);
    printf("  store i32 %s, i32* %%%s\n", e, x);
    expect(';');
}


void while_st(void)
{
    int while_entry = lsuffix++;
    printf("  br label %%L.%d\n", while_entry);

    // whileループ冒頭のブロック
    printf("L.%d:\n", while_entry);
    expect(TK_WHILE);
    expect('(');
    char e[MAX_TK_LEN];
    expr(e);
    expect(')');

    int c = vsuffix++;
    printf("  %%t.%d = icmp eq i32 %s, 0\n", c, e);
    int while_body = lsuffix++;
    int while_end = lsuffix++;
    printf("  br i1 %%t.%d, label %%L.%d, label %%L.%d\n", c, while_end, while_body);

    // whileループ本体のブロック
    printf("L.%d:\n", while_body);
    statement();
    printf("  br label %%L.%d\n", while_entry);

    // whileループの直後のブロック
    printf("L.%d:\n", while_end);
}


void var_decl()
{
    expect(TK_INT);
    printf("  %%%s = alloca i32\n", name);
    expect(TK_ID);
    expect(';');
}


void block(void)
{
    expect('{');
    while (token != '}') {
        if (token == TK_INT) var_decl(); else statement();
    }
    expect('}');
}


void statement(void)
{
    switch (token) {
        case TK_PRINT:
            print_st();
            break;
        case TK_ID:
            assign_st();
            break;
        case TK_WHILE:
            while_st();
            break;
        case '{':
            block();
            break;
        default:
            fprintf(stderr, "Syntax error\n");
            exit(1);
    }
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

    block();

    printf("  ret i32 0\n");
    printf("}\n");
}

int main()
{
    token = get_next_token(name);
    program();
}

