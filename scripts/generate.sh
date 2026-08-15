#!/usr/bin/env bash
# The call site. Nothing about HOW this SDK is generated lives here.
#
# The invocation is logic and lives once, in hanzoai/openapi's `generate.py`;
# every per-language knob is data in `sdks.yaml` beside it, including the four
# `cpp-restsdk` mustache overrides this client needs — those now sit at
# `templates/cpp-restsdk/` in the driver repo, next to the flags they go with.
# This file says "cpp, into this checkout" and nothing else.
#
#   ./scripts/generate.sh              # regenerate include/ and src/
#   ./scripts/generate.sh --check      # fail if the committed client drifted
#
# WHAT THIS REPLACED, because the reason is the whole point. This script used to
# carry the entire invocation: it fetched the generator jar, converted YAML to
# JSON, passed --skip-validate-spec, ran the diff check, and named the document
# itself. Four of those are copies of logic the driver already owns once, and
# the fifth is what went wrong — it defaulted to SPEC_REPO=hanzoai/openapi,
# `hanzo.yaml`, at the branch `main`, while this repo's own hanzo.yml already
# declared hanzoai/cloud's `openapi.yaml`. One repo, two answers to "which
# document is this client a projection of", and no `.spec-lock` to settle it.
#
# It also pinned generator 7.24.0 against the fleet's 7.14.0. That floor was
# real when written and expired with the document that caused it: both defects
# it named live in schemas the document no longer carries, and at the locked ref
# the two versions emit the same library. There is one pin now, in sdks.yaml.
#
# BOTH INPUTS ARRIVE AS VALUES. $SPEC is the document, already fetched at a
# pinned ref and digest-checked; $OPENAPI is the checkout holding the driver.
# hanzoai/ci's client lane sets both, because it holds the one credential that
# reads this forge. Without $SPEC the driver reads `.spec-lock` and fetches the
# ref it names, which is what a maintainer regenerating by hand gets.
#
# uv rather than a bare python3: the driver needs PyYAML and the runner image
# promises no interpreter at all, let alone one with it installed.
#
# Requires: java 17+, uv.
set -euo pipefail
cd "$(dirname "$0")/.."

: "${OPENAPI:?the generator lives in hanzoai/openapi; the ci client lane sets OPENAPI, or point it at a checkout}"

if [ -n "${SPEC:-}" ]; then set -- --spec "$SPEC" "$@"; fi

exec uv run --with pyyaml python3 "$OPENAPI/generate.py" cpp --repo "$PWD" "$@"
