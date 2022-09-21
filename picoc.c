#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scan.h"
#include "hashmap.h"

int token;             // 最後に読んだトークン
char name[MAX_TK_LEN]; // ...の名前
int vsuffix = 1;       // 値の一時名の接尾番号
int lsuffix = 1;       // ラベルの接尾番号
#define MAX_TMP_LEN 16 // 値の一時名の最大長
hashmap *hmap;         // 変数名と接尾番号の対を保存するハッシュ表

// デバッグ用
void debug_info(char *msg)
{
    if (msg) {
        fputs(msg, stderr);
        fputs("\n", stderr);
    }
    else fprintf(stderr,
                 "Consume token %s in line %d (at %d)\n",
                 name, row, col);
}

#ifdef DEBUG
#define DBG(msg); debug_info(msg);
#else
#define DBG(msg);
#endif

void statement(void);


void expect(int expected)
{
    DBG("Entering expected()");
    if (token == expected) {
        DBG(NULL);
        token = get_next_token(name);
        return;
    }
    fprintf(stderr, "! Syntax error around line %d (at %d)\n", row, col);
    fprintf(stderr, "! %s...\n", name);
    fprintf(stderr, "! ^Here\n");
    exit(1);
}


void id(char *ret)
{
    DBG("Entering id()");
    char x[MAX_TK_LEN];
    strcpy(x, name);
    int r = row, c = col;
    expect(TK_ID);
    if (hget(hmap, x) < 0) {
        fprintf(stderr,
                "Undefined variable in line %d (at %d): %s\n",
                r, c, x);
        exit(1);
    }
    int v = vsuffix++;
    printf("  %%t.%d = load i32, i32* %%%s\n", v, x);
    sprintf(ret, "%%t.%d", v);
}


void num(char *ret)
{
    DBG("Entering num()");
    strcpy(ret, name);
    expect(TK_NUM);
}


void factor(char *ret)
{
    DBG("Entering factor()");
    switch (token) {
        case TK_ID:
            id(ret);
            break;
        case TK_NUM:
            num(ret);
            break;
        default:
            fprintf(stderr,
                    "Syntax error around line %d (at %d): identifier or number is expected, but found: %s\n",
                    row, col, name);
            exit(1);
    }
}


void expr(char *left)
{
    DBG("Entering expr()");
    factor(left);
    while (token == '+' || token == '-' || token == '<') {
        int op = token;
        DBG(NULL);
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
    DBG("Entering print_st()");
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
    DBG("Entering assign_st()");
    char x[MAX_TK_LEN];
    strcpy(x, name);
    int r = row, c = col;
    expect(TK_ID);
    if (hget(hmap, x) < 0) {
        fprintf(stderr,
                "Undefined variable in line %d (at %d): %s\n",
                r, c, x);
        exit(1);
    }
    expect('=');
    char e[MAX_TMP_LEN];
    expr(e);
    printf("  store i32 %s, i32* %%%s\n", e, x);
    expect(';');
}


void while_st(void)
{
    DBG("Entering while_st()");
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
    DBG("Entering var_decl()");
    expect(TK_INT);
    // 宣言重複のチェック
    if (hget(hmap, name) >= 0) {
        fprintf(stderr,
                "Duplicated variable declaration in line %d (at %d): %s\n",
                row, col, name);
        exit(1);
    }
    // ハッシュ表のキーを保存する文字列バッファ
    // 少なくともハッシュ表と同じ寿命をもつ必要があるのでstatic
    static char buf[4096];
    static char *x = buf;
    if ((x - buf) + strlen(name) + 1 > 4096) {
        fprintf(stderr, "Error: Name buffer is full\n");
        exit(1);
    }
    strcpy(x, name);
    expect(TK_ID);
    printf("  %%%s = alloca i32\n", x);
    // キー(識別子名)をハッシュ表に登録
    // 値は使用しないのでなんでも可(ここでは0にした)
    hinsert(hmap, x, 0);
    x += strlen(x) + 1;
    expect(';');
}


void block(void)
{
    DBG("Entering block()");
    expect('{');
    while (token != '}') {
        if (token == TK_INT) var_decl(); else statement();
    }
    expect('}');
}


void statement(void)
{
    DBG("Entering statement()");
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
            fprintf(stderr,
                    "Syntax error around line %d (at %d): statement is expected, but found: %s\n",
                    row, col, name);
            exit(1);
    }
}

void program(void)
{
    DBG("Entering program()");
    printf("@fmt_pd = private constant [4 x i8] c\"%%d\\0A\\00\"\n");
    printf("declare i32 @printf(i8*, ...)\n\n");

    expect(TK_INT);
    if (strcmp(name, "main") != 0) {
        fprintf(stderr, "Syntax error: function main needed");
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
    // 識別子(変数名)とその接尾番号の対を保存するハッシュ表
    hmap = hnew();
    token = get_next_token(name);
    program();
    hdestroy(hmap);
}

