# WARNING: THIS FILE IS AUTOGENERATED BY update-deps.py DO NOT EDIT

load("@//:build/http.bzl", "http_file")

TAG_NAME = "llvm-18.1.8"
URL = "https://github.com/cloudflare/workerd-tools/releases/download/llvm-18.1.8/llvm-18.1.8-linux-amd64-clang-format"
SHA256 = "34677b7593943121858197358481d6941d9be1977e024c9e21862bddef62c762"

def dep_clang_format_linux_amd64():
    http_file(
        name = "clang-format-linux-amd64",
        url = URL,
        executable = True,
        sha256 = SHA256,
    )