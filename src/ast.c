#include "ast.h"
#include <stdlib.h>
#include <string.h>

ASTNodeList* create_node_list() {
    ASTNodeList* list = malloc(sizeof(ASTNodeList));
    list->count = 0;
    list->capacity = 4;
    list->nodes = malloc(sizeof(ASTNode*) * list->capacity);
    return list;
}
void node_list_add(ASTNodeList* list, ASTNode* node) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->nodes = realloc(list->nodes, sizeof(ASTNode*) * list->capacity);
    }
    list->nodes[list->count++] = node;
}
void free_node_list(ASTNodeList* list) {
    free(list->nodes);
    free(list);
}

static ASTNode* new_node(ASTKind kind) {
    ASTNode* n = malloc(sizeof(ASTNode));
    n->kind = kind;
    n->children = create_node_list();
    return n;
}

ASTNode* create_int_node(int v) {
    ASTNode* n = new_node(AST_INT);
    n->u.intval = v;
    return n;
}
ASTNode* create_float_node(float f) {
    ASTNode* n = new_node(AST_FLOAT);
    n->u.floatval = f;
    return n;
}

ASTNode* create_fn_node(const char* ret_type, const char* name,
                        ASTNodeList* params, ASTNodeList* body) {
    ASTNode* n = new_node(AST_FN);
    size_t len = strlen(ret_type) + 1;
    n->u.fn.rettype = malloc(len);
    memcpy(n->u.fn.rettype, ret_type, len);
    len = strlen(name) + 1;
    n->u.fn.name = malloc(len);
    memcpy(n->u.fn.name, name, len);
    n->u.fn.params = params;
    n->u.fn.body = body;
    return n;
}
ASTNode* create_fn_call_node(const char* name, ASTNodeList* args) {
    ASTNode* n = new_node(AST_FN_CALL);
    size_t len = strlen(name) + 1;
    n->u.call.name = malloc(len);
    memcpy(n->u.call.name, name, len);
    n->u.call.args = args;
    return n;
}

ASTNode* create_return_node(ASTNode* value) {
    ASTNode* n = new_node(AST_RETURN);
    n->u.retval = value;
    return n;
}

ASTNode* create_binop_node(char op, ASTNode* l, ASTNode* r) {
    ASTNode* n = new_node(AST_BINOP);
    n->u.bin.op = op;
    n->u.bin.lhs = l;
    n->u.bin.rhs = r;
    return n;
}
ASTNode* create_assign_node(const char* name, ASTNode* value) {
    ASTNode* n = new_node(AST_ASSIGN);
    size_t len = strlen(name) + 1;
    n->u.assign.name = malloc(len);
    memcpy(n->u.assign.name, name, len);
    n->u.assign.value = value;
    return n;
}
ASTNode* create_var_decl_node(const char* type, const char* name,
                              ASTNode* value) {
    ASTNode* n = new_node(AST_VAR_DECL);
    size_t len = strlen(type) + 1;
    n->u.vardecl.type = malloc(len);
    memcpy(n->u.vardecl.type, type, len);
    len = strlen(name) + 1;
    n->u.vardecl.name = malloc(len);
    memcpy(n->u.vardecl.name, name, len);
    n->u.vardecl.value = value;
    return n;
}
ASTNode* create_con_decl_node(const char* type, const char* name,
                              ASTNode* value) {
    ASTNode* n = new_node(AST_CON_DECL);
    size_t len = strlen(type) + 1;
    n->u.vardecl.type = malloc(len);
    memcpy(n->u.vardecl.type, type, len);
    len = strlen(name) + 1;
    n->u.vardecl.name = malloc(len);
    memcpy(n->u.vardecl.name, name, len);
    n->u.vardecl.value = value;
    return n;
}
ASTNode* create_id_node(const char* name) {
    ASTNode* n = new_node(AST_ID);
    size_t len = strlen(name) + 1;
    n->u.varname = malloc(len);
    memcpy(n->u.varname, name, len);
    return n;
}

ASTNode* create_list_node(ASTKind kind) { return new_node(kind); }

void free_node(ASTNode* e) {
    if (!e)
        return;
    switch (e->kind) {
        case AST_INT:
            break;
        case AST_FLOAT:
            break;

        case AST_FN_CALL:
            free(e->u.call.name);
            for (int i = 0; i < e->u.call.args->count; i++) {
                free_node(e->u.call.args->nodes[i]);
            }
            free_node_list(e->u.call.args);
            break;

        case AST_RETURN:
            free_node(e->u.retval);
            break;

        case AST_BINOP:
            free_node(e->u.bin.lhs);
            free_node(e->u.bin.rhs);
            break;
        case AST_ASSIGN:
        case AST_VAR_DECL:
        case AST_CON_DECL:
            if (e->u.vardecl.type)
                free(e->u.vardecl.type);
            free(e->u.vardecl.name);
            free_node(e->u.vardecl.value);
            break;
        case AST_ID:
            free(e->u.varname);
            break;
        default:
            break;
    }
    for (int i = 0; i < e->children->count; i++) {
        free_node(e->children->nodes[i]);
    }
    free_node_list(e->children);
    free(e);
}
