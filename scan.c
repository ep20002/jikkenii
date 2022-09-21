#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "scan.h"

int row = 1;
int col = 0;

static int inc = 0; // 前回の増分保留分

static inline int kind(char *name)
{
    if (strcmp("if",       name) ==0) return TK_IF;
    if (strcmp("else",     name) ==0)return TK_ELSE;
    if (strcmp("while",    name) ==0) return TK_WHILE;
    if (strcmp("print",    name) ==0) return TK_PRINT;
    if (strcmp("scan",    name) ==0) return TK_SCAN;
    if (strcmp("int",      name) ==0) return TK_INT;
    if (strcmp("return",   name) ==0) return TK_RET;
    if (strcmp("break",    name) ==0) return TK_BREAK;
    if (strcmp("continue", name) ==0) return TK_CONT;
    return TK_ID;
}

int get_next_token(char *name)
{
    int c;
    col += inc; inc = 0;
    // 空白のスキップ
    for (;;) {
        c = getchar(); col++;
        if (c == EOF) return 0;
        // コメント行(//...)のスキップ
        if (c == '/') {
            name[0] = '/';
            c = getchar(); col++;
            if (c != '/') {
                ungetc(c, stdin); col--;
                name[1] = '\0';
                return '/';
            }
            while (c != '\n') {
                if (c == EOF) return 0;
                c = getchar(); col++;
            }
        }
        if (!isspace(c)) break;
        if (c == '\n') { row++; col = 0; }
    }
    // c  != EOF && !isspace(c)

    // 演算子(=, <, >, ==, <=, >=)の認識
    if (c == '=' || c == '<' || c == '>') {
        name[0] = c;
        char first = c;
        c = getchar(); inc++;
        if (c == '=') {
            name[1] = '=';
            name[2] = '\0';
            switch (first) {
                case '=': return TK_EQ;
                case '<': return TK_LEQ;
                case '>': return TK_GEQ;
            }
        }
        ungetc(c, stdin); inc--;
        name[1] = '\0';
        return first;
    }
    // 演算子(!, !=)の認識
    if (c == '!') {
        name[0] = '!';
        c = getchar(); inc++;
        if (c == '=') {
            name[1] = '=';
            name[2] = '\0';
            return TK_NEQ;
        }
        ungetc(c, stdin); inc--;
        name[1] = '\0';
        return '!';
    }
    // 演算子(&, &&)の認識
    if (c == '&') {
        name[0] = '&';
        c = getchar(); inc++;
        if (c == '&') {
            name[1] = '&';
            name[2] = '\0';
            return TK_AND;
        }
        ungetc(c, stdin); inc--;
        name[1] = '\0';
        return '&';
    }
    // 演算子(|, ||)の認識
    if (c == '|') {
        name[0] = '|';
        c = getchar(); inc++;
        if (c == '|') {
            name[1] = '|';
            name[2] = '\0';
            return TK_OR;
        }
        ungetc(c, stdin); inc--;
        name[1] = '\0';
        return '|';
    }
    // 数(10進整数)[0-9]+の認識
    if (isdigit(c)) {
        name[0] = c;
        int i;
        for (i = 1; i < MAX_TK_LEN - 1; i++) {
            c = getchar(); inc++;
            if (!isdigit(c)) break;
            name[i] = c;
        }
        ungetc(c, stdin); inc--;
        name[i] = '\0';
        return TK_NUM;
    }
    // 識別子または2文字以上の予約語[a-zA-Z_][a-zA-Z0-9_]*の認識
    if (isalpha(c) || c == '_') {
        name[0] = c;
        int i;
        for (i = 1; i < MAX_TK_LEN - 1; i++) {
            c = getchar(); inc++;
            if (!isalnum(c) && c != '_') break;
            name[i] = c;
        }
        ungetc(c, stdin); inc--;
        name[i] = '\0';
        return kind(name);
    }
    // いずれでもない文字は1文字のトークンとして認識
    name[0] = c;
    name[1] = '\0';
    return c;
}

