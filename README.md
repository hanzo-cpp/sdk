# Hanzo C++ SDK

The full Hanzo cloud, native in C++ — one typed client for every `/v1` service: AI and agents, inference, compute, data, network, security (IAM/KMS), platform, observe, and web3.

Generated from the cloud OpenAPI spec, so the surface is always complete and never drifts from the platform.

## Status

**The client is not published yet.** No openapi-generator 7.14.0 C++ generator
currently emits a tree that compiles from the Hanzo cloud spec — the closest,
`cpp-restsdk`, still leaves 11 of 2316 translation units broken on defects in
the generator's own type resolution. Neither scale nor the HTTP dependency is
the problem: the library builds ~2300 objects in under 11 minutes at `-j20`, and
cpprestsdk + Boost + OpenSSL resolve cleanly. Rather than commit a tree that
does not build, this repo carries the generation seam, the CI gates, and the
generator template overrides that fix the five defect classes we could reach.

The measurement, the exact remaining blockers and the path to a client are in
[`LLM.md`](./LLM.md). Everything below is the contract that lands with it.

## Install

Fetch it with CMake and link against the `hanzo::hanzo` target:

```cmake
find_package(hanzo CONFIG REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE hanzo::hanzo)
```

Or vendor it directly with `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
  hanzo
  GIT_REPOSITORY https://github.com/hanzo-cpp/sdk.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(hanzo)
```

Requires a C++17 compiler and CMake 3.20+.

## Configuration

Auth is a bearer token — an IAM-issued JWT or an `hk-` Cloud API key — read from
the environment, or set explicitly on the client.

| Option | Env | Default |
|--------|-----|---------|
| `api_key` | `HANZO_API_KEY` | — |
| `base_url` | `HANZO_BASE_URL` | `https://api.hanzo.ai` |
| `org_id` | `HANZO_ORG_ID` | — (required by org-scoped services, sent as `X-Org-Id`) |
| `timeout` | — | `60s` |

Every request targets `https://api.hanzo.ai` by default. Point at another
environment by setting `base_url` on the client options.

Models are the **Zen** family (`zen-1`, and siblings) — Hanzo's own models, first-class across the whole SDK.

## What's covered

One client, the entire cloud surface under `/v1`:

| Domain | Services |
|--------|----------|
| **AI** | Models, agents, inference, fine-tuning, embeddings, evals |
| **Compute** | GPUs, machines, containers, functions, jobs |
| **Data** | Vector, SQL, KV, object storage, datastore, DocDB |
| **Network** | Gateway, VPC, DNS, CDN, load balancer |
| **Security** | IAM, authz, KMS, secrets, audit |
| **Platform** | Projects, environments, builds, registry, releases |
| **Observe** | Logs, metrics, traces, dashboards, alerts |
| **Web3** | Settlement, chains, wallets, tokens, indexer |

## Regenerating

The spec is the only source. `hanzoai/openapi` owns both the document and its
projection into every language, and this script is a call site into that one
driver — never a bespoke generator invocation:

```bash
./scripts/generate.sh            # regenerate the client in place
./scripts/generate.sh --check    # diff only; non-zero if the client drifted
```

Never edit generated sources. Edit the per-service spec in `hanzoai/openapi` and
regenerate.

## Building from source

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Hanzo — the Open AI Cloud

Open source · every language · on-chain settlement. [hanzo.ai](https://hanzo.ai) · [docs.hanzo.ai](https://docs.hanzo.ai)

**SDKs in every language** — [Python](https://github.com/hanzoai/python-sdk) (flagship) · [TypeScript](https://github.com/hanzo-js/sdk) · [Go](https://github.com/hanzo-go/sdk) · [Rust](https://github.com/hanzo-rs/sdk) · [C++](https://github.com/hanzo-cpp/sdk) · [Swift](https://github.com/hanzo-swift/sdk) · [Kotlin](https://github.com/hanzo-kt/sdk) · [umbrella](https://github.com/hanzoai/sdk)
