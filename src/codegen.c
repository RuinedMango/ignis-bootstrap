#include "ast.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <stdio.h>
#include <string.h>

static LLVMContextRef ctx;
static LLVMModuleRef mod;
static LLVMBuilderRef builder;
LLVMTypeRef void_type;
LLVMTypeRef int8_type;
LLVMTypeRef int16_type;
LLVMTypeRef int32_type;
LLVMTypeRef int64_type;
LLVMTypeRef float_type;

typedef struct Symbol {
    char* name;
    struct ValueType* valtype;
    struct Symbol* next;
} Symbol;
typedef struct ValueType {
    LLVMValueRef value;
    LLVMTypeRef type;
    int is_signed;
} ValueType;

Symbol* symbol_table = NULL;

void symbol_put(const char* name, LLVMValueRef value, LLVMTypeRef type,
                int is_signed) {
    Symbol* sym = malloc(sizeof(Symbol));
    size_t len = strlen(name) + 1;
    sym->name = malloc(len);
    memcpy(sym->name, name, len);
    ValueType* valutype = malloc(sizeof(ValueType));
    valutype->value = value;
    valutype->type = type;
    valutype->is_signed = is_signed;
    sym->valtype = valutype;
    sym->next = symbol_table;
    symbol_table = sym;
}

ValueType* symbol_get(const char* name) {
    for (Symbol* s = symbol_table; s != NULL; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            return s->valtype;
        }
    }
    return NULL;
}

void register_builtin_casts() {
    LLVMTypeRef i8toi32_type = LLVMFunctionType(int32_type, &int8_type, 1, 0);
    LLVMValueRef i8toi32 = LLVMAddFunction(mod, "i8toi32", i8toi32_type);

    symbol_put("i8toi32", i8toi32, i8toi32_type, 0);

    LLVMTypeRef i32toi8_type = LLVMFunctionType(int8_type, &int32_type, 1, 0);
    LLVMValueRef i32toi8 = LLVMAddFunction(mod, "i32toi8", i32toi8_type);

    symbol_put("i32toi8", i32toi8, i32toi8_type, 0);
}

void init_codegen() {
    ctx = LLVMContextCreate();
    mod = LLVMModuleCreateWithNameInContext("ignis", ctx);
    builder = LLVMCreateBuilderInContext(ctx);
    void_type = LLVMVoidTypeInContext(ctx);
    int8_type = LLVMInt8TypeInContext(ctx);
    int16_type = LLVMInt16TypeInContext(ctx);
    int32_type = LLVMInt32TypeInContext(ctx);
    int64_type = LLVMInt64TypeInContext(ctx);
    float_type = LLVMFloatTypeInContext(ctx);
    register_builtin_casts();
}

LLVMTypeRef typeSwatch(char* type) {
    if (strcmp(type, "void") == 0) {
        return void_type;
    } else if (strcmp(type, "i8") == 0) {
        return int8_type;
    } else if (strcmp(type, "i16") == 0) {
        return int16_type;
    } else if (strcmp(type, "i32") == 0) {
        return int32_type;
    } else if (strcmp(type, "i64") == 0) {
        return int64_type;
    } else if (strcmp(type, "u8") == 0) {
        return int8_type;
    } else if (strcmp(type, "u16") == 0) {
        return int16_type;
    } else if (strcmp(type, "u32") == 0) {
        return int32_type;
    } else if (strcmp(type, "u64") == 0) {
        return int64_type;
    } else if (strcmp(type, "float") == 0) {
        return float_type;
    }
    return NULL;
}

LLVMTypeRef arrayTypeHandler(ASTNode* arrayType) {
    return LLVMArrayType(typeSwatch(arrayType->u.arraytype.base_type),
                         arrayType->u.arraytype.size);
}

LLVMValueRef autocast(LLVMValueRef value, LLVMTypeRef from, LLVMTypeRef to) {
    if (from == to)
        return value;

    if (LLVMGetTypeKind(from) == LLVMIntegerTypeKind &&
        LLVMGetTypeKind(to) == LLVMIntegerTypeKind) {
        return LLVMBuildIntCast(builder, value, to, "casttmp");
    }

    fprintf(stderr, "(%s):(%s)", LLVMPrintTypeToString(from),
            LLVMPrintTypeToString(to));

    fprintf(stderr, "Unsupported autocast\n");
    exit(1);
}

LLVMValueRef codegen_expr(ASTNode* e) {
    switch (e->kind) {
        case AST_INT: {
            return LLVMConstInt(int32_type, e->u.intval, 0);
        }
        case AST_FLOAT: {
            return LLVMConstReal(float_type, e->u.floatval);
        }
        case AST_FN: {
            break;
        }
        case AST_FN_CALL: {
            if (strcmp(e->u.call.name, "i8toi32") == 0) {
                LLVMValueRef arg_val = codegen_expr(e->u.call.args->nodes[0]);
                return LLVMBuildSExt(builder, arg_val, int32_type, "casttmp");
            } else if (strcmp(e->u.call.name, "i32toi8") == 0) {
                LLVMValueRef arg_val = codegen_expr(e->u.call.args->nodes[0]);
                return LLVMBuildTrunc(builder, arg_val, int8_type, "casttmp");
            }
            ValueType* callee = symbol_get(e->u.call.name);
            if (!callee) {
                fprintf(stderr, "Unknown function %s\n", e->u.call.name);
                exit(1);
            }
            int arg_count = e->u.call.args->count;
            LLVMValueRef* args = malloc(sizeof(LLVMValueRef) * arg_count);
            for (int i = 0; i < arg_count; i++) {
                args[i] = codegen_expr(e->u.call.args->nodes[i]);
            }
            LLVMValueRef result =
                LLVMBuildCall2(builder, callee->type, callee->value, args,
                               arg_count, "calltmp");
            free(args);
            return result;
        }
        case AST_RETURN: {
            LLVMValueRef retval = codegen_expr(e->u.retval);
            return LLVMBuildRet(builder, retval);
        }
        case AST_BINOP: {
            LLVMValueRef l = codegen_expr(e->u.bin.lhs);
            LLVMValueRef r = codegen_expr(e->u.bin.rhs);
            if (e->u.bin.lhs->kind == AST_FLOAT ||
                e->u.bin.rhs->kind == AST_FLOAT) {
                if (e->u.bin.op == '+')
                    return LLVMBuildFAdd(builder, l, r, "faddtmp");
                if (e->u.bin.op == '-')
                    return LLVMBuildFSub(builder, l, r, "fsubtmp");
                if (e->u.bin.op == '*')
                    return LLVMBuildFMul(builder, l, r, "fmultmp");
                if (e->u.bin.op == '/')
                    return LLVMBuildFDiv(builder, l, r, "fdivtmp");
            } else {
                if (e->u.bin.op == '+')
                    return LLVMBuildAdd(builder, l, r, "addtmp");
                if (e->u.bin.op == '-')
                    return LLVMBuildSub(builder, l, r, "subtmp");
                if (e->u.bin.op == '*')
                    return LLVMBuildMul(builder, l, r, "multmp");
                if (e->u.bin.op == '/')
                    return LLVMBuildSDiv(builder, l, r, "divtmp");
            }
        }
        case AST_ASSIGN: {
            ValueType* var = symbol_get(e->u.assign.name);
            if (!var) {
                fprintf(stderr, "Unknown variable %s\n", e->u.assign.name);
                exit(1);
            }
            LLVMValueRef val = codegen_expr(e->u.assign.value);
            val = autocast(val, LLVMTypeOf(val), var->type);
            if (!val) {
                fprintf(stderr, "Error: codegen failed on; %s",
                        e->u.assign.name);
                exit(1);
            }
            return LLVMBuildStore(builder, val, var->value);
        }
        case AST_VAR_DECL:
        case AST_CON_DECL: {
            LLVMTypeRef decl_type = NULL;
            if (e->u.vardecl.type->kind == AST_TYPE) {
                decl_type = typeSwatch(e->u.vardecl.type->u.type);
            } else if (e->u.vardecl.type->kind == AST_ARRAY_TYPE) {
                decl_type = arrayTypeHandler(e->u.vardecl.type);
            }

            LLVMValueRef init = codegen_expr(e->u.vardecl.value);
            init = autocast(init, LLVMTypeOf(init), decl_type);
            if (!init) {
                fprintf(stderr, "Error: codegen failed on; %s",
                        e->u.vardecl.name);
                exit(1);
            }
            LLVMValueRef alloc =
                LLVMBuildAlloca(builder, decl_type, e->u.vardecl.name);
            LLVMBuildStore(builder, init, alloc);
            symbol_put(e->u.vardecl.name, alloc, decl_type,
                       e->u.vardecl.is_signed);
            return alloc;
        }
        case AST_ID: {
            ValueType* var = symbol_get(e->u.varname);
            if (!var) {
                fprintf(stderr, "Unknown variable %s\n", e->u.varname);
                exit(1);
            }
            return LLVMBuildLoad2(builder, var->type, var->value, e->u.varname);
        }
        case AST_ARRAY_LITERAL: {
            int n = e->u.arraylit.elements->count;
            LLVMValueRef* vals = malloc(sizeof(LLVMValueRef) * n);
            vals[0] = codegen_expr(e->u.arraylit.elements->nodes[0]);
            LLVMTypeRef elem_ty = LLVMTypeOf(vals[0]);
            for (int i = 1; i < n; i++) {
                LLVMValueRef value =
                    codegen_expr(e->u.arraylit.elements->nodes[i]);
                vals[i] = autocast(value, LLVMTypeOf(value), elem_ty);
            }
            LLVMValueRef arr = LLVMConstArray(elem_ty, vals, n);
            free(vals);
            return arr;
        }
        case AST_ARRAY_INDEX: {
            ValueType* vt = symbol_get(
                e->u.arrayindex.array->u.varname); // assume it's a variable ID
            LLVMValueRef array_ptr = vt->value;
            LLVMTypeRef array_ty = vt->type;

            LLVMValueRef idx = codegen_expr(e->u.arrayindex.index);

            LLVMValueRef zero = LLVMConstInt(int32_type, 0, 0);
            LLVMValueRef indices[2] = {zero, idx};

            LLVMValueRef gep = LLVMBuildInBoundsGEP2(
                builder, array_ty, array_ptr, indices, 2, "elem_ptr");
            return LLVMBuildLoad2(builder, LLVMGetElementType(array_ty), gep,
                                  "elem_val");
        }
        default: {
            break;
        }
    }
    return NULL;
}

LLVMValueRef declare_func(ASTNode* fn) {
    int param_count = fn->u.fn.params->count;

    LLVMTypeRef* param_types = malloc(sizeof(LLVMTypeRef) * param_count);
    for (int i = 0; i < param_count; i++) {
        LLVMTypeRef decl_type = NULL;
        if (fn->u.fn.params->nodes[i]->u.vardecl.type->kind == AST_TYPE) {
            decl_type =
                typeSwatch(fn->u.fn.params->nodes[i]->u.vardecl.type->u.type);
        } else if (fn->u.fn.params->nodes[i]->u.vardecl.type->kind ==
                   AST_ARRAY_TYPE) {
            decl_type =
                arrayTypeHandler(fn->u.fn.params->nodes[i]->u.vardecl.type);
        }
        param_types[i] = decl_type;
    }

    LLVMTypeRef decl_type = NULL;
    if (fn->u.fn.rettype->kind == AST_TYPE) {
        decl_type = typeSwatch(fn->u.fn.rettype->u.type);
    } else if (fn->u.fn.rettype->kind == AST_ARRAY_TYPE) {
        decl_type = arrayTypeHandler(fn->u.fn.rettype);
    }

    LLVMTypeRef func_type =
        LLVMFunctionType(decl_type, param_types, param_count, 0);

    LLVMValueRef function = LLVMAddFunction(mod, fn->u.fn.name, func_type);
    symbol_put(fn->u.fn.name, function, func_type, 0);
    free(param_types);
    return function;
}

void codegen_func_body(ASTNode* fn) {
    ValueType* fnval = symbol_get(fn->u.fn.name);
    if (!fnval) {
        fprintf(stderr, "Function %s not declared\n", fn->u.fn.name);
        exit(1);
    }

    LLVMValueRef function = fnval->value;
    int param_count = fn->u.fn.params->count;

    LLVMBasicBlockRef entry =
        LLVMAppendBasicBlockInContext(ctx, function, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    for (int i = 0; i < param_count; i++) {
        ASTNode* param = fn->u.fn.params->nodes[i];
        LLVMValueRef value = LLVMGetParam(function, i);
        LLVMTypeRef decl_type = NULL;
        if (fn->u.fn.params->nodes[i]->u.vardecl.type->kind == AST_TYPE) {
            decl_type =
                typeSwatch(fn->u.fn.params->nodes[i]->u.vardecl.type->u.type);
        } else if (fn->u.fn.params->nodes[i]->u.vardecl.type->kind ==
                   AST_ARRAY_TYPE) {
            decl_type =
                arrayTypeHandler(fn->u.fn.params->nodes[i]->u.vardecl.type);
        }
        LLVMValueRef alloc =
            LLVMBuildAlloca(builder, decl_type, param->u.vardecl.name);
        LLVMBuildStore(builder, value, alloc);
        symbol_put(param->u.vardecl.name, alloc, decl_type, 0);
    }

    for (int i = 0; i < fn->u.fn.body->count; i++) {
        codegen_expr(fn->u.fn.body->nodes[i]);
    }

    if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(builder)) == NULL) {
        LLVMBuildRet(builder, LLVMConstInt(int32_type, 0, 0));
    }
}

void codegen(ASTNodeList* functions) {
    for (int i = 0; i < functions->count; i++) {
        if (functions->nodes[i]->kind == AST_FN) {
            declare_func(functions->nodes[i]);
        }
    }

    for (int i = 0; i < functions->count; i++) {
        if (functions->nodes[i]->kind == AST_FN) {
            codegen_func_body(functions->nodes[i]);
        }
    }
}

void finish_codegen() {
    LLVMPrintModuleToFile(mod, "simple.ll", NULL);
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(mod);
    LLVMContextDispose(ctx);
}
