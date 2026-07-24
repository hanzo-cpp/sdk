# LLM.md — hanzo-cpp/sdk

Guidance for AI agents working in this repo.

## What this is
The **full Hanzo cloud SDK for C++** — a typed client over the entire `/v1`
surface (AI, agents, inference, compute, data, network, security/IAM/KMS,
platform, observe, web3). Generated from the cloud OpenAPI spec.

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

## Brand rules (hard — enforce in all docs/code)
- **Never** call Hanzo an "LLM gateway" or position it against LiteLLM. It is a
  full AI SDK / AI cloud, not a proxy.
- Paths are `/v1/…` only — never an `/api/` prefix.
- **Zen** models are Hanzo's own family — never reference upstream model names.
- Voice: "Hanzo — the Open AI Cloud." Modern, crisp, developer-first.

## Key entry points
- `include/hanzo/client.hpp` — client construction + service accessors.
- `CMakeLists.txt` — the `hanzo::hanzo` target and install/export config.
- Service methods are code-generated from the OpenAPI spec; regenerate, don't hand-fork.

## Pointer
Full SDK model + one-way rules: `~/work/hanzo/SDK-ARCHITECTURE.md`.
