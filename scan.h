#ifndef SCAN_H
#define SCAN_H

#define MAX_TK_LEN 32 // トークンの名前の最大長(\0を含む)
#define MAX_KW_LEN 16 // キーワードの最大長(\0を含む)

// トークン(識別番号)
#define TK_ID 128
#define TK_NUM 129
#define TK_EQ 130
#define TK_NEQ 131
#define TK_LEQ 132
#define TK_GEQ 133
#define TK_IF 134
#define TK_ELSE 135
#define TK_WHILE 136
#define TK_PRINT 137
#define TK_SCAN 138
#define TK_INT 139
#define TK_AND 140
#define TK_OR 141
#define TK_BREAK 142
#define TK_CONT 143
#define TK_RET 144

extern int row; // 最後に処理した行番号
extern int col; // 最後に処理した行頭からの文字数

// 次の字句を読んで返す
// 引数name: 読んだトークンの名前を返却する文字列バッファ(配列)へのポインタ
// 戻り値: トークン識別番号(次の字句がなければ0)
// (終端\0も含め)高々MAX_TK_LEN長の文字列しか返さないので、バッファのサイズ
// はMAX_TK_LENあれば十分
int get_next_token(char *name);

#endif

