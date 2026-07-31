# LLM.md — hanzo-cpp/sdk

Guidance for AI agents working in this repo.

## What this is
The **full Hanzo cloud SDK for C++** — a typed client over the entire `/v1`
surface (AI, agents, inference, compute, data, network, security/IAM/KMS,
platform, observe, web3). Generated from `hanzoai/openapi`'s `hanzo.yaml`.

## Canonical role
- This is the **real code** for the C++ full cloud SDK line: `hanzo-cpp/sdk`.
  `hanzoai/cpp-sdk` is a rename redirect to it (`gh api repos/hanzoai/cpp-sdk
  --jq .full_name` → `hanzo-cpp/sdk`), so the two are one repo, not two.
- Separate line: the AI/agents lib lives at `hanzo-cpp/ai`. Don't merge the two.
- One impl, one place. DRY. Discovery repos link OUT.

## Generation — one place, one way

`hanzoai/openapi` owns the document and the matrix of language projections
(`sdks.yaml` + `generate.py`). Most clients are a bare call site into that
driver. **C++ is not, deliberately**, and `sdks.yaml` records why: this
projection needs generator template **files**, and a file has to sit beside the
invocation or the pair drifts apart. That is the same rule under which `go` and
`rust` left the registry. So `scripts/generate.sh` owns the whole invocation —
same document, same `--check` contract — and there is no `cpp` row upstream.
What is NOT allowed is a row there AND flags here.

```
./scripts/generate.sh            # regenerate include/ and src/
./scripts/generate.sh --check    # diff only; non-zero on drift
```

`hanzoai/openapi` is PRIVATE. raw.githubusercontent.com serves public repos only
and answers **404, not 403**, for a private path, so an anonymous miss is
indistinguishable from a deleted file. The script names that cause and refuses;
it never falls back to a stale spec.

### Two things about the generator that are not free

**1. openapi-generator 7.24.0, not the 7.14.0 `sdks.yaml` pins.** A measured
floor. On 7.14.0 this document leaves **10** translation units uncompilable
(measured at spec `ea45dde`, 2370 TUs — the total moves with the document, the
defects do not) through two defects in the generator's own Java type resolution.
Neither is template-reachable — mustache interpolates the strings that
resolution produces, it cannot recompute a type:

| n | defect | trigger |
|---|---|---|
| 9 | `additionalProperties: {$ref: <array-typed schema>}` emits `std::map<utility::string_t, std::vector>` — the aliased array's element type is dropped | `vector_NamedVectors` → `vector_DenseVector` |
| 1 | an integer-backed enum declares its conversion helpers over `utility::string_t` and defines them over `int32_t` | `framework_Document.docstatus` |

7.24.0 fixes both. The floor is stated once, in `scripts/generate.sh`, where the
invocation is. Above the floor is fine; below it this repo does not build.

**The fleet raises to 7.24.0 too — in ONE coordinated regeneration wave, not
piecemeal, and not yet. This is decided; do not re-litigate it and do not bump
any other language ad hoc.** The reason to wait is sequencing, not doubt: a
large workflow is currently converting cloud's untyped routes into typed ops
with real In/Out structs, so every client will regenerate against a materially
different document when that lands.

How big that is, measured rather than quoted — parse `hanzo.yaml` and count
operations with NO `requestBody` and no schema under any `2xx`
`content.<mediaType>`:

    total operations 2455 · fully untyped 648 · of which cloud_ 621

(The source side of the same work reads differently because it counts
registrations, not projections: cloud's own LLM.md says 986 untyped routes
across 101 packages against 76 typed. Both are real; they measure different
things. A "~445" figure circulates and reproduces from neither — do not carry
it.) Doing the version bump and the
schema growth in one wave means each language's committed output changes ONCE,
reviewably, instead of twice — and a two-step change to 2000+ generated files
per language is a diff nobody reads. C++ goes first only because it cannot
build at all below the floor.

**2. `-DmaxYamlCodePoints`.** swagger-parser hands the document to snakeyaml,
which refuses anything over `3 * 1024 * 1024` = 3145728 code points. `hanzo.yaml`
passed that mark (3,654,449 code points at `1bac13f`) and the failure does not
say so: the parser logs `SnakeException`, silently falls through to the **Swagger
2.0** compat reader, and dies with `Issues with the OpenAPI input` — which reads
like a malformed spec. It is not; the document validates at **0 errors / 0
warnings** (240 "unused model" recommendations). This is fleet-wide, not a C++
problem, and it is also set in `hanzoai/openapi`'s `generate.py` so that every
language gets it from one place.

### `templates/cpp-restsdk/` — four overrides

Forked from **7.24.0** stock, so a generator bump re-derives them rather than
merging them. Each hunk is commented in place; none of them says anything about
the Hanzo API. Deforming the document to suit one C++ generator would degrade
the other language projections that read the same file — **do not do it**.

1. **A oneOf class cannot be a field of another model.** It is generated outside
   the `ModelBase` hierarchy and declares only explicit-`Target` templates, so
   nothing resolves it as a member. The override derives it from `ModelBase` and
   adds the non-template `fromJson`/`fromMultiPart` pair, trying each
   alternative in declaration order.
2. **oneOf visitors call members the alternatives do not have** — `arg.toJson()`
   where `arg` is a `std::shared_ptr<Model>` or a primitive. `ModelBase` already
   overloads every alternative kind; the template just did not use it.
3. **oneOf explicit instantiations are HTML-escaped** — `{{.}}` where the header
   uses the triple form, emitting `fromJson<std::shared_ptr&lt;Object&gt;>`.
4. **A free-form `object` is held BY VALUE.** `Object` is a `ModelBase`
   subclass, but the overload set covers primitives and `std::shared_ptr<T>`
   only, so `toJson` / `fromJson` / `toHttpContent` / `fromHttpContent` on a
   container of `Object` — or on a oneOf whose alternatives are by-value models
   — find no overload. The override adds `is_base_of` overloads for all four,
   plus the `Object.h` include that oneOf headers omit.
5. **An API response container and its element type are resolved separately**,
   so an array of free-form objects declares `std::vector<Object>` and is filled
   with `push_back(std::shared_ptr<Object>)`. The override reads the element
   type off the container itself (`::value_type` / `::mapped_type`).

Plus two in the enum path, both template-shaped and both fixed here:
`isPrimitiveType` and `isEnum` are **not exclusive**, so an integer-backed enum
declared its setter twice and defined it once; and the conversion helpers were
declared over `utility::string_t` in the header while defined over the property's
own `dataType` in the source, with string literals in bodies that must return a
number. The literal form now switches on the property's `isNumeric` — note that
`enumVars`' own `isString` is **false for `utility::string_t`** and is the wrong
discriminator; using it silently emits bare identifiers (`if (value == user)`).

**Mustache comments must not contain `{{` or `}}`.** Such a comment closes at the
inner tag and dumps its remainder into the generated C++. That cost two full
regeneration cycles here; the templates are clean now, keep them so.

## Layout
- `include/hanzo/…`, `src/…` — generated. Never hand-edit; `--check` enforces it.
- `CMakeLists.txt`, `cmake/` — the repo's own. The generator's `CMakeLists.txt`
  and `Config.cmake.in` are dropped before the tree lands: the `hanzo::hanzo`
  target, the export set and `HANZO_BUILD_EXAMPLES` are this repo's contract
  with a consumer, not a generator artifact.
- `examples/` — the six canonical flows.
- `templates/cpp-restsdk/` — the overrides above.

Namespaces are `hanzo::api` and `hanzo::model`; the include root is `hanzo/`.
Every call returns `pplx::task<T>`.

## Examples — the six canonical flows
`hello, chat, money, store, agent, tools`, named and ordered by
`hanzoai/openapi`'s **`flows.yaml`**. Read that file; do not re-derive them — a
sibling lane shipped divergent flows by deriving its own and had to reconcile.
It names the exact operationIds per flow, and a gate upstream asserts every one
of them exists in the merged document.

`HANZO_API_KEY`, `HANZO_BASE_URL` (default `https://api.hanzo.ai`) and
`HANZO_ORG_ID` resolve in ONE place — `examples/hanzo.cpp` — not once per flow.
`store` deletes in a scope guard; `agent` polls the run list to a terminal
state; `tools` reads `error` before `result`, because JSON-RPC reports failure
inside a 200.

One divergence, and the manifest is the side that is wrong: `flows.yaml` says
`hello` should print `data.owner` and `data.name`, but the typed `bot_User` the
route actually returns has `id / handle / displayName / email / role` and no
such fields. **Printing what the model has is correct — do NOT "fix" this
example to match the manifest, and do not invent an `owner` field to satisfy
it.** The spec lane is correcting `flows.yaml` so every language stops working
around it.

## Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j"$(nproc)"
cmake -B build -DHANZO_BUILD_EXAMPLES=ON && cmake --build build -j"$(nproc)"
```
Needs C++17, CMake 3.20+, cpprestsdk + Boost + OpenSSL (`libcpprest-dev` 2.10.19
from Ubuntu universe resolves cleanly, and so do `find_package(cpprestsdk)` and
`find_package(Boost)`).

**KNOWN FUTURE MIGRATION — cpprestsdk is ARCHIVED upstream by Microsoft.** This
is a decision with an expiry, recorded so it is not a surprise later. It ships
because `cpp-restsdk` is the only openapi-generator C++ target that produces a
compiling library from this document — `cpp-qt-client` fails on defects no
template reaches, and the rest are embedded or engine targets (see below). An
archived dependency gets no security fixes, so the trigger to revisit is
whichever comes first: a CVE in cpprestsdk, its removal from Ubuntu universe, or
a C++ generator target that compiles from `hanzo.yaml` without an archived HTTP
stack. When that happens the move is a generator change plus new
`templates/<target>/` overrides — the document, the seam and the six flows do
not move.

Scale is not a problem, and these are measured rather than estimated: generation
takes under two minutes, the 2376 objects plus the six examples build in
**6m14s** at `-j20`, peak RSS across the whole build is **1.6 GB** (the largest
TU is `AdminApi.cpp`), and `libhanzo.a` comes out at 342 MB.

The generators that were measured and rejected, so nobody re-runs the
experiment: `cpp-qt-client` (drags in Qt6 Core + Network **+ Gui**, and 31 of its
failures are models it references but never emits — not template-reachable);
`cpp-tiny` (not a library at all — emits `platformio.ini`, `root.cert`,
`src/main.cpp`; an Arduino/PlatformIO target); `cpp-ue4` (Unreal); `cpp-tizen`
(Samsung Tizen).

## CI
`hanzo.yml` declares the gates, `.github/workflows/cicd.yml` is the seven-line
call into `hanzoai/ci`. **Wired, and UNEXERCISED — do not claim it is green.**
Two distinct failure modes, both observed, neither fixed by anything in this
repo:

- On `hanzo-cpp/sdk`, both runs to date are `completed / startup_failure` with
  **zero jobs created** — GitHub never got as far as scheduling one. It is not a
  missing reusable: `hanzoai/ci` is public, `@v1` resolves to 18bd16c, and its
  `build.yml` is present (57 KB).
- On the `hanzoai/*` repos the same workflow instead sits `queued` forever
  (`hanzoai/java-sdk`'s only run has been queued since 16:30). That one is
  capacity: `hanzoai/ci` defaults to `runs-on: [hanzo-build-linux-amd64]`, an arc
  pool with **zero registered runners** (`gh api orgs/<org>/actions/runners` →
  `total_count: 0` for hanzoai, hanzo-cpp and hanzo-kotlin alike).

Actions itself is enabled and unrestricted (`enabled_repositories: all`,
`allowed_actions: all`), so neither is a permissions problem. Verification here
is a **clean-clone local build**, and that is where every number above comes
from.

C++ has no registry, so the release artifact is a tagged GitHub release carrying
headers + library, built by `binaries:` on every push and published on a tag.

## Brand rules (hard — enforce in all docs/code)
- **Never** call Hanzo an "LLM gateway" or position it against LiteLLM. It is a
  full AI SDK / AI cloud, not a proxy.
- Paths are `/v1/…` only — never an `/api/` prefix.
- **Zen** models are Hanzo's own family — never reference upstream model names.
- Voice: "Hanzo — the Open AI Cloud." Modern, crisp, developer-first.

## Pointer
Full SDK model + one-way rules: `~/work/hanzo/SDK-ARCHITECTURE.md`.
