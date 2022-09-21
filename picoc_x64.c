#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scan.h"
#include "hashmap.h"

int token;             // 最後に読んだトークン
char name[MAX_TK_LEN]; // ...の名前
int lsuffix = 1;       // ラベルの接尾番号
hashmap *hmap;         // 変数名と接尾番号の対を保存するハッシュ表
#define MAX_OFFSET 320 // 320で変数40個分, 16の倍数に要整列

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


void id(void)
{
    DBG("Entering id()");
    char x[MAX_TK_LEN];
    strcpy(x, name);
    int r = row, c = col;
    expect(TK_ID);
    int off = hget(hmap, x);
    if (off < 0) {
        fprintf(stderr,
                "Undefined variable in line %d (at %d): %s\n",
                r, c, x);
        exit(1);
    }
    printf("  mov  rax, [rbp - %d]\t # %s\n", off, x);
    printf("  push rax\n");
    // 以下も可
    //printf("  push qword ptr [rbp - %d] # %s\n", off, x);
}


void factor(void)
{
    DBG("Entering factor()");
    switch (token) {
        case TK_ID:
            id();
            break;
        case TK_NUM:
            printf("  push %d\n", atoi(name));
            expect(TK_NUM);
            break;
        default:
            fprintf(stderr,
                    "Syntax error around line %d (at %d): identifier or number is expected, but found: %s\n",
                    row, col, name);
            exit(1);
    }
}


void expr(void)
{
    DBG("Entering expr()");
    factor();
    while (token == '+' || token == '-' || token == '<') {
        int op = token;
        DBG(NULL);
        token = get_next_token(name);
        factor();
        char *f = op == '+' ? "add" : op == '-' ? "sub" : "cmp";
        printf("  pop  rdx\n");
        printf("  pop  rax\n");
        printf("  %s  rax, rdx\n", f);
        if (op == '<') {
            // setlは8ビットレジスタしか取れないので1バイトから8バイトへゼロ拡張
            printf("  setl al\n");
            printf("  movzx rax, al\n");
        }
        printf("  push rax\n");
    }
}


void print_st(void)
{
    DBG("Entering print_st()");
    expect(TK_PRINT);
    expr();
    printf("  lea  rdi, [rip + fmt]\n"); // rdi=第1引数
    printf("  pop  rsi\n"); // rsi=第2引数
    printf("  xor  eax, eax\n"); // al <- 0
    printf("  call printf\n");
    expect(';');
}


void assign_st(void)
{
    DBG("Entering assign_st()");
    char x[MAX_TK_LEN];
    strcpy(x, name);
    int r = row, c = col;
    expect(TK_ID);
    int off = hget(hmap, x);
    if (off < 0) {
        fprintf(stderr,
                "Undefined variable in line %d (at %d): %s\n",
                r, c, x);
        exit(1);
    }
    expect('=');
    expr();
    printf("  pop  rax\n");
    printf("  mov  [rbp - %d], rax\t # %s\n", off, x);
    // 以下も可
    //printf("  pop  qword ptr [rbp - %d] # %s\n", off, x);
    expect(';');
}


void while_st(void)
{
    DBG("Entering while_st()");
    int while_entry = lsuffix++;
    printf("  jmp .L.%d\n", while_entry);

    // whileループ冒頭のブロック
    printf(".L.%d:\n", while_entry);
    expect(TK_WHILE);
    expect('(');
    expr();
    expect(')');

    printf("  pop  rax\n");
    printf("  cmp  rax, 0\n");
    int while_end = lsuffix++;
    printf("  je   .L.%d\n", while_end);

    // whileループ本体のブロック
    statement();
    printf("  jmp  .L.%d\n", while_entry);

    // whileループの直後のブロック
    printf(".L.%d:\n", while_end);
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
    // キー(識別子名)とオフセットをハッシュ表に登録
    static int off = 8; // rbpからのオフセットの絶対値,8の倍数
    if (off > MAX_OFFSET) {
        fprintf(stderr, "Error: Too many variables\n");
        exit(1);
    }
    printf("  # %s is allocated to rbp - %d\n", x, off);
    hinsert(hmap, x, off);
    off += 8;
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
    printf("  .intel_syntax noprefix\n");
    printf("  .data\n");
    printf("  .text\n");
    printf("fmt: .asciz \"%%d\\n\"\n");
    printf("  .global main\n");
    expect(TK_INT);
    if (strcmp(name, "main") != 0) {
        fprintf(stderr, "Syntax error: function main needed");
        exit(1);
    }
    expect(TK_ID);
    expect('(');
    expect(')');
    printf("main:\n");
    printf("  push rbp\n");
    // 復帰アドレス(8)+rbpのバックアップ(8)で
    // この時点でrspは16バイトで整列されているはず
    printf("  mov  rbp, rsp\n");
    // rspの16バイト整列が保たれるようにMAX_OFFSETは16の倍数
    printf("  sub  rsp, %d\n", MAX_OFFSET);
    block();
    printf("  mov  rsp, rbp\n");
    printf("  pop  rbp\n");
    printf("  xor rax, rax\n");
    printf("  ret\n");
}

int main()
{
    // 識別子(変数名)とその接尾番号の対を保存するハッシュ表
    hmap = hnew();
    token = get_next_token(name);
    program();
    hdestroy(hmap);
}

