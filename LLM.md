# LLM.md — hanzo-cpp/sdk

Guidance for AI agents working in this repo.

## What this is
The **full Hanzo cloud SDK for C++** — a typed client over the entire `/v1`
surface (AI, agents, inference, compute, data, network, security/IAM/KMS,
platform, observe, web3). Generated from the cloud OpenAPI spec.

## Status — there is no client here yet, and why

**No openapi-generator 7.14.0 C++ generator emits a tree that compiles from
`hanzo.yaml`.** That is a measured result, not an impression, and it is the
reason this repo carries the seam (`scripts/generate.sh`), the CI wiring and the
template overrides but no `include/` or `src/`. A tree that does not compile is
worse than no tree, so none was committed.

Measured on aarch64 Ubuntu 24.04, g++ 13.3, cmake 3.28.3, generator 7.14.0,
spec `hanzoai/openapi@0e51842` (1737 paths · 2478 operations · 1798 schemas ·
263 tags). Method: generate, then `g++ -std=c++17 -fsyntax-only` every emitted
translation unit.

| generator | verdict |
|---|---|
| `cpp-restsdk` | 2316 TUs, **52 fail** as generated → **11 fail** with the overrides in `templates/` |
| `cpp-qt-client` | 2311 TUs, **51 fail**; also drags in Qt6 Core + Network **+ Gui** (`OAIOauth.h` includes `QDesktopServices`). 31 of the 51 are models it references but never emits, 14 are self-recursive schemas held by value — neither reachable from a template |
| `cpp-tiny` | not a library — emits `platformio.ini`, `root.cert`, `src/main.cpp`. PlatformIO/Arduino target |
| `cpp-ue4` | Unreal Engine target, not a normal toolchain |
| `cpp-tizen` | Samsung Tizen platform target |

**Scale is not the blocker.** The library is one static target of 2316
translation units and a full Release build compiles ~2300 objects in **under 11
minutes** at `-j20` on this box (peak RSS 1.6 GB on the largest TU,
`AdminApi.cpp` at 586 KB). Generation itself takes 50 seconds.

**The dependency is not the blocker either.** `cpp-restsdk` needs cpprestsdk +
Boost + OpenSSL; `libcpprest-dev` 2.10.19 installs from Ubuntu universe and both
`find_package(cpprestsdk)` and `find_package(Boost)` resolve, with cmake
configuring clean. Worth noting anyway that cpprestsdk is archived upstream by
Microsoft — a second reason not to commit to it before the codegen is sound.

The blocker is codegen correctness, and nothing else.

### What `templates/cpp-restsdk/` fixes (proven: 52 → 11)

Four defect classes live in the generator's mustache templates, so a
`-t` override reaches them. Each hunk is commented in place.

1. **oneOf explicit instantiations are HTML-escaped.** `model-source.mustache`
   interpolates the alternative with `{{.}}` where the header uses `{{{.}}}`,
   emitting `fromJson<std::shared_ptr&lt;Object&gt;>`. Never valid C++.
2. **oneOf visitors call members the alternatives do not have.**
   `val = arg.toJson()` where `arg` is `std::shared_ptr<Model>` or
   `utility::string_t`. `ModelBase` already overloads `toJson`/`fromJson` for
   every alternative kind; the template just does not use it.
3. **A oneOf model cannot be a field of another model.** The generated class
   stands outside the `ModelBase` hierarchy and declares only an
   explicit-`Target` template, so nothing resolves it when it appears as a
   member. The override derives it from `ModelBase` and gives it the
   non-template `fromJson`/`fromMultiPart` pair, trying each alternative in
   declaration order.
4. **`Object` by value has no `ModelBase` overload.** A free-form `object`
   schema maps to `Object`, held by value in containers, and the overload set
   covers primitives and `std::shared_ptr<T>` but not its own subclasses. The
   override adds the four `is_base_of` overloads, plus the `Object.h` include
   that oneOf headers omit.
5. **An API response container and its element type are resolved separately**,
   so an array of free-form objects declares `std::vector<Object>` and is filled
   with `push_back(std::shared_ptr<Object>)`. The override reads the element
   type off the container itself (`::value_type` / `::mapped_type`) and routes
   both directions through `ModelBase`, which is correct for every item kind.

### What remains, and why a template cannot reach it

The last 11 TUs fail inside the generator's **Java type resolution**. Mustache
only interpolates the strings that resolution produces (`{{{dataType}}}`, the
`oneOf` list, `{{#imports}}`); it cannot recompute a type or filter an import
list. These need a fix in `CppRestSdkClientCodegen`, upstream.

| n | defect | trigger in `hanzo.yaml` |
|---|---|---|
| 9 | `additionalProperties: {$ref: <array-typed schema>}` emits `std::map<utility::string_t, std::vector>` — the aliased array's element type is dropped | `vector_NamedVectors` → `vector_DenseVector` |
| 1 | `FunctionsApi.h` includes `hanzo/model/Edge_deployFunction_request.h`, a model the resolver names but never emits | `edge_deployFunction` — both `application/octet-stream` and an inline `multipart/form-data` object body |
| 1 | an integer-backed enum: the conversion helpers are declared over `utility::string_t` and their bodies compare against string literals, while the source calls them with `int32_t` | `framework_Document` |

The enum one is template-*shaped* — the signature is hardcoded in
`model-header.mustache`. It is deliberately **not** overridden here: aligning
the signature alone only moves the error, because the conversion bodies compare
against string literals throughout. Fixing it properly means rewriting enum
codegen for non-string base types, which belongs upstream and not in a fork.

**The spec is not at fault.** `hanzo.yaml` validates at 0 errors / 0 warnings
under 7.14.0 (346 recommendations, all "unused model"), so
`--skip-validate-spec` is not needed and is not used. Every trigger above is
well-formed OpenAPI. Deforming the document to suit one C++ generator would
degrade the seven other language projections that read the same file — do not
do it.

### The path to a client, in order

1. Fix the four resolver defects upstream in openapi-generator, or carry a
   patched generator build.
2. Add the `cpp` row to `hanzoai/openapi`'s `sdks.yaml` — generator, properties,
   and a `take` mapping `include`/`src` into this repo. **There is no `cpp` row
   today**, deliberately: a row describes a projection that compiles, and this
   one does not yet. `scripts/generate.sh` will exit with
   `invalid choice: cpp` until it exists.
3. `./scripts/generate.sh` then populates `include/` and `src/`, and
   `hanzo.yml`'s `cmake-build` / `examples-build` / `binaries` steps become live.
4. The six canonical example flows come from `hanzoai/openapi`'s `flows.yaml` —
   hello, chat, money, store, agent, tools. Read that file; do not re-derive
   them. `HANZO_API_KEY`, `HANZO_BASE_URL` and `HANZO_ORG_ID` resolve in ONE
   shared place in this repo, not once per flow.

## Canonical role
- This is the **real code** for the C++ full cloud SDK line: `hanzo-cpp/sdk`.
  The `hanzoai/cpp-sdk` wrapper is docs/landing and links here — never duplicate impl.
- Separate line: the AI/agents lib lives at `hanzo-cpp/ai`. Don't merge the two.
- Completeness order across languages: Python → Rust → C++ → Go → others.
- One impl, one place. DRY. Discovery repos link OUT.

## Install / run
- Consume via CMake: `find_package(hanzo CONFIG REQUIRED)` → link `hanzo::hanzo`.
- Build: `cmake -B build && cmake --build build && ctest --test-dir build`.
- Requires C++17, CMake 3.20+.

## Generation — one place, one way
`hanzoai/openapi` owns both the document (`hanzo.yaml`) and its projection into
every language: the matrix is data in `sdks.yaml`, the driver is `generate.py`.
`scripts/generate.sh` here is a **call site** into that driver and nothing more —
never a bespoke generator invocation. `./scripts/generate.sh --check` diffs
instead of writing and exits non-zero on drift, which is what makes "the client
cannot be hand-edited" a fact rather than a convention.

## CI
`hanzo.yml` declares the gates, `.github/workflows/cicd.yml` is the seven-line
call into `hanzoai/ci`. **Wired and registered, but unexercised.** Actions is
enabled and unrestricted on the org; what is missing is a runner — `hanzoai/ci`
defaults to `runs-on: [hanzo-build-linux-amd64]`, an arc pool with zero
registered runners, so a run queues instead of executing. Two of the four gates
additionally have nothing to build until the client lands; they are written now
so that landing it is one commit.
C++ has no registry, so the release artifact is a tagged GitHub release carrying
headers + library, built by `binaries:` on every push and published on a tag.

## Brand rules (hard — enforce in all docs/code)
- **Never** call Hanzo an "LLM gateway" or position it against LiteLLM. It is a
  full AI SDK / AI cloud, not a proxy.
- Paths are `/v1/…` only — never an `/api/` prefix.
- **Zen** models are Hanzo's own family — never reference upstream model names.
- Voice: "Hanzo — the Open AI Cloud." Modern, crisp, developer-first.

## Key entry points
- `scripts/generate.sh` — the call site into `hanzoai/openapi`'s `generate.py`.
- `templates/cpp-restsdk/` — the four proven generator template overrides.
- `CMakeLists.txt` — the `hanzo::hanzo` target and install/export config. Lands
  with the client; the repo root owns it, never the generator.
- Service methods are code-generated from the OpenAPI spec; regenerate, don't hand-fork.

## Pointer
Full SDK model + one-way rules: `~/work/hanzo/SDK-ARCHITECTURE.md`.
