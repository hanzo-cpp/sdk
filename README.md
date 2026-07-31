# Hanzo C++ SDK

The full Hanzo cloud, native in C++ — one typed client over every `/v1` service: AI and agents, inference, compute, data, network, security (IAM/KMS), platform, observe, and web3.

Generated from `hanzoai/openapi`, so the surface is complete by construction and cannot drift from the platform. Nothing here is hand-written, and `./scripts/generate.sh --check` is what makes that a fact rather than a convention.

## Install

C++ has no package registry, so a consumer takes it from a tag.

```cmake
include(FetchContent)
FetchContent_Declare(
  hanzo
  GIT_REPOSITORY https://github.com/hanzo-cpp/sdk.git
  GIT_TAG        v8.0.0
)
FetchContent_MakeAvailable(hanzo)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE hanzo::hanzo)
```

Or install it once and resolve it as a package:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build -j"$(nproc)" --target install
```

```cmake
find_package(hanzo CONFIG REQUIRED)
target_link_libraries(app PRIVATE hanzo::hanzo)
```

Requires C++17, CMake 3.20+, and cpprestsdk + Boost + OpenSSL
(`apt install libcpprest-dev libboost-dev libssl-dev`).

## Use

```cpp
#include <cstdlib>
#include <iostream>

#include "hanzo/ApiClient.h"
#include "hanzo/ApiConfiguration.h"
#include "hanzo/api/AuthApi.h"

int main() {
    auto config = std::make_shared<hanzo::api::ApiConfiguration>();
    config->setBaseUrl(U("https://api.hanzo.ai"));
    config->getDefaultHeaders()[U("Authorization")] =
        U("Bearer ") + utility::conversions::to_string_t(std::getenv("HANZO_API_KEY"));

    hanzo::api::AuthApi auth(std::make_shared<hanzo::api::ApiClient>(config));
    auto me = auth.botAuthMe().get();

    std::cout << utility::conversions::to_utf8string(me->getDisplayName()) << std::endl;
}
```

Every call returns a `pplx::task<T>`: `.get()` blocks, `.then(...)` composes.

## Configuration

Auth is a bearer token — an IAM-issued JWT or an `hk-` Cloud API key.

| Setting | Env the examples read | Default |
|---|---|---|
| bearer token | `HANZO_API_KEY` | — (required) |
| base URL | `HANZO_BASE_URL` | `https://api.hanzo.ai` |
| org | `HANZO_ORG_ID` | — (required by org-scoped services, sent as `X-Org-Id`) |

Models are the **Zen** family (`zen-1`, and siblings) — Hanzo's own models, first-class across the whole SDK.

## Examples

The six canonical flows every Hanzo SDK ships, named and ordered by
`hanzoai/openapi`'s `flows.yaml`:

| flow | what it does |
|---|---|
| `hello` | Identity — the call that says no, and who the key belongs to. |
| `chat` | One completion. |
| `money` | Balance, then the usage that moved it. |
| `store` | KV round-trip — create, read back, delete. |
| `agent` | Create an agent, run it, poll the run to a terminal state. |
| `tools` | List the MCP tools this key can reach. |

```bash
cmake -B build -DHANZO_BUILD_EXAMPLES=ON
cmake --build build -j"$(nproc)" --target hello chat money store agent tools
HANZO_API_KEY=hk-... ./build/examples/hello
```

They resolve the key, the base URL and the org in ONE place — `examples/hanzo.cpp` — not once per flow.

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

The document is the only source.

```bash
./scripts/generate.sh            # regenerate include/ and src/ in place
./scripts/generate.sh --check    # diff only; non-zero if the client drifted
```

`hanzoai/openapi` is private, so the script needs `SPEC_TOKEN` (or `GH_TOKEN` /
`GITHUB_TOKEN`), or `SPEC=/path/to/hanzo.yaml` from a checkout. It refuses,
loudly and by name, rather than falling back to a stale spec.

Never edit generated sources. Edit the per-service spec in `hanzoai/openapi` and
regenerate.

## Hanzo — the Open AI Cloud

Open source · every language · on-chain settlement. [hanzo.ai](https://hanzo.ai) · [docs.hanzo.ai](https://docs.hanzo.ai)

**SDKs in every language** — [Python](https://github.com/hanzoai/python-sdk) (flagship) · [TypeScript](https://github.com/hanzoai/js-sdk) · [Go](https://github.com/hanzo-go/sdk) · [Rust](https://github.com/hanzo-rs/sdk) · [C++](https://github.com/hanzo-cpp/sdk) · [Java](https://github.com/hanzoai/java-sdk) · [Kotlin](https://github.com/hanzo-kotlin/sdk) · [umbrella](https://github.com/hanzoai/sdk)

## License

Apache-2.0 — see [`LICENSE`](./LICENSE).
