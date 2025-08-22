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
    LLVMTypeRef base_type;
} ValueType;

Symbol* symbol_table = NULL;

void symbol_put(const char* name, ValueType* valtype) {
    Symbol* sym = malloc(sizeof(Symbol));
    size_t len = strlen(name) + 1;
    sym->name = malloc(len);
    memcpy(sym->name, name, len);
    sym->valtype = valtype;
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
    ValueType* i8toi32valtype = malloc(sizeof(ValueType));
    i8toi32valtype->type = i8toi32_type;
    i8toi32valtype->value = i8toi32;
    i8toi32valtype->is_signed = 0;
    i8toi32valtype->base_type = NULL;

    symbol_put("i8toi32", i8toi32valtype);

    LLVMTypeRef i32toi8_type = LLVMFunctionType(int8_type, &int32_type, 1, 0);
    LLVMValueRef i32toi8 = LLVMAddFunction(mod, "i32toi8", i32toi8_type);
    ValueType* i32toi8valtype = malloc(sizeof(ValueType));
    i32toi8valtype->type = i32toi8_type;
    i32toi8valtype->value = i32toi8;
    i32toi8valtype->is_signed = 0;
    i32toi8valtype->base_type = NULL;

    symbol_put("i32toi8", i32toi8valtype);
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
    } else if (strcmp(type, "*opaqueptr")) {
        return LLVMPointerType(int8_type, 0);
    }
    return NULL;
}

ValueType* handleType(ASTNode* type) {
    if (type->kind == AST_TYPE) {
        ValueType* valtype = malloc(sizeof(ValueType));
        valtype->base_type = NULL;
        valtype->type = typeSwatch(type->u.type);
        return valtype;
    } else if (type->kind == AST_ARRAY_TYPE) {
        ValueType* valtype = malloc(sizeof(ValueType));
        valtype->base_type = NULL;
        valtype->type =
            LLVMArrayType(handleType(type->u.arraytype.base_type)->type,
                          type->u.arraytype.size);
        return valtype;
    } else if (type->kind == AST_PTR_TYPE) {
        ValueType* valtype = malloc(sizeof(ValueType));
        LLVMTypeRef base_type = handleType(type->u.ptrtype.base_type)->type;
        valtype->base_type = base_type;
        valtype->type = LLVMPointerType(base_type, 0);
        return valtype;
    }

    return NULL;
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
        case AST_NULL: {
            LLVMTypeRef ptr_type = LLVMPointerType(int8_type, 0);
            return LLVMConstNull(ptr_type);
        }
        case AST_INT: {
            return LLVMConstInt(int32_type, e->u.intval, 0);
        }
        case AST_FLOAT: {
            return LLVMConstReal(float_type, e->u.floatval);
        }
        case AST_FN: {
            break;
        }
        case AST_EXTERN_FN: {
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
            if (LLVMGetTypeKind(LLVMGetReturnType(callee->type)) ==
                LLVMVoidTypeKind) {
                LLVMBuildCall2(builder, callee->type, callee->value, args,
                               arg_count, "");
                return NULL;
            }
            LLVMValueRef result =
                LLVMBuildCall2(builder, callee->type, callee->value, args,
                               arg_count, "calltmp");
            free(args);
            return result;
        }
        case AST_BLOCK: {
            for (int i = 0; i < e->u.block.stmts->count; i++) {
                codegen_expr(e->u.block.stmts->nodes[i]);
            }

            return NULL;
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
        case AST_IF: {
            LLVMValueRef condv = codegen_expr(e->u.ifstmt.cond);

            LLVMBasicBlockRef cur_block = LLVMGetInsertBlock(builder);
            LLVMValueRef cur_fn = LLVMGetBasicBlockParent(cur_block);

            LLVMBasicBlockRef then_bb =
                LLVMAppendBasicBlockInContext(ctx, cur_fn, "if.then");
            LLVMBasicBlockRef else_bb =
                e->u.ifstmt.else_body
                    ? LLVMAppendBasicBlockInContext(ctx, cur_fn, "if.else")
                    : NULL;
            LLVMBasicBlockRef merge_bb =
                LLVMAppendBasicBlockInContext(ctx, cur_fn, "if.end");

            if (else_bb) {
                LLVMBuildCondBr(builder, condv, then_bb, else_bb);
            } else {
                LLVMBuildCondBr(builder, condv, then_bb, merge_bb);
            }

            LLVMPositionBuilderAtEnd(builder, then_bb);
            codegen_expr(e->u.ifstmt.then_body);
            if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(builder))) {
                LLVMBuildBr(builder, merge_bb);
            }

            if (else_bb) {
                LLVMPositionBuilderAtEnd(builder, else_bb);
                codegen_expr(e->u.ifstmt.else_body);
                if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(builder))) {
                    LLVMBuildBr(builder, merge_bb);
                }
            }

            LLVMPositionBuilderAtEnd(builder, merge_bb);
            return NULL;
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
            ValueType* decl_valtype = handleType(e->u.vardecl.type);

            LLVMValueRef init = codegen_expr(e->u.vardecl.value);
            init = autocast(init, LLVMTypeOf(init), decl_valtype->type);
            if (!init) {
                fprintf(stderr, "Error: codegen failed on; %s",
                        e->u.vardecl.name);
                exit(1);
            }

            LLVMValueRef alloc =
                LLVMBuildAlloca(builder, decl_valtype->type, e->u.vardecl.name);
            LLVMBuildStore(builder, init, alloc);
            decl_valtype->value = alloc;
            decl_valtype->is_signed = 0;
            symbol_put(e->u.vardecl.name, decl_valtype);
            return alloc;
        }
        case AST_ID: {
            ValueType* var = symbol_get(e->u.varname);
            if (!var) {
                fprintf(stderr, "Unknown variable %s\n", e->u.varname);
                exit(1);
            }
            LLVMTypeRef outtype = NULL;
            outtype = var->type;
            return LLVMBuildLoad2(builder, outtype, var->value, e->u.varname);
        }
        case AST_ADDRESS_OF: {
            ValueType* var = symbol_get(e->u.addressof.id);
            if (!var) {
                fprintf(stderr, "Unknown variable %s\n", e->u.addressof.id);
                exit(1);
            }
            return var->value;
        }
        case AST_DEREFERENCE: {
            ValueType* var = symbol_get(e->u.dereference.id);
            if (!var->base_type) {
                fprintf(stderr, "Fuck %s:%s\n", e->u.dereference.id,
                        LLVMPrintTypeToString(var->base_type));
                exit(1);
            }
            LLVMValueRef init =
                LLVMBuildLoad2(builder, var->type, var->value, "preref");
            return LLVMBuildLoad2(builder, var->base_type, init, "deref");
        }
        case AST_STRING_LITERAL: {
            size_t len = strlen(e->u.stringval) + 1;
            LLVMTypeRef array_ty = LLVMArrayType(int8_type, len);

            static int str_count = 0;
            char global_name[32];
            sprintf(global_name, ".str.%d", str_count++);

            LLVMValueRef global = LLVMAddGlobal(mod, array_ty, global_name);
            LLVMSetInitializer(global, LLVMConstStringInContext(
                                           ctx, e->u.stringval, len - 1, 0));
            LLVMSetGlobalConstant(global, 0);
            LLVMSetLinkage(global, LLVMPrivateLinkage);

            LLVMValueRef zero = LLVMConstInt(int32_type, 0, 1);
            LLVMValueRef indices[] = {zero, zero};
            return LLVMBuildInBoundsGEP2(builder, array_ty, global, indices, 2,
                                         "strptr");
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
        case AST_EQ: {
            return LLVMBuildICmp(builder, LLVMIntEQ,
                                 codegen_expr(e->u.comp.lhs),
                                 codegen_expr(e->u.comp.rhs), "cmpeq");
        }
        case AST_NE: {
            return LLVMBuildICmp(builder, LLVMIntNE,
                                 codegen_expr(e->u.comp.lhs),
                                 codegen_expr(e->u.comp.rhs), "cmpne");
        }
        case AST_LT: {
            return LLVMBuildICmp(builder, LLVMIntSLT,
                                 codegen_expr(e->u.comp.lhs),
                                 codegen_expr(e->u.comp.rhs), "cmplt");
        }
        case AST_LE: {
            return LLVMBuildICmp(builder, LLVMIntSLE,
                                 codegen_expr(e->u.comp.lhs),
                                 codegen_expr(e->u.comp.rhs), "cmple");
        }
        case AST_GT: {
            return LLVMBuildICmp(builder, LLVMIntSGT,
                                 codegen_expr(e->u.comp.lhs),
                                 codegen_expr(e->u.comp.rhs), "cmpgt");
        }
        case AST_GE: {
            return LLVMBuildICmp(builder, LLVMIntSGE,
                                 codegen_expr(e->u.comp.lhs),
                                 codegen_expr(e->u.comp.rhs), "cmpge");
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
        ValueType* decl_type =
            handleType(fn->u.fn.params->nodes[i]->u.vardecl.type);
        param_types[i] = decl_type->type;
    }

    ValueType* retvaltype = handleType(fn->u.fn.rettype);
    LLVMTypeRef decl_type = retvaltype->type;
    LLVMTypeRef base_type = retvaltype->base_type;

    LLVMTypeRef func_type =
        LLVMFunctionType(decl_type, param_types, param_count, 0);

    LLVMValueRef function = LLVMAddFunction(mod, fn->u.fn.name, func_type);
    if (strcmp(fn->u.fn.callconv, "cdecl") == 0) {
        LLVMSetFunctionCallConv(function, LLVMCCallConv);
    } else if (strcmp(fn->u.fn.callconv, "fastcall") == 0) {
        LLVMSetFunctionCallConv(function, LLVMFastCallConv);
    }

    ValueType* valtype = malloc(sizeof(ValueType));
    valtype->value = function;
    valtype->type = func_type;
    valtype->base_type = base_type;
    valtype->is_signed = 0;

    symbol_put(fn->u.fn.name, valtype);
    free(param_types);
    return function;
}

LLVMValueRef declare_extern_func(ASTNode* fn) {
    int param_count = fn->u.externfn.params->count;

    LLVMTypeRef* param_types = malloc(sizeof(LLVMTypeRef) * param_count);
    for (int i = 0; i < param_count; i++) {
        ValueType* decl_type =
            handleType(fn->u.externfn.params->nodes[i]->u.vardecl.type);
        param_types[i] = decl_type->type;
    }

    ValueType* retvaltype = handleType(fn->u.externfn.rettype);
    LLVMTypeRef decl_type = retvaltype->type;

    LLVMTypeRef func_type =
        LLVMFunctionType(decl_type, param_types, param_count, 0);

    LLVMValueRef function =
        LLVMAddFunction(mod, fn->u.externfn.name, func_type);
    if (strcmp(fn->u.externfn.callconv, "cdecl") == 0) {
        LLVMSetFunctionCallConv(function, LLVMCCallConv);
    } else if (strcmp(fn->u.externfn.callconv, "fastcall") == 0) {
        LLVMSetFunctionCallConv(function, LLVMFastCallConv);
    }

    ValueType* valtype = malloc(sizeof(ValueType));
    valtype->value = function;
    valtype->type = func_type;
    valtype->base_type = retvaltype->base_type;
    valtype->is_signed = 0;

    symbol_put(fn->u.externfn.name, valtype);
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
        LLVMTypeRef decl_type =
            handleType(fn->u.fn.params->nodes[i]->u.vardecl.type)->type;
        LLVMValueRef alloc =
            LLVMBuildAlloca(builder, decl_type, param->u.vardecl.name);
        LLVMBuildStore(builder, value, alloc);
        ValueType* valtype = malloc(sizeof(ValueType));
        valtype->type = decl_type;
        valtype->value = alloc;
        valtype->base_type = NULL;
        valtype->is_signed = 0;
        symbol_put(param->u.vardecl.name, valtype);
    }

    for (int i = 0; i < fn->u.fn.body->count; i++) {
        codegen_expr(fn->u.fn.body->nodes[i]);
    }

    if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(builder)) == NULL) {
        LLVMBuildRetVoid(builder);
    }
}

void codegen(ASTNodeList* functions) {
    for (int i = 0; i < functions->count; i++) {
        if (functions->nodes[i]->kind == AST_FN) {
            declare_func(functions->nodes[i]);
        }
        if (functions->nodes[i]->kind == AST_EXTERN_FN) {
            declare_extern_func(functions->nodes[i]);
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
