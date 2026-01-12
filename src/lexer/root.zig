const std = @import("std");
const Io = std.Io;
const Allocator = std.mem.Allocator;

pub const Lexer = struct {
    src: []u8,
    pos: u64,
    line: u32,
    col: u32,

    tkns: std.ArrayList(Token),

    pub fn init() !Lexer {
        return Lexer{ .src = "", .pos = 0, .line = 1, .col = 1, .tkns = std.ArrayList(Token).empty };
    }
    pub fn lex(self: *Lexer, io: Io, alloc: Allocator, file: Io.File) !void {
        try lexFile(self, io, alloc, file);
    }
    pub fn printTokens(self: *Lexer, alloc: Allocator) !void {
        for (try self.*.tkns.toOwnedSlice(alloc)) |tkn| {
            var token = tkn;
            printToken(&token);
        }
    }
    pub fn deinit(self: *Lexer, alloc: Allocator) void {
        self.tkns.deinit(alloc);
        alloc.free(self.src);
    }

    pub fn addToken(self: *Lexer, ttype: TType, data: TData, start: u64, len: u32, alloc: Allocator) void {
        const token: Token = .{ .type = ttype, .data = data, .col = self.col, .line = self.line, .start = start, .len = len };
        self.*.tkns.append(alloc, token) catch std.log.info("Unable to append", .{});
    }

    pub fn cur(self: *Lexer) u8 {
        return self.src[self.pos];
    }
    pub fn peek(self: *Lexer) u8 {
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

const Token = struct {
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

const TType = enum {
    COMMENT,
    IDENT,
    KEYWORD,
    INT,
    STRING,
    ASSIGN,
    PLUS,
    MINUS,
    MUL,
    DIV,
    OCTO,
    BANG,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACK,
    RBRACK,
    COMMA,
    COLON,
    SEMI,
    EOF,
    UNKNOWN,
};

fn printToken(tkn: *Token) void {
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

fn lexFile(lexer: *Lexer, io: Io, alloc: Allocator, file: Io.File) !void {
    const stat = try file.stat(io);
    lexer.src = try alloc.alloc(u8, stat.size);
    _ = try file.readPositionalAll(io, lexer.src, 0);

    var token_start: u64 = 0;
    var number_acc: i64 = 0;

    var state = LState.START;

    while (!lexer.isEof()) {
        const c: u8 = lexer.cur();

        switch (state) {
            LState.START => blk: {
                if (std.ascii.isWhitespace(c)) {
                    lexer.advance();
                    break :blk;
                }
                if (std.ascii.isAlphabetic(c) or c == '_') {
                    token_start = lexer.pos;
                    state = LState.IDENT;
                    lexer.advance();
                } else if (std.ascii.isDigit(c)) {
                    token_start = lexer.pos;
                    number_acc = (c - '0');
                    state = LState.NUMBER;
                    lexer.advance();
                } else if (c == '"') {
                    token_start = lexer.pos;
                    state = LState.STRING;
                    lexer.advance();
                } else if (c == '/' and lexer.peek() == '/') {
                    token_start = lexer.pos;
                    state = LState.LINE_COMMENT;
                    lexer.advance();
                    lexer.advance();
                } else {
                    token_start = lexer.pos;
                    switch (c) {
                        '+' => {
                            lexer.addToken(TType.PLUS, TData{ .chr = '+' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        '-' => {
                            lexer.addToken(TType.MINUS, TData{ .chr = '-' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        '*' => {
                            lexer.addToken(TType.MUL, TData{ .chr = '*' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        '/' => {
                            lexer.addToken(TType.DIV, TData{ .chr = '/' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        '#' => {
                            lexer.addToken(TType.OCTO, TData{ .chr = '#' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        '!' => {
                            lexer.addToken(TType.BANG, TData{ .chr = '!' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        '(' => {
                            lexer.addToken(TType.LPAREN, TData{ .chr = '(' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        ')' => {
                            lexer.addToken(TType.RPAREN, TData{ .chr = ')' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        '{' => {
                            lexer.addToken(TType.LBRACE, TData{ .chr = '{' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        '}' => {
                            lexer.addToken(TType.RBRACE, TData{ .chr = '}' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        '[' => {
                            lexer.addToken(TType.LBRACK, TData{ .chr = '[' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        ']' => {
                            lexer.addToken(TType.RBRACK, TData{ .chr = ']' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        ',' => {
                            lexer.addToken(TType.COMMA, TData{ .chr = ',' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        ':' => {
                            lexer.addToken(TType.COLON, TData{ .chr = ':' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        ';' => {
                            lexer.addToken(TType.SEMI, TData{ .chr = ';' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        '=' => {
                            lexer.addToken(TType.ASSIGN, TData{ .chr = '=' }, token_start, 1, alloc);
                            lexer.advance();
                        },
                        else => {
                            lexer.addToken(TType.UNKNOWN, undefined, token_start, 1, alloc);
                            lexer.advance();
                        }
                    }
                }
            },
            LState.IDENT => {
                if (std.ascii.isAlphanumeric(c) or c == '_') {
                    lexer.advance();
                } else {
                    const len: u32 = @intCast(lexer.pos - token_start);
                    lexer.addToken(TType.IDENT, TData{ .str = lexer.src[token_start..lexer.pos] }, token_start, len, alloc);
                    state = LState.START;
                }
            },
            LState.NUMBER => {
                if (std.ascii.isDigit(c)) {
                    number_acc = number_acc * 10 + (c - '0');
                    lexer.advance();
                } else {
                    const len: u32 = @intCast(lexer.pos - token_start);
                    lexer.addToken(TType.INT, TData{ .int = number_acc }, token_start, len, alloc);
                    state = LState.START;
                }
            },
            LState.STRING => {
                if (c == '\\') {
                    state = LState.STRING_ESC;
                    lexer.advance();
                } else if (c == '"') {
                    lexer.advance();
                    const len: u32 = @intCast(lexer.pos - token_start);
                    lexer.addToken(TType.STRING, TData{ .str = lexer.src[token_start..lexer.pos] }, token_start, len, alloc);
                    state = LState.START;
                } else if (lexer.isEof()) {
                    std.log.err("Unterminated string at {d}:{d}\n", .{ lexer.line, lexer.col });
                    return;
                } else {
                    lexer.advance();
                }
            },
            LState.STRING_ESC => {
                if (lexer.isEof()) {
                    std.log.err("Unterminated string escape at {d}:{d}\n", .{ lexer.line, lexer.col });
                    return;
                }
                lexer.advance();
                state = LState.STRING;
            },
            LState.LINE_COMMENT => {
                if (c == '\n' or lexer.isEof()) {
                    const len: u32 = @intCast(lexer.pos - token_start);
                    lexer.addToken(TType.COMMENT, TData{ .str = lexer.src[token_start..lexer.pos] }, token_start, len, alloc);
                    state = LState.START;
                    lexer.advance();
                } else {
                    lexer.advance();
                }
            },
            LState.BLOCK_COMMENT => {
                if (c == '\n' or lexer.isEof()) {
                    const len: u32 = @intCast(lexer.pos - token_start);
                    lexer.addToken(TType.COMMENT, TData{ .str = lexer.src[token_start..lexer.pos] }, token_start, len, alloc);
                    state = LState.START;
                } else {
                    lexer.advance();
                }
            },
        }
    }

    if (state == LState.IDENT or state == LState.NUMBER or state == LState.LINE_COMMENT) {
        const len: u32 = @intCast(lexer.pos - token_start);
        if (state == LState.IDENT) {
            lexer.addToken(TType.IDENT, TData{ .str = lexer.src[token_start..lexer.pos] }, token_start, len, alloc);
        } else if (state == LState.NUMBER) {
            lexer.addToken(TType.INT, TData{ .int = number_acc }, token_start, len, alloc);
        } else if (state == LState.LINE_COMMENT) {
            lexer.addToken(TType.COMMENT, TData{ .str = lexer.src[token_start..lexer.pos] }, token_start, len, alloc);
        }
    } else if (state == LState.STRING or state == LState.STRING_ESC) {
        std.log.err("Unterminated string at EOF (line {d})\n", .{lexer.line});
    }

    lexer.addToken(TType.EOF, undefined, token_start, 1, alloc);
}
