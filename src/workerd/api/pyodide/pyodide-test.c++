// Copyright (c) 2017-2022 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
//     https://opensource.org/licenses/Apache-2.0

#include "pyodide.h"

#include <kj/test.h>

namespace workerd::api {
namespace {

KJ_TEST("basic `import` tests") {
  auto files = kj::heapArrayBuilder<kj::String>(2);
  files.add(kj::str("import a\nimport z"));
  files.add(kj::str("import b"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 3);
  KJ_REQUIRE(result[0] == "a");
  KJ_REQUIRE(result[1] == "z");
  KJ_REQUIRE(result[2] == "b");
}

KJ_TEST("supports whitespace") {
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str("import      a\nimport    \n\tz"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 2);
  KJ_REQUIRE(result[0] == "a");
  KJ_REQUIRE(result[1] == "z");
}

KJ_TEST("supports windows newlines") {
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str("import      a\r\nimport    \r\n\tz"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 2);
  KJ_REQUIRE(result[0] == "a");
  KJ_REQUIRE(result[1] == "z");
}

KJ_TEST("basic `from` test") {
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str("from x import a,b\nfrom z import y"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 2);
  KJ_REQUIRE(result[0] == "x");
  KJ_REQUIRE(result[1] == "z");
}

KJ_TEST("ignores indented blocks") {
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str("import a\nif True:\n  import x\nimport y"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 2);
  KJ_REQUIRE(result[0] == "a");
  KJ_REQUIRE(result[1] == "y");
}

KJ_TEST("supports nested imports") {
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str("import a.b\nimport z.x.y.i"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 2);
  KJ_REQUIRE(result[0] == "a.b");
  KJ_REQUIRE(result[1] == "z.x.y.i");
}

KJ_TEST("nested `from` test") {
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str("from x.y.z import a,b\nfrom z import y"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 2);
  KJ_REQUIRE(result[0] == "x.y.z");
  KJ_REQUIRE(result[1] == "z");
}

KJ_TEST("ignores trailing period") {
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str("import a.b.\nimport z.x.y.i."));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 0);
}

KJ_TEST("ignores relative import") {
  // This is where we diverge from the old AST-based approach. It would have returned `y` in the
  // input below.
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str("import .a.b\nimport ..z.x\nfrom .y import x"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 0);
}

KJ_TEST("supports commas") {
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str("import a,b"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 2);
  KJ_REQUIRE(result[0] == "a");
  KJ_REQUIRE(result[1] == "b");
}

KJ_TEST("supports backslash") {
  auto files = kj::heapArrayBuilder<kj::String>(4);
  files.add(kj::str("import a\\\n,b"));
  files.add(kj::str("import\\\n q,w"));
  files.add(kj::str("from \\\nx import y"));
  files.add(kj::str("from \\\n   c import y"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 6);
  KJ_REQUIRE(result[0] == "a");
  KJ_REQUIRE(result[1] == "b");
  KJ_REQUIRE(result[2] == "q");
  KJ_REQUIRE(result[3] == "w");
  KJ_REQUIRE(result[4] == "x");
  KJ_REQUIRE(result[5] == "c");
}

KJ_TEST("multiline-strings ignored") {
  auto files = kj::heapArrayBuilder<kj::String>(4);
  files.add(kj::str(R"SCRIPT(
FOO="""
import x
from y import z
"""
)SCRIPT"));
  files.add(kj::str(R"SCRIPT(
FOO='''
import f
from g import z
'''
)SCRIPT"));
  files.add(kj::str(R"SCRIPT(FOO = "\
import b \
")SCRIPT"));
  files.add(kj::str("FOO=\"\"\"  \n", R"SCRIPT(import x
from y import z
""")SCRIPT"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 0);
}

KJ_TEST("multiline-strings with imports in-between") {
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str(
      R"SCRIPT(FOO="""
import x
from y import z
"""import q
import w
BAR="""
import e
"""
from t import u)SCRIPT"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 2);
  KJ_REQUIRE(result[0] == "w");
  KJ_REQUIRE(result[1] == "t");
}

KJ_TEST("import after string literal") {
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str(R"SCRIPT(import a
"import b)SCRIPT"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 1);
  KJ_REQUIRE(result[0] == "a");
}

KJ_TEST("import after `i`") {
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str(R"SCRIPT(import a
iimport b)SCRIPT"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 1);
  KJ_REQUIRE(result[0] == "a");
}

KJ_TEST("langchain import") {
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str(R"SCRIPT(from js import Response, console, URL
from langchain.chat_models import ChatOpenAI
import openai)SCRIPT"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 3);
  KJ_REQUIRE(result[0] == "js");
  KJ_REQUIRE(result[1] == "langchain.chat_models");
  KJ_REQUIRE(result[2] == "openai");
}

KJ_TEST("quote in multiline string") {
  auto files = kj::heapArrayBuilder<kj::String>(1);
  files.add(kj::str(R"SCRIPT(temp = """
w["h
""")SCRIPT"));
  auto result = pyodide::ArtifactBundler::parsePythonScriptImports(files.finish());
  KJ_REQUIRE(result.size() == 0);
}

}  // namespace
}  // namespace workerd::api
