#include "ast.h"
#include <stdio.h>

extern int yyparse();
extern FILE* yyin;

int main(int argc, char** argv) {
    init_codegen();
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.c>\n", argv[0]);
        return 1;
    }
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror("fopen");
        return 1;
    }
    yyparse();
    finish_codegen();
    return 0;
}

int yywrap(void) {
    return 1; // Return 1 means "no more input"
}

void yyerror(const char* s) { fprintf(stderr, "Parse error: %s\n", s); }
