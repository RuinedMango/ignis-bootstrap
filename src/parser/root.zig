const std = @import("std");
const Allocator = std.mem.Allocator;

const lex = @import("ignis_lexer");

pub const ExprKind = enum { Number, Ident, Unary, Binary, Call };
pub const Expr = union(enum) { Number: struct { value: f64 }, Ident: struct { slice: []u8 }, Unary: struct { op: lex.TType, rhs } };

pub const StmtKind = enum {};
const Stmt = union(enum) {};

const Parser = struct {
    lexer: lex.Lexer,
    alloc: Allocator,
};
