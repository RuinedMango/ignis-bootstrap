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

%token T_NULL
%token <intval> T_INT
%token <floatval> T_FLOAT
%token <str> T_ID T_TYPE
%token <str> T_STRING

%token T_FN T_EXTERN T_VAR T_CON T_RETURN
%token <str> T_CDECL T_STDCALL T_FASTCALL T_THISCALL T_VECTORCALL
%token T_AMPERSAND T_DOT T_ASSIGN T_COMMA T_COLON T_QUOTE T_DQUOTE T_SEMI T_LPAREN T_RPAREN T_LBRACE T_RBRACE T_LBRACKET T_RBRACKET
%token T_PLUS T_MINUS T_MUL T_DIV
%token T_EQ T_NE T_LT T_LE T_GT T_GE
%token T_IF T_ELSE T_STRUCT

%type <str> callconv
%type <node> func_call
%type <nodelist> functions param_list expr_list stmts
%type <node> program function param stmt expr type array_literal
%type <node> struct_decl struct_members
%type <node> other_stmt matched_stmt unmatched_stmt

%left T_EQ T_NE
%nonassoc T_LT T_LE T_GT T_GE
%left T_PLUS T_MINUS
%left T_MUL T_DIV

%%
program:
	functions{
		codegen($1);
		free_node_list($1);
	}
	;

functions:
		 function { $$ = create_node_list(); node_list_add($$, $1); }
		 | functions function { node_list_add($1, $2); $$ = $1; }
		 | functions struct_decl { node_list_add($1, $2); $$ = $1; }
		 ;

function:
		T_FN type T_COLON T_ID T_LPAREN param_list T_RPAREN T_LBRACE stmts T_RBRACE { $$ = create_fn_node($2, $4, $6, $9, "cdecl"); free($4); }
		| T_FN type T_COLON T_ID T_LPAREN param_list T_RPAREN callconv T_LBRACE stmts T_RBRACE { $$ = create_fn_node($2, $4, $6, $10, $8); free($4); }
		| T_EXTERN T_FN type T_COLON T_ID T_LPAREN param_list T_RPAREN T_SEMI { $$ = create_extern_fn_node($3, $5, $7, "cdecl"); free($5); }
		| T_EXTERN T_FN type T_COLON T_ID T_LPAREN param_list T_RPAREN callconv T_SEMI { $$ = create_extern_fn_node($3, $5, $7, $9); free($5); }
		;

struct_decl:
		   T_STRUCT T_ID T_LBRACE struct_members T_RBRACE { $$ = create_struct_def_node($2, $4); free($2); }
		   ;

struct_members:
			  { $$ = create_node_list(); }
			  | struct_members type T_COLON T_ID T_SEMI
			  {
			  	ASTNode* fld = create_var_decl_node($2, $4, NULL, (strstr($1->u.type, "u") != NULL) ? 1 : 0);
				node_list_add($1, fld);
				$$ = $1;
				free($4);
			  }
			  ;

func_call:
		 T_ID T_LPAREN expr_list T_RPAREN { $$ = create_fn_call_node($1, $3); }
		 ;

callconv:
		T_CDECL { $$ = $1; }
		| T_STDCALL { $$ = $1; }
		| T_FASTCALL { $$ = $1; }
		| T_THISCALL { $$ = $1; }
		| T_VECTORCALL { $$ = $1; }
		;

param_list:
		  { $$ = create_node_list(); }
		  | param { $$ = create_node_list(); node_list_add($$, $1); }
		  | param_list T_COMMA param { node_list_add($1, $3); $$ = $1; }
		  ;
param:
	 type T_COLON T_ID { $$ = create_var_decl_node($1, $3, NULL, (strstr($1->u.type, "u") != NULL) ? 1 : 0); free($3); }
	 ;

type:
	T_TYPE { $$ = create_type_node($1); }
	| T_LBRACKET T_INT T_RBRACKET type { $$ = create_array_type_node($4, $2); }
	| T_MUL type { $$ = create_ptr_type_node($2); }
	;

stmts:
	 { $$ = create_node_list(); }
	 | stmts stmt { node_list_add($1, $2); $$ = $1; }
	 ;

stmt:
	matched_stmt
	| unmatched_stmt
	;

other_stmt:
          T_VAR type T_COLON T_ID T_ASSIGN expr T_SEMI { $$ = create_var_decl_node($2, $4, $6, (strstr($2->u.type, "u") != NULL) ? 1 : 0); free($4); }
	      | T_CON type T_COLON T_ID T_ASSIGN expr T_SEMI { $$ = create_con_decl_node($2, $4, $6, (strstr($2->u.type, "u") != NULL) ? 1 : 0); free($4); }
		  | T_ID T_ASSIGN expr T_SEMI { $$ = create_assign_node($1, $3); free($1); }
		  | T_RETURN expr T_SEMI { $$ = create_return_node($2); }
		  | func_call T_SEMI { $$ = $1; }
		  | T_LBRACE stmts T_RBRACE { $$ = create_block_node($2); }
		  ;

matched_stmt:
			other_stmt
			| T_IF T_LPAREN expr T_RPAREN matched_stmt T_ELSE matched_stmt { $$ = create_if_node($3, $5, $7); }
			;

unmatched_stmt:
			  T_IF T_LPAREN expr T_RPAREN stmt { $$ = create_if_node($3, $5, NULL); }
			  | T_IF T_LPAREN expr T_RPAREN matched_stmt T_ELSE unmatched_stmt { $$ = create_if_node($3, $5, $7); }
			  ;

expr:
	T_NULL { $$ = create_null_node(); }
	| T_INT { $$ = create_int_node($1); }
	| T_FLOAT { $$ = create_float_node($1); }
	| T_ID { $$ = create_id_node($1); }
	| expr T_PLUS expr { $$ = create_binop_node('+', $1, $3); }
	| expr T_MINUS expr { $$ = create_binop_node('-', $1, $3); }
	| expr T_MUL expr { $$ = create_binop_node('*', $1, $3); }
	| expr T_DIV expr { $$ = create_binop_node('/', $1, $3); }
	| expr T_LBRACKET expr T_RBRACKET { $$ = create_array_index_node($1, $3); }
	| T_AMPERSAND T_ID { $$ = create_address_of_node($2); }
	| T_ID T_DOT T_MUL { $$ = create_dereference_node($1); }
	| expr T_DOT T_ID { $$ = create_  }
	| func_call { $$ = $1; }
	| T_STRING { $$ = create_string_literal_node($1); }
	| array_literal { $$ = $1; }

	| expr T_EQ expr { $$ = create_eq_node($1, $3); }
	| expr T_NE expr { $$ = create_ne_node($1, $3); }
	| expr T_LT expr { $$ = create_lt_node($1, $3); }
	| expr T_LE expr { $$ = create_le_node($1, $3); }
	| expr T_GT expr { $$ = create_gt_node($1, $3); }
	| expr T_GE expr { $$ = create_ge_node($1, $3); }
	;

array_literal:
			 T_LBRACE expr_list T_RBRACE { $$ = create_array_literal_node($2); }
			 ;

expr_list:
		{ $$ = create_node_list(); }
		| expr { $$ = create_node_list(); node_list_add($$, $1); }
		| expr_list T_COMMA expr { node_list_add($1, $3); $$ = $1; }
		;
%%
