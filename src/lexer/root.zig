const std = @import("std");
const Io = std.Io;
const Allocator = std.mem.Allocator;

pub const Lexer = struct {
    src: []u8,
    pos: u64,
    line: u32,
    col: u32,

    peeked: ?Token,

    // Load and Unload
    pub fn init() !Lexer {
        return Lexer{ .src = "", .pos = 0, .line = 1, .col = 1, .peeked = null };
    }
    pub fn loadFile(self: *Lexer, io: Io, alloc: Allocator, file: Io.File) !void {
        const stat = try file.stat(io);
        self.src = try alloc.alloc(u8, stat.size);
        _ = try file.readPositionalAll(io, self.src, 0);
    }
    pub fn deinit(self: *Lexer, alloc: Allocator) void {
        alloc.free(self.src);
    }

    // Self src adjacent
    pub fn cur(self: *Lexer) u8 {
        return self.src[self.pos];
    }
    pub fn byteNPeek(self: *Lexer, n: u16) u8 {
        return self.src[self.pos + n];
    }
    pub fn bytePeek(self: *Lexer) u8 {
        return self.src[self.pos + 1];
    }
    pub fn advance(self: *Lexer) void {
        if (self.cur() == '\n') {
            self.line += 1;
            self.col = 0;
        } else {
            self.col += 1;
        }
        self.pos += 1;
    }
    pub fn isEof(self: *Lexer) bool {
        return self.pos >= self.src.len;
    }

    // Token manipulation
    pub fn printToken(self: *Lexer, tkn: Token) void {
        _ = self;
        std.log.info("{s} (line: {d} col: {d}):", .{ @tagName(tkn.type), tkn.line, tkn.col });
        switch (tkn.data) {
            .chr => {
                std.log.info("  [chr {c}]", .{tkn.data.chr});
            },
            .flt => {
                std.log.info("  [flt {d}]", .{tkn.data.flt});
            },
            .int => {
                std.log.info("  [int {d}]", .{tkn.data.int});
            },
            .str => {
                std.log.info("  [str {s}]", .{tkn.data.str});
            },
        }
    }
    pub fn makeToken(self: *Lexer, ttype: TType, data: TData, start: u64, len: u32) Token {
        return .{ .type = ttype, .data = data, .col = self.col, .line = self.line, .start = start, .len = len };
    }

    // Core lex functions
    pub fn next(self: *Lexer) Token {
        if (self.peeked) {
            const t = self.peeked.?;
            self.peeked = null;
            return t;
        }

        return self.lexOne();
    }
    pub fn peek(self: *Lexer) Token {
        if (self.peeked) return self.peeked.?;
        const t = self.lexOne();
        self.peeked = t;
        return t;
    }
    pub fn unget(self: *Lexer, t: Token) void {
        self.peeked = t;
    }
    pub fn lexOne(self: *Lexer) Token {
        if (self.isEof()) {
            return self.makeToken(TType.EOF, TData{ .str = @constCast("EOF") }, self.pos, 1);
        }
        var token_start: u64 = 0;
        var int_acc: i64 = 0;
        var frac_acc: f64 = 0;
        var frac_div: f64 = 0;
        var float_mode: bool = false;

        var state = LState.START;

        while (true) {
            const c: u8 = self.cur();
            switch (state) {
                LState.START => blk: {
                    if (std.ascii.isWhitespace(c)) {
                        self.advance();
                        break :blk;
                    }
                    if (std.ascii.isAlphabetic(c) or c == '_') {
                        token_start = self.pos;
                        state = LState.IDENT;
                        self.advance();
                    } else if (std.ascii.isDigit(c)) {
                        token_start = self.pos;
                        int_acc = (c - '0');
                        frac_acc = @floatFromInt(c - '0');
                        state = LState.NUMBER;
                        self.advance();
                    } else if (c == '"') {
                        token_start = self.pos;
                        state = LState.STRING;
                        self.advance();
                    } else if (c == '/' and self.bytePeek() == '/') {
                        token_start = self.pos;
                        state = LState.LINE_COMMENT;
                        self.advance();
                        self.advance();
                    } else if (c == '-' and self.bytePeek() == '>') {
                        token_start = self.pos;
                        const tok = self.makeToken(TType.ARROW, TData{ .str = @constCast("->") }, token_start, 2);
                        self.advance();
                        self.advance();
                        return tok;
                    } else {
                        token_start = self.pos;
                        switch (c) {
                            '+' => {
                                const tok = self.makeToken(TType.PLUS, TData{ .chr = '+' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '-' => {
                                const tok = self.makeToken(TType.MINUS, TData{ .chr = '-' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '*' => {
                                const tok = self.makeToken(TType.MUL, TData{ .chr = '*' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '/' => {
                                const tok = self.makeToken(TType.DIV, TData{ .chr = '/' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '%' => {
                                const tok = self.makeToken(TType.MOD, TData{ .chr = '%' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '#' => {
                                const tok = self.makeToken(TType.OCTO, TData{ .chr = '#' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '!' => {
                                const tok = self.makeToken(TType.BANG, TData{ .chr = '!' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '.' => {
                                const tok = self.makeToken(TType.DOT, TData{ .chr = '.' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '>' => {
                                const tok = self.makeToken(TType.GRTHAN, TData{ .chr = '>' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '<' => {
                                const tok = self.makeToken(TType.LSTHAN, TData{ .chr = '<' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '(' => {
                                const tok = self.makeToken(TType.LPAREN, TData{ .chr = '(' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            ')' => {
                                const tok = self.makeToken(TType.RPAREN, TData{ .chr = ')' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '{' => {
                                const tok = self.makeToken(TType.LBRACE, TData{ .chr = '{' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '}' => {
                                const tok = self.makeToken(TType.RBRACE, TData{ .chr = '}' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '[' => {
                                const tok = self.makeToken(TType.LBRACK, TData{ .chr = '[' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            ']' => {
                                const tok = self.makeToken(TType.RBRACK, TData{ .chr = ']' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            ',' => {
                                const tok = self.makeToken(TType.COMMA, TData{ .chr = ',' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            ':' => {
                                const tok = self.makeToken(TType.COLON, TData{ .chr = ':' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            ';' => {
                                const tok = self.makeToken(TType.SEMI, TData{ .chr = ';' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            '=' => {
                                const tok = self.makeToken(TType.ASSIGN, TData{ .chr = '=' }, token_start, 1);
                                self.advance();
                                return tok;
                            },
                            else => {
                                const tok = self.makeToken(TType.UNKNOWN, TData{ .str = @constCast("Unknown") }, token_start, 1);
                                self.advance();
                                return tok;
                            }
                        }
                    }
                },
                LState.IDENT => {
                    if (std.ascii.isAlphanumeric(c) or c == '_') {
                        self.advance();
                    } else {
                        const len: u32 = @intCast(self.pos - token_start);
                        const tok = makeKeyword(self, self.src[token_start..self.pos], token_start, len);
                        state = LState.START;
                        return tok;
                    }
                },
                LState.NUMBER => {
                    if (std.ascii.isDigit(c)) {
                        if (float_mode) {
                            frac_acc += @as(f64, @floatFromInt(c - '0')) / frac_div;
                            frac_div *= 10.0;
                        } else {
                            int_acc = int_acc * 10 + (c - '0');
                        }
                        self.advance();
                    } else if (c == '.') {
                        const nextByte = self.byteNPeek(1);
                        if (std.ascii.isDigit(nextByte)) {
                            float_mode = true;
                            frac_acc = 0.0;
                            frac_div = 10.0;
                            self.advance();
                        } else {
                            const len: u32 = @intCast(self.pos - token_start);
                            const tok = self.makeToken(TType.INT, TData{ .int = int_acc }, token_start, len);
                            state = LState.START;
                            float_mode = false;
                            int_acc = 0;
                            return tok;
                        }
                    } else {
                        const len: u32 = @intCast(self.pos - token_start);
                        if (float_mode) {
                            const value: f64 = @as(f64, @floatFromInt(int_acc)) + frac_acc;
                            const tok = self.makeToken(TType.FLOAT, TData{ .flt = value }, token_start, len);
                            return tok;
                        } else {
                            const tok = self.makeToken(TType.INT, TData{ .int = int_acc }, token_start, len);
                            return tok;
                        }
                        state = LState.START;
                        float_mode = false;
                        int_acc = 0;
                    }
                },

                LState.STRING => {
                    if (c == '\\') {
                        state = LState.STRING_ESC;
                        self.advance();
                    } else if (c == '"') {
                        self.advance();
                        const len: u32 = @intCast(self.pos - token_start);
                        return self.makeToken(TType.STRING, TData{ .str = self.src[token_start..self.pos] }, token_start, len);
                    } else if (self.isEof()) {
                        std.log.err("Unterminated string at {d}:{d}\n", .{ self.line, self.col });
                        return undefined;
                    } else {
                        self.advance();
                    }
                },
                LState.STRING_ESC => {
                    if (self.isEof()) {
                        std.log.err("Unterminated string escape at {d}:{d}\n", .{ self.line, self.col });
                        return undefined;
                    }
                    self.advance();
                    state = LState.STRING;
                },
                LState.LINE_COMMENT => {
                    if (c == '\n' or self.isEof()) {
                        const len: u32 = @intCast(self.pos - token_start);
                        const tok = self.makeToken(TType.COMMENT, TData{ .str = self.src[token_start..self.pos] }, token_start, len);
                        state = LState.START;
                        self.advance();
                        return tok;
                    } else {
                        self.advance();
                    }
                },
                LState.BLOCK_COMMENT => {
                    if (c == '\n' or self.isEof()) {
                        const len: u32 = @intCast(self.pos - token_start);
                        return self.makeToken(TType.COMMENT, TData{ .str = self.src[token_start..self.pos] }, token_start, len);
                    } else {
                        self.advance();
                    }
                },
            }
        }
    }
};

const LState = enum {
    START,
    IDENT,
    NUMBER,
    STRING,
    STRING_ESC,
    LINE_COMMENT,
    BLOCK_COMMENT,
};

pub const Token = struct {
    type: TType,
    data: TData,
    col: u32,
    line: u32,
    start: u64,
    len: u32,
};

const TData = union(enum) {
    str: []u8,
    flt: f64,
    int: i64,
    chr: u8,
};

pub const TType = enum { COMMENT, IDENT, KEYWORD, INT, FLOAT, STRING, ASSIGN, PLUS, MINUS, MUL, DIV, MOD, OCTO, BANG, DOT, ARROW, GRTHAN, LSTHAN, LPAREN, RPAREN, LBRACE, RBRACE, LBRACK, RBRACK, COMMA, COLON, SEMI, EOF, UNKNOWN, FN, WHILE, FOR, IF, STRUCT, RETURN, NULL, VOID };

fn makeKeyword(lexer: *Lexer, ident: []u8, token_start: u64, len: u32) Token {
    var ttype: TType = TType.IDENT;
    if (std.mem.eql(u8, ident, "fn")) {
        ttype = TType.FN;
    } else if (std.mem.eql(u8, ident, "for")) {
        ttype = TType.FOR;
    } else if (std.mem.eql(u8, ident, "while")) {
        ttype = TType.WHILE;
    } else if (std.mem.eql(u8, ident, "if")) {
        ttype = TType.IF;
    } else if (std.mem.eql(u8, ident, "struct")) {
        ttype = TType.STRUCT;
    } else if (std.mem.eql(u8, ident, "return")) {
        ttype = TType.RETURN;
    } else if (std.mem.eql(u8, ident, "null")) {
        ttype = TType.NULL;
    } else if (std.mem.eql(u8, ident, "void")) {
        ttype = TType.VOID;
    }
    return lexer.makeToken(ttype, TData{ .str = ident }, token_start, len);
}
