#!/usr/bin/env bash
# Regenerate include/ and src/ from the Hanzo OpenAPI document.
#
# hanzoai/openapi owns the document (hanzo.yaml) and the matrix of language
# projections (sdks.yaml + generate.py). Most clients are a call site into that
# driver and carry no flags at all. C++ cannot be: it needs TEMPLATE OVERRIDES,
# and a template is a FILE that has to sit beside the invocation or the pair
# drifts apart. So C++ owns its whole invocation here — the same document, the
# same generator, one place — exactly as go and rust do, and for the same stated
# reason. What is NOT allowed is a row there AND flags here.
#
#   ./scripts/generate.sh            # regenerate in place
#   ./scripts/generate.sh --check    # diff only; non-zero if the tree drifted
#
# Environment:
#   SPEC        a hanzo.yaml on disk; skips fetching entirely
#   SPEC_URL    where to fetch it from (default: raw.githubusercontent.com)
#   SPEC_REPO   default hanzoai/openapi
#   SPEC_REF    default main
#   JAR         an openapi-generator-cli jar to use instead of downloading
#   GENERATOR_VERSION  default 7.24.0 — see the note below, it is not free
#   SPEC_TOKEN, GH_TOKEN, GITHUB_TOKEN — hanzoai/openapi is PRIVATE
#
# Requires: java 17+, curl, python3 with PyYAML.
set -euo pipefail
cd "$(dirname "$0")/.."

# 7.24.0, and NOT the 7.14.0 that sdks.yaml pins for every other language. This
# is a measured floor, not a preference. On 7.14.0 this document leaves 10
# translation units uncompilable (measured at spec ea45dde, 2370 TUs; the count
# moves with the document, the defects do not) through two defects in its own
# Java type resolution, neither of which a mustache template can reach —
# mustache interpolates the strings that resolution produces, it cannot
# recompute a type:
#
#   9 TUs  `additionalProperties: {$ref: <array-typed schema>}` emits
#          `std::map<utility::string_t, std::vector>` — the aliased array's
#          element type is dropped (vector_NamedVectors -> vector_DenseVector).
#   1 TU   an integer-backed enum declares its conversion helpers over
#          `utility::string_t` and defines them over `int32_t`
#          (framework_Document.docstatus).
#
# 7.24.0 fixes both. The fleet raises to 7.24.0 as well, but in ONE coordinated
# regeneration wave — deliberately NOT piecemeal, so that the version bump and
# the in-flight typing of cloud's untyped routes (648 of 2455 operations carry
# neither a requestBody nor a typed 2xx schema; 621 of those are cloud_) change
# each language's committed output once rather than twice. Do not bump another
# language here.
# Anything ABOVE this floor is fine; below it, this repo does not build.
GENERATOR_VERSION="${GENERATOR_VERSION:-7.24.0}"
SPEC_REPO="${SPEC_REPO:-hanzoai/openapi}"
SPEC_REF="${SPEC_REF:-main}"
SPEC_URL="${SPEC_URL:-https://raw.githubusercontent.com/${SPEC_REPO}/${SPEC_REF}/hanzo.yaml}"
SPEC="${SPEC:-}"
JAR="${JAR:-${TMPDIR:-/tmp}/openapi-generator-cli-${GENERATOR_VERSION}.jar}"

check=0
[ "${1:-}" = "--check" ] && check=1

spec_tmp=""
out=""
cleanup() { [ -n "$spec_tmp" ] && rm -f "$spec_tmp"; [ -n "$out" ] && rm -rf "$out"; return 0; }
trap cleanup EXIT

if [ -z "$SPEC" ]; then
  spec_tmp="$(mktemp "${TMPDIR:-/tmp}/hanzo-spec.XXXXXX")"
  SPEC="$spec_tmp"
  # Public fetch first: it needs no credential and starts working the day
  # hanzoai/openapi opens. While the repo is private, raw.githubusercontent.com
  # answers 404 rather than 403, so an anonymous miss looks exactly like a
  # deleted file — hence the fallback, which says which case it was. Both paths
  # are `curl -f` under `set -e`, so a failed fetch STOPS here. Regenerating
  # from a stale spec is the one outcome this script must never produce quietly.
  if ! curl -fsSL "$SPEC_URL" -o "$SPEC"; then
    token="${SPEC_TOKEN:-${GH_TOKEN:-${GITHUB_TOKEN:-$(gh auth token 2>/dev/null || true)}}}"
    if [ -z "$token" ]; then
      cat >&2 <<EOF
error: could not read the spec, and there is no token to try again with.

  CAUSE  $SPEC_REPO is a PRIVATE repository. raw.githubusercontent.com serves
         public repos only and answers 404 — not 403 — for a private path, so
         an anonymous miss is indistinguishable from a deleted file.
  TRIED  $SPEC_URL
  FIX    export SPEC_TOKEN (or GH_TOKEN / GITHUB_TOKEN) with contents:read, run
         'gh auth login', or pass SPEC=/path/to/hanzo.yaml from a checkout.

Refusing to continue: generating from a stale spec is worse than not generating.
EOF
      exit 1
    fi
    echo "note: $SPEC_URL served no spec ($SPEC_REPO is private) — reading it through the GitHub API instead" >&2
    curl -fsSL \
      -H "Authorization: Bearer $token" \
      -H "Accept: application/vnd.github.raw" \
      "https://api.github.com/repos/${SPEC_REPO}/contents/hanzo.yaml?ref=${SPEC_REF}" -o "$SPEC"
  fi
fi

if [ ! -f "$JAR" ]; then
  echo "fetching openapi-generator-cli $GENERATOR_VERSION" >&2
  curl -fsSL -o "$JAR" \
    "https://repo1.maven.org/maven2/org/openapitools/openapi-generator-cli/${GENERATOR_VERSION}/openapi-generator-cli-${GENERATOR_VERSION}.jar"
fi

out="$(mktemp -d "${TMPDIR:-/tmp}/hanzo-cpp-gen.XXXXXX")"

# The document as JSON, because YAML has a ceiling and JSON does not.
# swagger-parser hands YAML to snakeyaml, which refuses anything over
# 3 * 1024 * 1024 = 3145728 code points; hanzo.yaml passed that mark and the
# failure does not say so — it logs SnakeException, silently falls through to
# the SWAGGER 2.0 compat reader, and dies with "Issues with the OpenAPI input",
# which reads like a malformed spec. It is not; the document validates at 0
# errors.
#
# -DmaxYamlCodePoints lifts the cap and is NOT the fix: 7.24.0's swagger-parser
# honours it, 7.14.0's IGNORES it, and 7.14.0 is what the rest of the fleet
# pins. JSON avoids snakeyaml altogether on every version — measured, not
# assumed: a JSON 539 KB OVER the ceiling validates clean on 7.14.0. So the
# fleet driver and this script do the same one thing, and neither carries a
# ceiling the document will keep growing into.
spec_json="$out/hanzo.json"
python3 -c 'import json,sys,yaml; json.dump(yaml.safe_load(open(sys.argv[1])), open(sys.argv[2],"w"))' \
  "$SPEC" "$spec_json"

# The library version is the document's own info.version. One number, decided
# once, upstream — never typed into this repo.
version="$(sed -n 's/^  version: *//p' "$SPEC" | head -1)"
: "${version:?could not read info.version from $SPEC}"

# Validation stays ON: hanzo.yaml validates at 0 errors / 0 warnings, and a
# malformed document must fail here rather than as compile errors spread over
# 2400-odd files.
#
# apiDocs/modelDocs/apiTests/modelTests off: ~4000 files of nothing at this
# surface size, and the docs would be a second, staler copy of the document.
#
# skipFormModel=false: edge_deployFunction offers both application/octet-stream
# and an inline multipart/form-data body. The generator skips a multipart form
# model by default and still imports it from the API class that used it, so
# FunctionsApi would include a header nobody emitted.
#
# -t templates/cpp-restsdk: four overridden templates, each hunk commented in
# place. They correct the generator; they say nothing about this API.
#
java -Xmx3g -jar "$JAR" generate \
  -g cpp-restsdk \
  -i "$spec_json" \
  -o "$out" \
  -t templates/cpp-restsdk \
  --global-property apis,models,supportingFiles,apiDocs=false,modelDocs=false,apiTests=false,modelTests=false,skipFormModel=false \
  --additional-properties "packageName=hanzo,apiPackage=hanzo.api,modelPackage=hanzo.model,packageVersion=$version" \
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
  [ "$rc" = 0 ] && echo "clean: include/ and src/ are what hanzo.yaml projects"
  exit "$rc"
fi

# Replaced wholesale, so a renamed or dropped operation cannot leave a stale
# header behind.
rm -rf include src
cp -r "$out"/include "$out"/src .

echo "generated $(find include -name '*.h' | wc -l) headers and $(find src -name '*.cpp' | wc -l) sources"
echo "  spec      ${SPEC_REPO}@${SPEC_REF}  (info.version $version)"
echo "  generator openapi-generator $GENERATOR_VERSION, cpp-restsdk, templates/cpp-restsdk"
