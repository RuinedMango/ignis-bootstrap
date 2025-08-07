#ifndef AST_H
#define AST_H

typedef enum {
    AST_INT,
    AST_FLOAT,

    AST_TYPE,
    AST_ARRAY_TYPE,

    AST_FN,
    AST_FN_CALL,

    AST_RETURN,

    AST_BINOP,
    AST_ASSIGN,
    AST_VAR_DECL,
    AST_CON_DECL,
    AST_ID,

    AST_ARRAY_LITERAL,
    AST_ARRAY_INDEX
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

        char* type;
        struct {
            char* base_type;
            int size;
        } arraytype;

        struct {
            struct ASTNode* rettype;
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
            struct ASTNode* type;
            char* name;
            struct ASTNode* value;
            int is_signed;
        } vardecl;
        char* varname;

        struct {
            ASTNodeList* elements;
        } arraylit;
        struct {
            struct ASTNode* array;
            struct ASTNode* index;
        } arrayindex;
    } u;
} ASTNode;

ASTNodeList* create_node_list();
void node_list_add(ASTNodeList* list, ASTNode* node);
void free_node_list(ASTNodeList* list);

ASTNode* create_int_node(int v);
ASTNode* create_float_node(float f);

ASTNode* create_type_node(const char* type);
ASTNode* create_array_type_node(const char* base_type, int size);

ASTNode* create_fn_node(ASTNode* ret_type, const char* name,
                        ASTNodeList* params, ASTNodeList* body);
ASTNode* create_fn_call_node(const char* name, ASTNodeList* args);

ASTNode* create_return_node(ASTNode* value);

ASTNode* create_binop_node(char op, ASTNode* l, ASTNode* r);
ASTNode* create_assign_node(const char* name, ASTNode* value);
ASTNode* create_var_decl_node(ASTNode* type, const char* name, ASTNode* value,
                              int is_signed);
ASTNode* create_con_decl_node(ASTNode* type, const char* name, ASTNode* value,
                              int is_signed);
ASTNode* create_id_node(const char* name);

ASTNode* create_array_literal_node(ASTNodeList* elements);
ASTNode* create_array_index_node(ASTNode* array, ASTNode* index);

void free_node(ASTNode* e);

void codegen(ASTNodeList* funcs);
void init_codegen();
void finish_codegen();

#endif
