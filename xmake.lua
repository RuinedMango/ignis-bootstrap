add_rules("mode.debug", "mode.release")

add_requires("libllvm", { configs = { libc = true, lld = true } })

add_rules("plugin.compile_commands.autoupdate", { outputdir = "./" })
target("ignis-bootstrap")
set_kind("binary")
add_files("src/*.c")
add_packages("libllvm")
