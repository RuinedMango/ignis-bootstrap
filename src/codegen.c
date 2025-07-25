#include "ast.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <stdio.h>
#include <string.h>

static LLVMContextRef ctx;
static LLVMModuleRef mod;
static LLVMBuilderRef builder;

void init_codegen() {
    ctx = LLVMContextCreate();
    mod = LLVMModuleCreateWithNameInContext("ignis", ctx);
    builder = LLVMCreateBuilderInContext(ctx);
}

LLVMTypeRef typeSwatch(char* type) {
    if (strcmp(type, "void") == 0) {
        return LLVMVoidTypeInContext(ctx);
    } else if (strcmp(type, "i8")) {
        return LLVMInt8TypeInContext(ctx);
    }
}

static LLVMValueRef codegen_expr(ASTNode* e) {
    switch (e->kind) {
        case AST_INT:
            return LLVMConstInt(LLVMInt32TypeInContext(ctx), e->u.intval, 0);
        case AST_FLOAT:
            LLVMConstReal(LLVMFloatTypeInContext(ctx), e->u.floatval);
        case AST_FN:
            LLVMTypeRef ftype =
                LLVMFunctionType(typeSwatch(e->u.fn.rettype), NULL, 0, 0);
            LLVMValueRef fn = LLVMAddFunction(mod, e->u.fn.name);
            break;
        case AST_VAR_REF:
            fprintf(stderr, "Unknown variable %s\n", e->u.varname);
        case AST_BINOP:
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
        case AST_RETURN:
            return LLVMBuildRet(
                builder, LLVMConstInt(LLVMInt32TypeInContext(ctx), 0, 0));
    }
    return NULL;
}

void evaluate_and_codegen(ASTNode* e) {
    LLVMTypeRef ftype =
        LLVMFunctionType(LLVMInt32TypeInContext(ctx), NULL, 0, 0);
    LLVMValueRef fn = LLVMAddFunction(mod, "main", ftype);
    LLVMBasicBlockRef bb = LLVMAppendBasicBlockInContext(ctx, fn, "entry");
    LLVMPositionBuilderAtEnd(builder, bb);

    LLVMValueRef val = codegen_expr(e);
    LLVMBuildRet(builder, val);

    LLVMVerifyFunction(fn, LLVMAbortProcessAction);
}

void finish_codegen() {
    LLVMPrintModuleToFile(mod, "simple.ll", NULL);
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(mod);
    LLVMContextDispose(ctx);
}
