const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lexer = b.addModule("ignis_lexer", .{
        .root_source_file = b.path("src/lexer/root.zig"),
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

    const run_lexer_tests = b.addRunArtifact(lexer_tests);

    const exe_tests = b.addTest(.{
        .root_module = exe.root_module,
    });

    const run_exe_tests = b.addRunArtifact(exe_tests);

    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&run_lexer_tests.step);
    test_step.dependOn(&run_exe_tests.step);
}
