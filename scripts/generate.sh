#!/usr/bin/env bash
# Regenerate the Hanzo C++ SDK from the unified OpenAPI spec.
#
# This is a CALL SITE, not a generator invocation. hanzoai/openapi owns both the
# document (hanzo.yaml) and the projection of it into every language: the matrix
# is data in sdks.yaml and the driver is generate.py. Every SDK repo calls that
# one driver, so no client can drift on its own and --check is what makes that a
# fact rather than a convention.
#
#   ./scripts/generate.sh            # regenerate this repo's client in place
#   ./scripts/generate.sh --check    # diff only; non-zero if the client drifted
#
# Environment:
#   OPENAPI_DIR  an existing hanzoai/openapi checkout to use instead of cloning
#   SPEC_REPO    default hanzoai/openapi
#   SPEC_REF     default main
#   SPEC_TOKEN, GH_TOKEN, GITHUB_TOKEN — hanzoai/openapi is PRIVATE, so a clone
#                needs one of these. raw.githubusercontent.com 404s on it.
#
# Requires: python3 with PyYAML, java 17+, git.
#
# STATUS — this script cannot succeed yet, and that is the honest state of the
# lane rather than a bug in the script. sdks.yaml carries no `cpp` row, because
# no openapi-generator 7.14.0 C++ generator emits a tree that compiles from this
# document. `generate.py cpp` therefore exits with "invalid choice: cpp". The
# exact blockers, with counts, are in REFUSAL.md. Adding the row is the last
# step of fixing them, not the first.
set -euo pipefail
cd "$(dirname "$0")/.."

SPEC_REPO="${SPEC_REPO:-hanzoai/openapi}"
SPEC_REF="${SPEC_REF:-main}"

if [ -n "${OPENAPI_DIR:-}" ]; then
  openapi="$OPENAPI_DIR"
  [ -f "$openapi/generate.py" ] || { echo "OPENAPI_DIR=$openapi has no generate.py" >&2; exit 1; }
else
  openapi="$(mktemp -d)"
  trap 'rm -rf "$openapi"' EXIT
  token="${SPEC_TOKEN:-${GH_TOKEN:-${GITHUB_TOKEN:-}}}"
  : "${token:?$SPEC_REPO is private: set SPEC_TOKEN (or GH_TOKEN/GITHUB_TOKEN), or point OPENAPI_DIR at a checkout}"
  git clone --depth 1 --branch "$SPEC_REF" \
    "https://x-access-token:${token}@github.com/${SPEC_REPO}.git" "$openapi" >/dev/null 2>&1
fi

echo "spec: $SPEC_REPO@$(git -C "$openapi" rev-parse --short HEAD)"
exec python3 "$openapi/generate.py" cpp --repo "$PWD" "$@"
