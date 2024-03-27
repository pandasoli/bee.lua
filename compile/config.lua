local lm = require "luamake"
lm:required_version "1.6"

lm:conf {
    c = "c11",
    cxx = "c++17",
    rtti = "off",
    windows = {
        defines = "_WIN32_WINNT=0x0601",
    },
    msvc = {
        flags = "/utf-8",
        ldflags = lm.mode == "debug" and lm.arch == "x86_64" and {
            "/STACK:"..0x160000
        },
    },
    macos = {
        flags = "-Wunguarded-availability",
        sys = "macos10.15",
    },
    linux = {
        crt = "static",
        flags = "-fPIC",
        ldflags = {
            "-Wl,-E",
            "-static-libgcc",
        },
    },
    netbsd = {
        crt = "static",
        ldflags = "-Wl,-E",
    },
    freebsd = {
        crt = "static",
        ldflags = "-Wl,-E",
    },
    openbsd = {
        crt = "static",
        ldflags = "-Wl,-E",
    },
    android = {
        ldflags = "-Wl,-E",
    },
}

if lm.sanitize then
    lm:conf {
        mode = "debug",
        flags = "-fsanitize=address",
        gcc = {
            ldflags = "-fsanitize=address"
        },
        clang = {
            ldflags = "-fsanitize=address"
        }
    }
    lm:msvc_copydll "sanitize-dll" {
        type = "asan",
        output = "$bin"
    }
end
