const std = @import("std");
const Allocator = std.mem.Allocator;

const lex = @import("ignis_lexer");

pub const ExprKind = enum { Number, Ident, Unary, Binary };
pub const Expr = union(ExprKind) {
    Number: struct { value: f64 },
    Ident: struct { slice: []u8 },
    Unary: struct { op: lex.TType, rhs: *Expr },
    Binary: struct { op: lex.TType, rhs: *Expr, lhs: *Expr },
    Call: struct { callee: *Expr, args: []*Expr },
    Member: struct { base: *Expr, name: []u8 },
    Cast: struct { expr: *Expr, to: *Type },
    AddressOf: struct { expr: *Expr },
    Null: struct {},
};

pub const TypeKind = enum { Named, Pointer, Array };
pub const Type = union(TypeKind) {
    Named: struct { name: []u8 },
    Pointer: struct { baseType: *Type },
    Array: struct { elemType: *Type, size: ?u32 },
};

pub const StmtKind = enum { ExprStmt, Def, Return, If, While, Block, Fn, Extern };
pub const Stmt = union(StmtKind) {
    ExprStmt: struct { expr: *Expr },
    Def: struct { name: []u8, typePtr: *Type, init: ?*Expr },
    Return: struct { expr: ?*Expr },
    If: struct { cond: *Expr, then_branch: *Stmt, else_branch: ?*Stmt },
    While: struct { cond: *Expr, body: *Stmt },
    Block: struct { stmts: []*Stmt },
    Fn: struct { name: []u8, params: []*Type, ret: ?*Type, stmts: []*Stmt, attrs: [][]u8, is_extern: bool },
    Extern: struct { inner: *Stmt },
};

pub const ParseErrors = error{
    FalseStatement,
};

pub const Parser = struct {
    lexer: *lex.Lexer,

    pub fn init(lex_ptr: *lex.Lexer) Parser {
        return Parser{ .lexer = lex_ptr };
    }

    pub fn accept(self: *Parser, kind: lex.TType) bool {
        const t = self.lexer.peek();
        if (t.type == kind) {
            _ = self.lexer.next();
            return true;
        }
        return false;
    }

    pub fn expect(self: *Parser, kind: lex.TType) lex.Token {
        const t = self.lexer.peek();
        if (t.type != kind) {
            std.log.err("Parse error: expected {s} at {d}:{d}, got {s}\n", .{ @tagName(kind), t.line, t.col, @tagName(t.type) });
            @panic("parse error");
        }
        return self.lexer.next();
    }

    pub fn parseExpression(self: *Parser, min_bp: i32, alloc: Allocator) !*Expr {
        _ = self;
        _ = min_bp;
        const expr = try alloc.create(Expr);
        expr.* = Expr{ .Number = .{ .value = 0 } };
        return expr;
    }

    pub fn parseType(self: *Parser, alloc: Allocator) !*Type {
        var prefix_arrays: std.ArrayList(?u32) = .empty;
        defer prefix_arrays.deinit(alloc);

        while (self.accept(lex.TType.LBRACK)) {
            if (self.accept(lex.TType.RBRACK)) {
                try prefix_arrays.append(alloc, null);
            } else {
                const nTok = self.expect(lex.TType.INT);
                try prefix_arrays.append(alloc, @intCast(nTok.data.int));
            }
        }
    }

    pub fn parseFunctionDecl(self: *Parser, alloc: Allocator, is_extern: bool) !*Stmt {
        const nameTok = self.expect(lex.TType.IDENT);
        _ = self.expect(lex.TType.LPAREN);

        var params: std.ArrayList(*Type) = .empty;
        defer params.deinit(alloc);

        if (!self.accept(lex.TType.RPAREN)) {
            while (true) {
                _ = self.expect(lex.TType.IDENT);
                _ = self.expect(lex.TType.COLON);
                const p_ty = try self.parseType(alloc);
                try params.append(alloc, p_ty);
                if (self.accept(lex.TType.COMMA)) continue;
                _ = self.expect(lex.TType.RPAREN);
                break;
            }
        }

        var ret_ty: ?*Type = null;
        if (self.accept(lex.TType.ARROW)) {
            ret_ty = try self.parseType(alloc);
        }

        const out = alloc.create(Stmt);
        out.* = Stmt{ .Fn = .{ .name = nameTok, .retType = ret_ty.?, .stmts = null, .is_extern = is_extern } };
        return out;
    }

    pub fn parseStatement(self: *Parser, alloc: Allocator) !*Stmt {
        const t = self.lexer.peek();
        if (t.type == lex.TType.DEF) {
            _ = self.lexer.next();
            const nameTok = self.expect(lex.TType.IDENT);
            _ = self.expect(lex.TType.COLON);
            const typeTok = self.expect(lex.TType.TYPE);
            var expr: *Expr = undefined;
            if (self.accept(lex.TType.ASSIGN)) {
                expr = try self.parseExpression(0, alloc);
            }
            _ = self.expect(lex.TType.SEMI);
            const out = try alloc.create(Stmt);
            out.* = Stmt{ .Def = .{ .name = nameTok.data.str, .type = typeTok.data.str, .init = expr } };
            return out;
        } else if (t.type == lex.TType.EXTERN) {
            _ = self.lexer.next();
            const stmt = try self.parseStatement(alloc);
            const out = try alloc.create(Stmt);
            out.* = Stmt{ .Extern = .{ .stmt = stmt } };
        } else if (t.type == lex.TType.FN) {
            _ = self.lexer.next();
            const nameTok = self.expect(lex.TType.IDENT);
        } else if (t.type == lex.TType.RETURN) {
            _ = self.lexer.next();
            var expr: ?*Expr = undefined;
            if (self.lexer.peek().type != lex.TType.SEMI) {
                expr = try self.parseExpression(0, alloc);
            } else {
                expr = null;
            }
            _ = self.expect(lex.TType.SEMI);
            const out = try alloc.create(Stmt);
            out.* = Stmt{ .Return = .{ .expr = expr } };
            return out;
        } else if (t.type == lex.TType.IF) {
            _ = self.lexer.next();
            _ = self.expect(lex.TType.LPAREN);
            const cond = try self.parseExpression(0, alloc);
            _ = self.expect(lex.TType.RPAREN);
            const then_st = try self.parseStatement(alloc);
            var else_st: ?*Stmt = null;
            if (self.accept(lex.TType.ELSE)) else_st = try self.parseStatement(alloc);
            const out = try alloc.create(Stmt);
            out.* = Stmt{ .If = .{ .cond = cond, .then_branch = then_st, .else_branch = else_st } };
            return out;
        } else if (t.type == lex.TType.LBRACE) {
            _ = self.lexer.next();
            var vec: std.ArrayList(*Stmt) = .empty;
            while (self.lexer.peek().type != lex.TType.RBRACE or self.lexer.peek().type != lex.TType.EOF) {
                try vec.append(alloc, try self.parseStatement(alloc));
            }
            const slice = try vec.toOwnedSlice(alloc);
            const out = try alloc.create(Stmt);
            out.* = Stmt{ .Block = .{ .stmts = slice } };
            return out;
        }
        return ParseErrors.FalseStatement;
    }

    pub fn parseProgram(self: *Parser, alloc: Allocator) ![]*Stmt {
        var out: std.ArrayList(*Stmt) = .empty;
        defer out.deinit(alloc);
        while (self.lexer.peek().type != lex.TType.EOF) {
            try out.append(alloc, try self.parseStatement(alloc));
        }
        return try out.toOwnedSlice(alloc);
    }
};

pub fn printStmt(stmt: *Stmt) void {
    switch (stmt.*) {
        StmtKind.Block => {
            for (stmt.Block.stmts) |blkStmt| {
                printStmt(blkStmt);
            }
        },
        StmtKind.Def => {
            std.log.info("Def: {s} : (type ptr) init? {any}\n", .{ stmt.Def.name, if (stmt.Def.init) "yes" else "no" });
        },
        StmtKind.Fn => {
            std.log.info("Fn: {s} (extern={d})\n", .{ stmt.Fn.name, if (stmt.Fn.is_extern) 1 else 0 });
            for (stmt.Fn.stmts) |s| {
                printStmt(s);
            }
        },
        else => {
            std.log.info("{s}", .{stmt});
        }
    }
}
