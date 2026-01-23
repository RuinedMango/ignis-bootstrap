const std = @import("std");
const Allocator = std.mem.Allocator;

const lex = @import("ignis_lexer");

pub const ExprKind = enum { Number, Ident, Unary, Binary, Call };
pub const Expr = union(ExprKind) { Number: struct { value: f64 }, Ident: struct { slice: []u8 }, Unary: struct { op: lex.TType, rhs: *Expr }, Binary: struct { op: lex.TType, rhs: *Expr, lhs: *Expr } };

pub const StmtKind = enum { ExprStmt, Def, Return, If, While, Block, Fn };
const Stmt = union(StmtKind) {
    ExprStmt: struct { expr: *Expr },
    Def: struct { name: []u8, init: ?*Expr },
    Return: struct { expr: ?*Expr },
    If: struct { cond: *Expr, then_branch: *Stmt, else_branch: ?*Stmt },
    While: struct { cond: *Expr, body: *Stmt },
    Block: struct { stmts: []*Stmt },
    Fn: struct { stmts: []*Stmt },
};

const Parser = struct {
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

    pub fn parseExpression(slef: *Parser, min_bp: i32) *Expr {}

    pub fn parseStatement(self: *Parser, alloc: Allocator) *Stmt {
        const t = self.lexer.peek();
        if (t.type == lex.TType.DEF) {
            _ = self.lexer.next();
            const nameTok = self.expect(lex.TType.IDENT);
            var expr: *Expr = undefined;
            if (self.accept(lex.TType.ASSIGN)) {
                expr = self.parseExpression(0);
            }
            _ = self.expect(lex.TType.SEMI);
            return &Stmt{.Def{ .name = nameTok.data.str, .init = expr }};
        } else if (t.type == lex.TType.RETURN) {
            _ = self.lexer.next();
            var expr: *Expr = undefined;
            if (self.lexer.peek().type != lex.TType.SEMI) {
                expr = self.parseExpression(0);
            } else {
                expr = null;
            }
            _ = self.expect(lex.TType.SEMI);
            return &Stmt{.Return{ .expr = expr }};
        } else if (t.type == lex.TType.IF) {
            _ = self.lexer.next();
            _ = self.expect(lex.TType.LPAREN);
            const cond = self.parseExpression(0);
            _ = self.expect(lex.TType.RPAREN);
            const then_st = self.parseStatement();
            var else_st: ?*Stmt = null;
            if (self.accept(lex.TType.ELSE)) else_st = self.parseStatement();
            return &Stmt{.If{ .cond = cond, .then_branch = then_st, .else_branch = else_st }};
        } else if (t.type == lex.TType.LBRACE) {
            _ = self.lexer.next();
            var vec: std.ArrayList(*Stmt) = .empty;
            while (self.lexer.peek().type != lex.TType.RBRACE or self.lexer.peek().type != lex.TType.EOF) {
                vec.append(alloc, self.parseStatement(alloc));
            }
            const slice = vec.toOwnedSlice(alloc);
            return &Stmt{.Block{ .stmts = slice }};
        }
    }

    pub fn parseProgram(self: *Parser, alloc: Allocator) ![]*Stmt {
        var out: std.ArrayList(*Stmt) = .empty;
        defer out.deinit(alloc);
        while (self.lexer.peek().type != lex.TType.EOF) {
            out.append(alloc, self.parseStatement(alloc));
        }
        return try out.toOwnedSlice(alloc);
    }
};

fn printStmt(stmt: *Stmt) void {}
