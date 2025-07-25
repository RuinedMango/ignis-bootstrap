%{
#include "parse.tab.h"
%}

%%
"fn" return T_FN;
"var" return T_VAR;
"con" return T_CON;
"return" return T_RETURN;

"void" { yylval.str = strdup(yytext); return T_TYPE; }
i[0-9]+ { yylval.str = strdup(yytext); return T_TYPE; }

[0-9]+\.[0-9]+ { yylval.floatval = strtof(yytext, NULL); return T_FLOAT; }
[0-9]+ { yylval.intval = strtol(yytext, NULL, 10); return T_INT; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.id = strdup(yytext); return T_ID; }

"+" return T_PLUS;
"-" return T_MINUS;
"*" return T_MUL;
"/" return T_DIV;

"=" return T_ASSIGN;
";" return T_SEMI;
":" return T_COLON;
"," return T_COMMA;
"(" return T_LPAREN;
")" return T_RPAREN;
"{" return T_LBRACE;
"}" return T_RBRACE;

[ \t\n]+ ;
. return yytext[0];
%%
