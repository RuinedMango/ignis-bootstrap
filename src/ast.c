#include "ast.h"
#include <stdio.h>
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

ASTNode* create_null_node() {
    ASTNode* n = new_node(AST_NULL);
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

ASTNode* create_type_node(const char* type) {
    ASTNode* n = new_node(AST_TYPE);
    size_t len = strlen(type) + 1;
    n->u.type = malloc(len);
    memcpy(n->u.type, type, len);
    return n;
}
ASTNode* create_array_type_node(ASTNode* base_type, int size) {
    ASTNode* n = new_node(AST_ARRAY_TYPE);
    n->u.arraytype.base_type = base_type;
    n->u.arraytype.size = size;
    return n;
}
ASTNode* create_ptr_type_node(ASTNode* base_type) {
    ASTNode* n = new_node(AST_PTR_TYPE);
    n->u.ptrtype.base_type = base_type;
    return n;
}

ASTNode* create_struct_def_node(const char* name, ASTNodeList* fields) {
    ASTNode* n = new_node(AST_STRUCT_DEF);
    size_t len = strlen(name) + 1;
    n->u.structdef.name = malloc(len);
    memcpy(n->u.structdef.name, name, len);
    n->u.structdef.fields = fields;
    return n;
}
ASTNode* create_field_access_node(ASTNode* left, const char* field) {
    ASTNode* n = new_node(AST_FIELD_ACCESS);
    n->u.fieldaccess.left = left;
    size_t len = strlen(field) + 1;
    n->u.fieldaccess.field = malloc(len);
    memcpy(n->u.fieldaccess.field, field, len);
    return n;
}

ASTNode* create_eq_node(ASTNode* lhs, ASTNode* rhs) {
    ASTNode* n = new_node(AST_EQ);
    n->u.comp.lhs = lhs;
    n->u.comp.rhs = rhs;
    return n;
}
ASTNode* create_ne_node(ASTNode* lhs, ASTNode* rhs) {
    ASTNode* n = new_node(AST_NE);
    n->u.comp.lhs = lhs;
    n->u.comp.rhs = rhs;
    return n;
}
ASTNode* create_lt_node(ASTNode* lhs, ASTNode* rhs) {
    ASTNode* n = new_node(AST_LT);
    n->u.comp.lhs = lhs;
    n->u.comp.rhs = rhs;
    return n;
}
ASTNode* create_le_node(ASTNode* lhs, ASTNode* rhs) {
    ASTNode* n = new_node(AST_LE);
    n->u.comp.lhs = lhs;
    n->u.comp.rhs = rhs;
    return n;
}
ASTNode* create_gt_node(ASTNode* lhs, ASTNode* rhs) {
    ASTNode* n = new_node(AST_GT);
    n->u.comp.lhs = lhs;
    n->u.comp.rhs = rhs;
    return n;
}
ASTNode* create_ge_node(ASTNode* lhs, ASTNode* rhs) {
    ASTNode* n = new_node(AST_GE);
    n->u.comp.lhs = lhs;
    n->u.comp.rhs = rhs;
    return n;
}

ASTNode* create_if_node(ASTNode* cond, ASTNode* then_body, ASTNode* else_body) {
    ASTNode* n = new_node(AST_IF);
    n->u.ifstmt.cond = cond;
    n->u.ifstmt.then_body = then_body;
    n->u.ifstmt.else_body = else_body;
    return n;
}

ASTNode* create_fn_node(ASTNode* ret_type, const char* name,
                        ASTNodeList* params, ASTNodeList* body,
                        const char* callconv) {
    ASTNode* n = new_node(AST_FN);
    n->u.fn.rettype = ret_type;
    size_t len = strlen(name) + 1;
    n->u.fn.name = malloc(len);
    memcpy(n->u.fn.name, name, len);
    n->u.fn.params = params;
    n->u.fn.body = body;
    len = strlen(callconv) + 1;
    n->u.fn.callconv = malloc(len);
    memcpy(n->u.fn.callconv, callconv, len);
    return n;
}
ASTNode* create_extern_fn_node(ASTNode* ret_type, const char* name,
                               ASTNodeList* params, const char* callconv) {
    ASTNode* n = new_node(AST_EXTERN_FN);
    n->u.externfn.rettype = ret_type;
    size_t len = strlen(name) + 1;
    n->u.externfn.name = malloc(len);
    memcpy(n->u.externfn.name, name, len);
    n->u.externfn.params = params;
    len = strlen(callconv) + 1;
    n->u.externfn.callconv = malloc(len);
    memcpy(n->u.externfn.callconv, callconv, len);
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
ASTNode* create_block_node(ASTNodeList* stmts) {
    ASTNode* n = new_node(AST_BLOCK);
    n->u.block.stmts = stmts;
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
ASTNode* create_var_decl_node(ASTNode* type, const char* name, ASTNode* value,
                              int is_signed) {
    ASTNode* n = new_node(AST_VAR_DECL);
    n->u.vardecl.type = type;
    size_t len = strlen(name) + 1;
    n->u.vardecl.name = malloc(len);
    memcpy(n->u.vardecl.name, name, len);
    n->u.vardecl.value = value;
    n->u.vardecl.is_signed = is_signed;
    return n;
}
ASTNode* create_con_decl_node(ASTNode* type, const char* name, ASTNode* value,
                              int is_signed) {
    ASTNode* n = new_node(AST_CON_DECL);
    n->u.vardecl.type = type;
    size_t len = strlen(name) + 1;
    n->u.vardecl.name = malloc(len);
    memcpy(n->u.vardecl.name, name, len);
    n->u.vardecl.value = value;
    n->u.vardecl.is_signed = is_signed;
    return n;
}
ASTNode* create_id_node(const char* name) {
    ASTNode* n = new_node(AST_ID);
    size_t len = strlen(name) + 1;
    n->u.varname = malloc(len);
    memcpy(n->u.varname, name, len);
    return n;
}

ASTNode* create_address_of_node(const char* id) {
    ASTNode* n = new_node(AST_ADDRESS_OF);
    size_t len = strlen(id) + 1;
    n->u.addressof.id = malloc(len);
    memcpy(n->u.addressof.id, id, len);
    return n;
}
ASTNode* create_dereference_node(const char* id) {
    ASTNode* n = new_node(AST_DEREFERENCE);
    size_t len = strlen(id) + 1;
    n->u.dereference.id = malloc(len);
    memcpy(n->u.dereference.id, id, len);
    return n;
}

ASTNode* create_string_literal_node(const char* str) {
    ASTNode* n = new_node(AST_STRING_LITERAL);
    size_t len = strlen(str) + 1;
    n->u.stringval = malloc(len);
    memcpy(n->u.stringval, str, len);
    return n;
}
ASTNode* create_array_literal_node(ASTNodeList* elements) {
    ASTNode* n = new_node(AST_ARRAY_LITERAL);
    n->u.arraylit.elements = elements;
    return n;
}
ASTNode* create_array_index_node(ASTNode* array, ASTNode* index) {
    ASTNode* n = new_node(AST_ARRAY_INDEX);
    n->u.arrayindex.array = array;
    n->u.arrayindex.index = index;
    return n;
}

void free_node(ASTNode* e) {
    if (!e)
        return;
    switch (e->kind) {
        case AST_INT:
            break;
        case AST_FLOAT:
            break;

        case AST_TYPE:
            free(e->u.type);
            break;
        case AST_ARRAY_TYPE:
            free(e->u.arraytype.base_type);
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

        case AST_ARRAY_LITERAL:
            free_node_list(e->u.arraylit.elements);
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
