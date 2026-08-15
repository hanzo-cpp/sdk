# Hanzo C++ SDK

The full Hanzo cloud, native in C++ — one typed client over every `/v1` service: AI and agents, inference, compute, data, network, security (IAM/KMS), platform, observe, and web3.

Generated from the OpenAPI document `hanzoai/cloud` emits, at the commit `.spec-lock` names, so the surface is complete by construction and cannot drift from the platform. Nothing under `include/` or `src/` is hand-written, and `./scripts/generate.sh --check` is what makes that a fact rather than a convention.

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
#include "hanzo/ApiException.h"
#include "hanzo/api/AskApi.h"

int main() {
    const char* key = std::getenv("HANZO_API_KEY");
    if (key == nullptr) {
        std::cerr << "HANZO_API_KEY is not set" << std::endl;
        return 1;
    }

    auto config = std::make_shared<hanzo::api::ApiConfiguration>();
    config->setBaseUrl(U("https://api.hanzo.ai"));
    config->getDefaultHeaders()[U("Authorization")] =
        U("Bearer ") + utility::conversions::to_string_t(key);

    hanzo::api::AskApi ask(std::make_shared<hanzo::api::ApiClient>(config));
    try {
        auto question = std::make_shared<hanzo::model::WebQuestion>();
        question->setQ(U("What are the three laws of thermodynamics?"));

        auto report = ask.researchWeb(question).get();
        std::cout << utility::conversions::to_utf8string(report->getAnswer()) << std::endl;
    } catch (const hanzo::api::ApiException& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
```

Every call returns a `pplx::task<T>`: `.get()` blocks, `.then(...)` composes.

## Configuration

Auth is a bearer token — an IAM-issued JWT or an `hk-` Cloud API key. The
document declares that scheme globally; `cpp-restsdk` does not emit a per-call
application for an HTTP bearer scheme, so the token is set once as a default
header on the configuration, as above and as `examples/hanzo.cpp` does.

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
| `hello` | Identity — the account this key resolves to, and the verdict it returns. |
| `chat` | One question, answered with its sources cited. |
| `money` | Credits remaining, then the usage that spent them. |
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

The document is the only source, and `.spec-lock` names it by commit:

```
ref=49389cf91a54e31f174ba6ac3a76e9c33b3f772c
sha256=eb375622d4d53653672031aeb2fb473424b26baa975be78693226a04a5005571
repo=hanzoai/cloud
path=openapi.yaml
```

```bash
./scripts/generate.sh            # regenerate include/ and src/ in place
./scripts/generate.sh --check    # diff only; non-zero if the client drifted
```

The document is read from `git.hanzo.ai` at that commit and refused if its bytes
hash to anything else, so this cannot regenerate from a document nobody shipped.
That read needs `FORGE_TOKEN` (contents:read); `SPEC=/path/to/openapi.yaml` from
a checkout skips the fetch.

Never edit generated sources. `hanzoai/cloud` emits the document from its own
routers, so a route changes there and arrives here by regeneration.

## Hanzo — the Open AI Cloud

Open source · every language · on-chain settlement. [hanzo.ai](https://hanzo.ai) · [docs.hanzo.ai](https://docs.hanzo.ai)

**SDKs in every language** — [Python](https://github.com/hanzoai/python-sdk) (flagship) · [TypeScript](https://github.com/hanzoai/js-sdk) · [Go](https://github.com/hanzo-go/sdk) · [Rust](https://github.com/hanzo-rs/sdk) · [C++](https://github.com/hanzo-cpp/sdk) · [Java](https://github.com/hanzoai/java-sdk) · [Kotlin](https://github.com/hanzo-kotlin/sdk) · [umbrella](https://github.com/hanzoai/sdk)

## License

Apache-2.0 — see [`LICENSE`](./LICENSE).
