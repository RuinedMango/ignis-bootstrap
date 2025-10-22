#ifndef AST_H
#define AST_H

typedef enum {
  AST_NULL,
  AST_INT,
  AST_FLOAT,

  AST_TYPE,
  AST_ARRAY_TYPE,
  AST_PTR_TYPE,

  AST_STRUCT_DEF,
  AST_FIELD_ACCESS,

  AST_EQ,
  AST_NE,
  AST_LT,
  AST_LE,
  AST_GT,
  AST_GE,

  AST_IF,

  AST_FN,
  AST_EXTERN_FN,
  AST_FN_CALL,
  AST_BLOCK,

  AST_RETURN,

  AST_BINOP,
  AST_ASSIGN,
  AST_VAR_DECL,
  AST_CON_DECL,
  AST_ID,

  AST_ADDRESS_OF,
  AST_DEREFERENCE,

  AST_STRING_LITERAL,
  AST_ARRAY_LITERAL,
  AST_ARRAY_INDEX
} ASTKind;

typedef struct ASTNodeList {
  struct ASTNode **nodes;
  int count;
  int capacity;
} ASTNodeList;

typedef struct ASTNode {
  ASTKind kind;
  ASTNodeList *children;
  union {
    int intval;
    float floatval;

    char *type;
    struct {
      struct ASTNode *base_type;
      int size;
    } arraytype;
    struct {
      struct ASTNode *base_type;
    } ptrtype;

    struct {
      char *name;
      ASTNodeList *fields;
    } structdef;
    struct {
      struct ASTNode *left;
      char *field;
    } fieldaccess;

    struct {
      struct ASTNode *lhs, *rhs;
    } comp;

    struct {
      struct ASTNode *cond;
      struct ASTNode *then_body;
      struct ASTNode *else_body;
    } ifstmt;

    struct {
      struct ASTNode *rettype;
      char *name;
      ASTNodeList *params;
      ASTNodeList *body;
      char *callconv;
    } fn;
    struct {
      struct ASTNode *rettype;
      char *name;
      ASTNodeList *params;
      char *callconv;
    } externfn;
    struct {
      char *name;
      ASTNodeList *args;
    } call;
    struct {
      ASTNodeList *stmts;
    } block;

    struct ASTNode *retval;

    struct {
      char op;
      struct ASTNode *lhs, *rhs;
    } bin;
    struct {
      char *name;
      struct ASTNode *value;
    } assign;
    struct {
      struct ASTNode *type;
      char *name;
      struct ASTNode *value;
      int is_signed;
    } vardecl;
    char *varname;

    struct {
      char *id;
    } addressof;
    struct {
      char *id;
    } dereference;

    char *stringval;
    struct {
      ASTNodeList *elements;
    } arraylit;
    struct {
      struct ASTNode *array;
      struct ASTNode *index;
    } arrayindex;
  } u;
} ASTNode;

ASTNodeList *create_node_list();
void node_list_add(ASTNodeList *list, ASTNode *node);
void free_node_list(ASTNodeList *list);

ASTNode *create_null_node();
ASTNode *create_int_node(int v);
ASTNode *create_float_node(float f);

ASTNode *create_type_node(const char *type);
ASTNode *create_array_type_node(ASTNode *base_type, int size);
ASTNode *create_ptr_type_node(ASTNode *base_type);

ASTNode *create_struct_def_node(const char *name, ASTNodeList *fields);
ASTNode *create_field_access_node(ASTNode *left, const char *field);

ASTNode *create_eq_node(ASTNode *lhs, ASTNode *rhs);
ASTNode *create_ne_node(ASTNode *lhs, ASTNode *rhs);
ASTNode *create_lt_node(ASTNode *lhs, ASTNode *rhs);
ASTNode *create_le_node(ASTNode *lhs, ASTNode *rhs);
ASTNode *create_gt_node(ASTNode *lhs, ASTNode *rhs);
ASTNode *create_ge_node(ASTNode *lhs, ASTNode *rhs);

ASTNode *create_if_node(ASTNode *cond, ASTNode *then_body, ASTNode *else_body);

ASTNode *create_fn_node(ASTNode *ret_type, const char *name,
                        ASTNodeList *params, ASTNodeList *body,
                        const char *callconv);
ASTNode *create_extern_fn_node(ASTNode *ret_type, const char *name,
                               ASTNodeList *params, const char *callconv);
ASTNode *create_fn_call_node(const char *name, ASTNodeList *args);
ASTNode *create_block_node(ASTNodeList *stmts);

ASTNode *create_return_node(ASTNode *value);

ASTNode *create_binop_node(char op, ASTNode *l, ASTNode *r);
ASTNode *create_assign_node(const char *name, ASTNode *value);
ASTNode *create_var_decl_node(ASTNode *type, const char *name, ASTNode *value,
                              int is_signed);
ASTNode *create_con_decl_node(ASTNode *type, const char *name, ASTNode *value,
                              int is_signed);
ASTNode *create_id_node(const char *name);

ASTNode *create_address_of_node(const char *id);
ASTNode *create_dereference_node(const char *id);

ASTNode *create_string_literal_node(const char *str);
ASTNode *create_array_literal_node(ASTNodeList *elements);
ASTNode *create_array_index_node(ASTNode *array, ASTNode *index);

void free_node(ASTNode *e);

void codegen(ASTNodeList *funcs);
void init_codegen();
void finish_codegen();

#endif
