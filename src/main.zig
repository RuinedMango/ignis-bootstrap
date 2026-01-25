const std = @import("std");
const Io = std.Io;

const lex = @import("ignis_lexer");
const par = @import("ignis_parser");

pub fn main(init: std.process.Init) !void {
    const arena: std.mem.Allocator = init.arena.allocator();
    const io = init.io;

    const args = try init.minimal.args.toSlice(arena);
    for (args) |arg| {
        std.log.info("arg: {s}", .{arg});
    }

    var lexer: lex.Lexer = try lex.Lexer.init();
    defer lexer.deinit(arena);

    const file = try std.Io.Dir.cwd().openFile(io, args[1], .{ .mode = .read_write });

    try lexer.loadFile(io, arena, file);

    var parser = par.Parser.init(&lexer);

    const ast = try parser.parseProgram(arena);
    for (ast) |node| {
        par.printStmt(node);
    }
}
