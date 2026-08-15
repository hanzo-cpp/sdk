#!/usr/bin/env bash
# Regenerate include/ and src/ from the Hanzo Cloud API document.
#
#   ./scripts/generate.sh            # regenerate in place
#   ./scripts/generate.sh --check    # diff only; non-zero if the tree drifted
#
# THE DOCUMENT IS THE ONE hanzoai/cloud EMITS, AT THE COMMIT `.spec-lock` NAMES.
# cloud projects its own routers into openapi.yaml and gates the emission by
# regenerating and failing on any diff, so it cannot describe a route the binary
# does not serve. Every Hanzo client reads that file at a pinned commit and
# carries the same four-line lock; this one is no exception.
#
# WHY THIS REPO OWNS ITS INVOCATION while python, go, typescript, java, kotlin,
# ruby and php are one row each in hanzoai/openapi's sdks.yaml: cpp-restsdk needs
# TEMPLATE OVERRIDES, and a template is a FILE that has to sit beside the call or
# the pair drifts apart. Measured at the locked ref: without templates/cpp-restsdk
# the library does not compile (8 errors, all in ModelBase.h). That is the whole
# of the reason, and it is the same one rust's own generate.sh states. What is
# NOT allowed is a row there AND flags here.
#
# Environment:
#   SPEC         a document on disk; skips the fetch and the digest check
#   FORGE_TOKEN  contents:read on git.hanzo.ai — how the document is read
#   JAR          an openapi-generator-cli jar to use instead of downloading
#
# Requires: java 17+, curl, python3 with PyYAML.
set -euo pipefail
cd "$(dirname "$0")/.."

# 7.14.0 — the version every other Hanzo client pins, and this repo is measured
# on it rather than exempt from it. An earlier floor of 7.24.0 was real when it
# was written: two defects in the generator's type resolution left 10 translation
# units uncompilable, and neither is reachable by a template. Both defects were
# in schemas the document no longer carries, so at the locked ref 7.14.0 and
# 7.24.0 emit the same library — one doc comment apart — and 7.14.0 builds clean.
# Anything above this is fine; the fleet moves as one when it moves.
GENERATOR_VERSION="${GENERATOR_VERSION:-7.14.0}"
JAR="${JAR:-${TMPDIR:-/tmp}/openapi-generator-cli-${GENERATOR_VERSION}.jar}"
FORGE="https://git.hanzo.ai/v1"

check=0
[ "${1:-}" = "--check" ] && check=1

spec_tmp=""
out=""
cleanup() { [ -n "$spec_tmp" ] && rm -f "$spec_tmp"; [ -n "$out" ] && rm -rf "$out"; return 0; }
trap cleanup EXIT

# `.spec-lock` is a receipt, not configuration: a cloud release writes it, and
# reading it here is what makes "this client is a projection of that commit" a
# checkable fact. Four keys, one per line.
lock() { sed -n "s/^$1=//p" .spec-lock; }
SPEC="${SPEC:-}"
if [ -z "$SPEC" ]; then
  [ -f .spec-lock ] || { echo "error: no .spec-lock, so this client names no document." >&2; exit 1; }
  repo="$(lock repo)"; path="$(lock path)"; ref="$(lock ref)"; wants="$(lock sha256)"
  : "${repo:?}" "${path:?}" "${ref:?}"
  spec_tmp="$(mktemp "${TMPDIR:-/tmp}/hanzo-spec.XXXXXX")"
  SPEC="$spec_tmp"
  [ -n "${FORGE_TOKEN:-}" ] || {
    echo "error: reading $repo@$ref from git.hanzo.ai needs FORGE_TOKEN (contents:read)." >&2
    echo "       Or pass SPEC=/path/to/openapi.yaml from a checkout." >&2
    exit 1
  }
  # The forge serves its API at /v1/, not /api/v1/. Same address hanzoai/openapi's
  # generate.py reads, because there is one document at one address.
  curl -fsSL -H "Authorization: token $FORGE_TOKEN" \
    "$FORGE/repos/$repo/raw/$path?ref=$ref" -o "$SPEC"
  got="$(sha256sum "$SPEC" | cut -d' ' -f1)"
  # A pinned commit whose bytes moved is not something regenerating can make
  # safe, so it stops here — the same refusal hanzoai/ci makes.
  [ -z "$wants" ] || [ "$got" = "$wants" ] || {
    echo "error: $repo@$ref:$path hashes to $got, .spec-lock says $wants" >&2; exit 1; }
fi

if [ ! -f "$JAR" ]; then
  echo "fetching openapi-generator-cli $GENERATOR_VERSION" >&2
  curl -fsSL -o "$JAR" \
    "https://repo1.maven.org/maven2/org/openapitools/openapi-generator-cli/${GENERATOR_VERSION}/openapi-generator-cli-${GENERATOR_VERSION}.jar"
fi

out="$(mktemp -d "${TMPDIR:-/tmp}/hanzo-cpp-gen.XXXXXX")"

# The document as JSON, because YAML has a ceiling and JSON does not.
# swagger-parser hands YAML to snakeyaml, which refuses anything over
# 3 * 1024 * 1024 = 3145728 code points, and reports the refusal as a malformed
# spec. JSON avoids snakeyaml on every generator version, so the fleet driver and
# this script do the same one thing and neither carries a ceiling the document
# will keep growing into.
spec_json="$out/openapi.json"
python3 -c 'import json,sys,yaml; json.dump(yaml.safe_load(open(sys.argv[1])), open(sys.argv[2],"w"))' \
  "$SPEC" "$spec_json"

# --skip-validate-spec: the document is OpenAPI 3.1, which made `responses`
# OPTIONAL on an operation. The 7.14.0 validator still enforces the 3.0 rule that
# it is required, so it refuses a document that is valid — 716 of 2479 operations
# are routes the router proves exist and whose response shape no seam can state,
# and cloud emits those with no `responses` on purpose. What keeps a bad document
# out is the cmake build in hanzo.yml's test: block, not the validator.
#
# --name-mappings: o11y.GettableAgentCheckIn declares `integration_config` AND
# `integrationConfig`, `removed_at` AND `removedAt` — the snake spellings are live
# wire, published so older agents keep working. C++ camel-cases a property into
# its accessor, so each pair lands on one method declared twice and the library
# does not compile. Renaming the ACCESSOR keeps both wire keys: the generator
# still serializes each under the document's own name, measured — all four keys
# are present in the emitted model. The python, go, java, kotlin, ruby and php
# rows correct the same two schemas the same way.
#
# apiDocs/modelDocs/apiTests/modelTests off: ~4000 files of nothing at this size.
# skipFormModel=false: edge_deployFunction offers both application/octet-stream
# and an inline multipart/form-data body, and the generator otherwise imports a
# form model it did not emit.
# -t templates/cpp-restsdk: four overrides, each hunk commented in place. They
# correct the generator; they say nothing about this API.
java -Xmx3g -jar "$JAR" generate \
  -g cpp-restsdk \
  --skip-validate-spec \
  -i "$spec_json" \
  -o "$out" \
  -t templates/cpp-restsdk \
  --global-property apis,models,supportingFiles,apiDocs=false,modelDocs=false,apiTests=false,modelTests=false,skipFormModel=false \
  --additional-properties "packageName=hanzo,apiPackage=hanzo.api,modelPackage=hanzo.model" \
  --name-mappings "removed_at=removed_at_legacy,integration_config=integration_config_legacy" \
  >/dev/null

# The repo owns its build, its ignore rules and its README; the generator offers
# a copy of each and none of them may land. CMakeLists.txt especially: the
# hanzo::hanzo target, the install/export set and the examples switch are this
# repo's contract with a consumer, not a generator artifact.
rm -f "$out"/CMakeLists.txt "$out"/Config.cmake.in "$out"/git_push.sh \
      "$out"/README.md "$out"/.gitignore "$out"/.openapi-generator-ignore
rm -rf "$out"/.openapi-generator

if [ "$check" = 1 ]; then
  # The client is generated, so the only thing that can rot is the committed
  # copy. This is what makes "it cannot be hand-edited" a fact and not a
  # convention.
  rc=0
  for d in include src; do
    if ! diff -qr "$out/$d" "$d" >/dev/null 2>&1; then
      echo "DRIFTED: $d"
      diff -qr "$out/$d" "$d" 2>&1 | head -20
      rc=1
    fi
  done
  [ "$rc" = 0 ] && echo "clean: include/ and src/ are what the locked document projects"
  exit "$rc"
fi

# Replaced wholesale, so a renamed or dropped operation cannot leave a stale
# header behind.
rm -rf include src
cp -r "$out"/include "$out"/src .

echo "generated $(find include -name '*.h' | wc -l) headers and $(find src -name '*.cpp' | wc -l) sources"
echo "  document  ${repo:-$SPEC}${ref:+@$ref}"
echo "  generator openapi-generator $GENERATOR_VERSION, cpp-restsdk, templates/cpp-restsdk"
