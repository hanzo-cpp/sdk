// The one place the six examples resolve their environment.
//
// HANZO_API_KEY is required, HANZO_BASE_URL defaults to https://api.hanzo.ai,
// and HANZO_ORG_ID carries the org that the KV and agent routes refuse to
// answer without. Every flow calls client(); none of them reads getenv itself.
#ifndef HANZO_EXAMPLES_H_
#define HANZO_EXAMPLES_H_

#include <memory>
#include <string>

#include "hanzo/ApiClient.h"

namespace hanzo {
namespace examples {

// A client authenticated as HANZO_API_KEY against HANZO_BASE_URL.
// Throws std::runtime_error when the key is missing.
std::shared_ptr<api::ApiClient> client();

// https://api.hanzo.ai unless HANZO_BASE_URL says otherwise.
utility::string_t baseUrl();

// The model the chat and agent flows ask for; HANZO_MODEL overrides.
utility::string_t model();

// Print one labelled line. Every flow reports through this, so the six read the
// same way whichever language a reader arrives from.
void print(const utility::string_t& label, const utility::string_t& value);
void print(const utility::string_t& label, int64_t value);

// Turn any exception into the process's exit code, printing an ApiException's
// status and body rather than just its what().
int fail(const utility::string_t& what, const std::exception& e);

}  // namespace examples
}  // namespace hanzo

#endif  // HANZO_EXAMPLES_H_
