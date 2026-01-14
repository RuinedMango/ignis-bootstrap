const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lexer = b.addModule("ignis_lexer", .{
        .root_source_file = b.path("src/lexer/root.zig"),
        .target = target,
    });
    const parser = b.addModule("ignis_parser", .{
        .root_source_file = b.path("src/parser/root.zig"),
        .target = target,
    });

    const exe = b.addExecutable(.{
        .name = "ignis_bootstrap",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
            .imports = &.{
                .{ .name = "ignis_lexer", .module = lexer },
                .{ .name = "ignis_parser", .module = parser },
            },
        }),
    });

    b.installArtifact(exe);

    const run_step = b.step("run", "Run the app");

    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);

    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const lexer_tests = b.addTest(.{
        .root_module = lexer,
    });
    const parser_tests = b.addTest(.{
        .root_module = parser,
    });

    const run_lexer_tests = b.addRunArtifact(lexer_tests);
    const run_parser_tests = b.addRunArtifact(parser_tests);

    const exe_tests = b.addTest(.{
        .root_module = exe.root_module,
    });

    const run_exe_tests = b.addRunArtifact(exe_tests);

    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&run_lexer_tests.step);
    test_step.dependOn(&run_parser_tests.step);
    test_step.dependOn(&run_exe_tests.step);
}
