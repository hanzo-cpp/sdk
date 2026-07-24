# Hanzo C++ SDK

The full Hanzo cloud, native in C++ — one typed client for every `/v1` service: AI and agents, inference, compute, data, network, security (IAM/KMS), platform, observe, and web3.

Generated from the cloud OpenAPI spec, so the surface is always complete and never drifts from the platform.

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

## Usage

Initialize a client and make a call. Configuration is read from `HANZO_API_KEY`, or set it explicitly:

```cpp
#include <hanzo/client.hpp>
#include <iostream>

int main() {
  hanzo::Client client{{.api_key = std::getenv("HANZO_API_KEY")}};

  auto response = client.chat.completions.create({
    .model = "zen-1",
    .messages = {
      {.role = "user", .content = "Explain composable systems in one sentence."},
    },
  });

  std::cout << response.choices.at(0).message.content << '\n';
}
```

Every request targets `https://api.hanzo.ai/v1` by default. Point at another
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

## Configuration

| Option | Env | Default |
|--------|-----|---------|
| `api_key` | `HANZO_API_KEY` | — |
| `base_url` | `HANZO_BASE_URL` | `https://api.hanzo.ai/v1` |
| `timeout` | — | `60s` |

## Building from source

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

## Hanzo — the Open AI Cloud

Open source · every language · on-chain settlement. [hanzo.ai](https://hanzo.ai) · [docs.hanzo.ai](https://docs.hanzo.ai)

**SDKs in every language** — [Python](https://github.com/hanzoai/python-sdk) (flagship) · [TypeScript](https://github.com/hanzo-js/sdk) · [Go](https://github.com/hanzo-go/sdk) · [Rust](https://github.com/hanzo-rs/sdk) · [C++](https://github.com/hanzo-cpp/sdk) · [Swift](https://github.com/hanzo-swift/sdk) · [Kotlin](https://github.com/hanzo-kt/sdk) · [umbrella](https://github.com/hanzoai/sdk)
