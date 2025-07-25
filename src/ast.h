#ifndef AST_H
#define AST_H

typedef enum {
    AST_INT,
    AST_FLOAT,

    AST_FN,
    AST_FN_CALL,

    AST_RETURN,

    AST_BINOP,
    AST_ASSIGN,
    AST_VAR_DECL,
    AST_CON_DECL,
    AST_ID,

    AST_PARAM_LIST,
    AST_STMT_LIST,
    AST_EXPR_LIST,
} ASTKind;

typedef struct ASTNodeList {
    struct ASTNode** nodes;
    int count;
    int capacity;
} ASTNodeList;

typedef struct ASTNode {
    ASTKind kind;
    ASTNodeList* children;
    union {
        int intval;
        float floatval;

        struct {
            char* rettype;
            char* name;
            ASTNodeList* params;
            ASTNodeList* body;
        } fn;
        struct {
            char* name;
            ASTNodeList* args;
        } call;

        struct ASTNode* retval;

        struct {
            char op;
            struct ASTNode *lhs, *rhs;
        } bin;
        struct {
            char* name;
            struct ASTNode* value;
        } assign;
        struct {
            char* type;
            char* name;
            struct ASTNode* value;
        } vardecl;
        char* varname;
    } u;
} ASTNode;

ASTNodeList* create_node_list();
void node_list_add(ASTNodeList* list, ASTNode* node);
void free_node_list(ASTNodeList* list);

ASTNode* create_int_node(int v);
ASTNode* create_float_node(float f);

ASTNode* create_fn_node(const char* ret_type, const char* name,
                        ASTNodeList* params, ASTNodeList* body);
ASTNode* create_fn_call_node(const char* name, ASTNodeList* args);

ASTNode* create_return_node(ASTNode* value);

ASTNode* create_binop_node(char op, ASTNode* l, ASTNode* r);
ASTNode* create_assign_node(const char* name, ASTNode* value);
ASTNode* create_var_decl_node(const char* type, const char* name,
                              ASTNode* value);
ASTNode* create_con_decl_node(const char* type, const char* name,
                              ASTNode* value);
ASTNode* create_id_node(const char* name);

ASTNode* create_list_node(ASTKind kind);

void free_node(ASTNode* e);

void evaluate_and_codegen(ASTNode* e);
void init_codegen();
void finish_codegen();

#endif
