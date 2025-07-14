#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>

int main(int argc, char **argv) {
  LLVMContextRef ctx = LLVMContextCreate();
  LLVMModuleRef mod = LLVMModuleCreateWithNameInContext("ignis", ctx);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

  // Add function
  LLVMTypeRef param_types[] = {LLVMInt32TypeInContext(ctx),
                               LLVMInt32TypeInContext(ctx)};
  LLVMTypeRef add_type =
      LLVMFunctionType(LLVMInt32TypeInContext(ctx), param_types, 2, 0);
  LLVMValueRef add_func = LLVMAddFunction(mod, "add", add_type);
  LLVMBasicBlockRef add_bb =
      LLVMAppendBasicBlockInContext(ctx, add_func, "entry");
  LLVMPositionBuilderAtEnd(builder, add_bb);
  LLVMValueRef x = LLVMGetParam(add_func, 0);
  LLVMValueRef y = LLVMGetParam(add_func, 1);
  LLVMValueRef sum = LLVMBuildAdd(builder, x, y, "sum");
  LLVMBuildRet(builder, sum);

  // Main function
  LLVMTypeRef main_type =
      LLVMFunctionType(LLVMInt32TypeInContext(ctx), NULL, 0, 0);
  LLVMValueRef main_func = LLVMAddFunction(mod, "main", main_type);
  LLVMBasicBlockRef main_bb =
      LLVMAppendBasicBlockInContext(ctx, main_func, "entry");
  LLVMPositionBuilderAtEnd(builder, main_bb);
  LLVMValueRef const2 = LLVMConstInt(LLVMInt32TypeInContext(ctx), 2, 0);
  LLVMValueRef const3 = LLVMConstInt(LLVMInt32TypeInContext(ctx), 5, 0);
  LLVMValueRef consts[] = {const2, const3};
  LLVMValueRef call =
      LLVMBuildCall2(builder, add_type, add_func, consts, 2, "call_add");
  LLVMBuildRet(builder, call);

  LLVMPrintModuleToFile(mod, "simple.ll", NULL);

  LLVMDisposeBuilder(builder);
  LLVMDisposeModule(mod);
  LLVMContextDispose(ctx);
  return 0;
}
