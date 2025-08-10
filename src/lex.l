%{
#include "parse.tab.h"
%}

%%
"fn" return T_FN;
"extern" return T_EXTERN;
"var" return T_VAR;
"con" return T_CON;
"return" return T_RETURN;
"cdecl" { yylval.str = strdup(yytext); return T_CDECL; }
"stdcall" { yylval.str = strdup(yytext); return T_STDCALL; }
"fastcall" { yylval.str = strdup(yytext); return T_FASTCALL; }
"thiscall" { yylval.str = strdup(yytext); return T_THISCALL; }
"vectorcall" { yylval.str = strdup(yytext); return T_VECTORCALL; }

"if" return T_IF;
"else" return T_ELSE;

"null" return T_NULL;
"void" { yylval.str = strdup(yytext); return T_TYPE; }
i[0-9]+ { yylval.str = strdup(yytext); return T_TYPE; }
u[0-9]+ { yylval.str = strdup(yytext); return T_TYPE; }

[0-9]+\.[0-9]+ { yylval.floatval = strtof(yytext, NULL); return T_FLOAT; }
[0-9]+ { yylval.intval = strtol(yytext, NULL, 10); return T_INT; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.str = strdup(yytext); return T_ID; }
\"([^\\\"]|\\.)*\" { yylval.str = strdup(yytext + 1); yylval.str[strlen(yylval.str) - 1] = '\0'; return T_STRING; }

"+" return T_PLUS;
"-" return T_MINUS;
"*" return T_MUL;
"/" return T_DIV;

"&" return T_AMPERSAND;
"." return T_DOT;

"=" return T_ASSIGN;
";" return T_SEMI;
":" return T_COLON;
"," return T_COMMA;
"'" return T_QUOTE;
"\"" return T_DQUOTE;
"(" return T_LPAREN;
")" return T_RPAREN;
"{" return T_LBRACE;
"}" return T_RBRACE;
"[" return T_LBRACKET;
"]" return T_RBRACKET;

"==" return T_EQ;
"!=" return T_NE;
"<" return T_LT;
"<=" return T_LE;
">" return T_GT;
">=" return T_GE;

[ \t\n]+ ;
. return yytext[0];
%%
