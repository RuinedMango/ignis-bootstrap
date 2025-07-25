%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
%}

%union {
	int intval;
	float floatval;
	char *str;
	struct ASTNode *node;
	struct ASTNodeList *nodelist;
}

%token <intval> T_INT
%token <floatval> T_FLOAT
%token <str> T_ID T_TYPE

%token T_FN T_VAR T_CON T_RETURN
%token T_ASSIGN T_COMMA T_COLON T_SEMI T_LPAREN T_RPAREN T_LBRACE T_RBRACE
%token T_PLUS T_MINUS T_MUL T_DIV

%type <nodelist> functions param_list arg_list stmts
%type <node> program function param stmt expr

%left T_PLUS T_MINUS
%left T_MUL T_DIV

%%
program:
	functions{
		for (int i = 0; i < $1->count; i++){
			print_ast($1->nodes[i], 0);
		}
		free_node_list($1);
	}
	;

functions:
		 function { $$ = create_node_list(); node_list_add($$, $1); }
		 | functions function { node_list_add($1, $2); $$ = $1; }
		 ;

function:
		T_FN T_TYPE T_COLON T_ID T_LPAREN param_list T_RPAREN T_LBRACE stmts T_RBRACE { $$ = create_fn_node($2, $4, $6, $9); free($2); free($4); }
		;

param_list:
		  { $$ = create_node_list(); }
		  | param { $$ = new_node_list(); node_list_add($$, $1); }
		  | param_list T_COMMA param { node_list_add($1, $3); $$ = $1; }
		  ;
param:
	 T_TYPE T_COLON T_ID { $$ create_var_decl_node($1, $3, NULL); free($1); free($3) }
	 ;

stmts:
	 { $$ = create_node_list(); }
	 | stmts stmt {node_list_add($1, $2); $$ = $1}
	 ;

stmt:
	T_VAR T_TYPE T_COLON T_ID T_ASSIGN expr T_SEMI { $$ = create_var_decl_node($2, $4, $6); free($2); free($4); }
	| T_CON T_TYPE T_COLON T_ID T_ASSIGN expr T_SEMI { $$ = create_con_decl_node($2, $4, $6); free($2); free($4); }
	| T_ID T_ASSIGN expr T_SEMI { $$ = create_assign_node($1, $3); free($1); }
	| T_RETURN expr T_SEMI { $$ = create_return_node($2); }
	;

expr:
	T_INT { $$ = create_int_node($1); }
	| T_FLOAT { $$ = create_float_node($1); }
	| T_ID { $$ = create_id_node($1); }
	| expr T_PLUS expr { $$ = create_binop_node('+', $1, $3); }
	| expr T_MINUS expr { $$ = create_binop_node('-', $1, $3); }
	| expr T_MUL expr { $$ = create_binop_node('*', $1, $3); }
	| expr T_DIV expr { $$ = create_binop_node('/', $1, $3); }
	| T_ID T_LPAREN arg_list T_RPAREN { $$ = make_fn_call_node($1, $3); }
	;

arg_list:
		{ $$ = create_node_list(); }
		| expr { $$ = create_node_list(); node_list_add($$, $1); }
		| arg_list T_COMMA expr { node_list_add($1, $3); $$ = $1; }
%%
