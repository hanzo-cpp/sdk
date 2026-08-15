# LLM.md — hanzo-cpp/sdk

Guidance for AI agents working in this repo.

## What this is
The **full Hanzo cloud SDK for C++** — a typed client over the entire `/v1`
surface (AI, agents, inference, compute, data, network, security/IAM/KMS,
platform, observe, web3). Generated from the `openapi.yaml` that **`hanzoai/cloud`
emits**, at the commit `.spec-lock` names — the same document, at the same kind of
pin, as every other Hanzo client.

## Canonical role
- This is the **real code** for the C++ full cloud SDK line: `hanzo-cpp/sdk`.
  `hanzoai/cpp-sdk` is a rename redirect to it (`gh api repos/hanzoai/cpp-sdk
  --jq .full_name` → `hanzo-cpp/sdk`), so the two are one repo, not two.
- Separate line: the AI/agents lib lives at `hanzo-cpp/ai`. Don't merge the two.
- One impl, one place. DRY. Discovery repos link OUT.

## Generation — one place, one way

`hanzoai/openapi` owns the invocation (`generate.py`) and the matrix of language
projections (`sdks.yaml`). **C++ is a row there like every other language**, and
`scripts/generate.sh` is a call site that says "cpp, into this checkout" and
nothing else. Nothing about HOW this SDK is generated lives in this repo.

```
OPENAPI=~/work/hanzo/openapi ./scripts/generate.sh          # regenerate include/ and src/
OPENAPI=~/work/hanzo/openapi ./scripts/generate.sh --check  # non-zero on drift
```

**This repo used to own its whole invocation, and both reasons it gave are
gone.** The first was templates: four `cpp-restsdk` mustache overrides are
required before the output compiles at all — measured, without them the library
fails with 8 errors in `ModelBase.h` — and the argument was that a template is a
FILE that must sit beside the call. Right about the risk, wrong about which
call: the call is `generate.py`'s `emit()`, so the overrides now live at
`templates/cpp-restsdk/` in the driver repo, one directory from the flags they
go with. The second was the generator version, and it expired (below).

What the departure cost is the reason not to repeat it: that script named
`hanzoai/openapi`'s `hanzo.yaml` at branch `main` while this repo's `hanzo.yml`
already declared cloud's `openapi.yaml`, and it re-derived the jar fetch, the
YAML-to-JSON conversion, `--skip-validate-spec` and the drift check — four
copies of logic the driver owns once. `generate.py cpp --check` reports **clean**
against the tree that script produced: byte for byte, 5327 files.

Both inputs arrive as VALUES. `$SPEC` is the document, already fetched at the
pinned ref and digest-checked; `$OPENAPI` is the checkout holding the driver.
hanzoai/ci's client lane sets both, because it holds the one credential that
reads the forge. Without `$SPEC` the driver reads `.spec-lock` and fetches the
commit it names from **`git.hanzo.ai`**, refusing if the bytes hash to anything
else. The forge serves its API at `/v1/`, **not** `/api/v1/`, and the wrong path
404s in a way that reads like a rejected credential.

### THE DOCUMENT MOVED — cloud's emission, not a projection of it

This client used to read `hanzoai/openapi`'s `hanzo.yaml` at branch `main`, with
no lock at all. It now reads what `hanzoai/cloud` emits, at a commit. Both halves
of that mattered: a branch name is not a version, and `hanzo.yaml` is a
projection of the same bytes rather than a second authority.

Measured across the two documents at this ref, so the trade is on the record
rather than argued:

| | hanzo.yaml | cloud openapi.yaml |
|---|---|---|
| routes | 2456 | **2479** |
| routes the other lacks | 0 | 23 |
| operations with a requestBody | 673 | 694 |
| operations with a typed 2xx | 1622 | 1645 |
| generated methods | 2456 | **2502** |
| …returning `void` | 834 | **834** |

**Nothing is lost: cloud's routes are a strict superset.** And the one thing
`publish.py` adds for C++ — a `default` response injected into every operation
that declares none — buys exactly nothing here: a `default` with no `content`
produces the same `void` return that no `responses` does, so both documents
yield the identical 834 void-returning methods. The projection's cost is real
(23 routes and 46 methods), its benefit for this language is zero.

### The generator version — 7.14.0, the fleet's one pin

This repo used to state a **7.24.0** floor of its own, and it was true when
written: two defects in the generator's type resolution left 10 translation units
uncompilable, neither reachable by a template. Both defects lived in schemas the
document **no longer carries** — `vector_NamedVectors`/`vector_DenseVector` under
`additionalProperties`, and `framework_Document.docstatus` — so the floor was a
fact about a document, not about `cpp-restsdk`.

Re-measured at the locked ref, 7.14.0 and 7.24.0 emit the **same library**: apart
from the generator string in each file's header comment and blank lines, exactly
one file differs, and the difference is a doc comment on
`ConsoleSettings::getOidcConfig`. 7.14.0 builds clean — 2663 sources, 0 errors.
So there is no cpp-specific version any more: `sdks.yaml` pins one generator for
everyone and this row needs no exception. The fleet moves as one when it moves.

**The document reaches the generator as JSON, and that is the whole of the
snakeyaml story.** swagger-parser hands YAML to snakeyaml, which refuses anything
over `3 * 1024 * 1024` = 3145728 code points and reports the refusal as
`Issues with the OpenAPI input` — which reads like a malformed spec and is not.
`-DmaxYamlCodePoints` lifts the cap and is NOT what the driver uses: it is
honoured by 7.24.0's parser and **ignored** by 7.14.0's. JSON avoids snakeyaml on
every version, so one mechanism serves every language and none of them carries a
ceiling the document will keep growing into.

### One flag, and it is the fleet's flag

`name-mappings: removed_at=removed_at_legacy,integration_config=integration_config_legacy`,
in the `cpp` row.
`o11y.GettableAgentCheckIn` declares `integration_config` AND `integrationConfig`,
`removed_at` AND `removedAt` — the snake spellings are live wire, published so
older agents keep working. C++ camel-cases a property into its accessor, so each
pair lands on one method declared twice and the library does not compile (5
errors). Renaming the ACCESSOR keeps both wire keys — measured, all four are
present in the emitted model, each under its own name. The python, go, java,
kotlin, ruby and php rows correct the same two schemas the same way.

### `templates/cpp-restsdk/` — four overrides, in `hanzoai/openapi`

Forked from **7.24.0** stock, so a generator bump re-derives them rather than
merging them. Each hunk is commented in place; none of them says anything about
the Hanzo API. Deforming the document to suit one C++ generator would degrade
the other language projections that read the same file — **do not do it**.

They are load-bearing, and the cost of dropping them is measured, not asserted:
regenerate this document with the same flags and no `-t`, then syntax-check all
2663 sources, and **24 errors** land in **two** files — 8 in
`include/hanzo/ModelBase.h` (defect 4, reached from `ConsoleSettings.cpp`'s
`std::vector<Object>`) and 16 in `src/model/Post_event_request.cpp` (defects 1–3,
the oneOf shape). Both root causes are below; check both if you ever re-derive.

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
- `templates/` — GONE from this repo; the overrides live in `hanzoai/openapi`
  at `templates/cpp-restsdk/`, beside the row whose flags they go with.

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
`store` deletes in a scope guard; `agent` polls the run list to a terminal state.

**Four flows changed route when the document did, and the rule they follow is
"demonstrate a call the document TYPES".** A generated client can only be as
typed as its document, and several of the obvious addresses are declared with no
body and no response — the generated method then takes nothing and returns
nothing, which cannot demonstrate anything. Where that was true, the flow moved
to the typed route for the same job and says so in its header comment:

| flow | was | is | why |
|---|---|---|---|
| `hello` | `bot_authMe` | `get_ai_account` → `Envelope` | the old route is in neither document |
| `chat` | `/v1/chat/completions` | `research_web` → `Report` | completions declares no body and no response |
| `money` | `/v1/billing/{balance,usage}` | `get_finance_{credits,usage}` | billing declares no parameters and no response |
| `tools` | `POST /v1/mcp` (JSON-RPC) | `aiMCPTools` → `AiMCPSurface` | `/v1/mcp` is in neither document |

`agent` kept its route and lost its input: `postAgentsByRefRun` is declared with
no request body and no response, so it starts the run and reports nothing — the
id, the status and the output are read from the typed run LIST instead.

None of this came from the document move. Every one of those routes is untyped
or absent in `hanzo.yaml` too, measured by regenerating from it: the committed
tree simply predated the change. Do not "restore" these to the old operations —
they do not exist, and a client cannot invent a shape the document does not
declare.

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
a C++ generator target that compiles from this document without an archived HTTP
stack. When that happens the move is a generator change plus new
`templates/<target>/` overrides — the document, the seam and the six flows do
not move.

Scale is not a problem, and these are measured rather than estimated. At document
`49389cf9`, aarch64 Ubuntu 24.04, g++ 13.3, generator 7.14.0, `-j20`,
`-DCMAKE_BUILD_TYPE=Release`, from a clean tree: **2663 sources / 2664 headers**,
192 API classes, 2462 models, 2502 operations; the library builds with **0
errors** and `libhanzo.a` is **349 MB**; the six examples then build with **0
errors**. Generation is well under a minute now that the document reaches the
generator as JSON. The file count moves with every resync — re-measure, do not
quote.

**Quote the build type with the archive size or the number means nothing.** 349
MB is Release. `cmake -B build && cmake --build build` leaves `CMAKE_BUILD_TYPE`
EMPTY, which is not a smaller build — it is an unoptimized one that inlines
nothing and keeps every template instantiation, and the same 2664 objects then
archive to **1281 MB**, 3.7x. Both figures are this tree at this document; they
differ only in that one flag.

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
- On the `hanzoai/*` repos the same workflow instead sat `queued` forever, for
  capacity rather than permissions: `hanzoai/ci` defaults to
  `runs-on: [hanzo-build-linux-amd64]` and nothing was registered under that
  label. That label is now served by the **`git-runner` fleet on git.hanzo.ai**
  (StatefulSet `git-runner`, ns `hanzo`) — the arc pool this note used to name
  was retired. Re-measure before claiming either state; do not quote this.

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
