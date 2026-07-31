#include "hanzo.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "hanzo/ApiConfiguration.h"
#include "hanzo/ApiException.h"

namespace hanzo {
namespace examples {
namespace {

utility::string_t env(const char* name, const utility::string_t& fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }
    return utility::conversions::to_string_t(value);
}

}  // namespace

utility::string_t baseUrl() {
    return env("HANZO_BASE_URL", utility::conversions::to_string_t("https://api.hanzo.ai"));
}

utility::string_t model() {
    return env("HANZO_MODEL", utility::conversions::to_string_t("zen-1"));
}

std::shared_ptr<api::ApiClient> client() {
    const char* key = std::getenv("HANZO_API_KEY");
    if (key == nullptr || *key == '\0') {
        throw std::runtime_error("HANZO_API_KEY is not set");
    }

    auto config = std::make_shared<api::ApiConfiguration>();
    config->setBaseUrl(baseUrl());
    config->setUserAgent(utility::conversions::to_string_t("hanzo-cpp-sdk/examples"));

    // cpp-restsdk emits header handling for apiKey schemes only, so an http
    // bearer scheme — which is what hanzo.yaml declares globally — has to be
    // set as a default header. Once, here, rather than in six flows.
    config->getDefaultHeaders()[utility::conversions::to_string_t("Authorization")] =
        utility::conversions::to_string_t("Bearer ") + utility::conversions::to_string_t(key);

    // Harmless where the tenant comes from the token, required where it does
    // not: /v1/kv and /v1/agents answer 403 "X-Org-Id required" without it.
    const utility::string_t org = env("HANZO_ORG_ID", utility::string_t());
    if (!org.empty()) {
        config->getDefaultHeaders()[utility::conversions::to_string_t("X-Org-Id")] = org;
    }

    return std::make_shared<api::ApiClient>(config);
}

void print(const utility::string_t& label, const utility::string_t& value) {
    std::cout << utility::conversions::to_utf8string(label) << "  "
              << utility::conversions::to_utf8string(value) << std::endl;
}

void print(const utility::string_t& label, int64_t value) {
    std::cout << utility::conversions::to_utf8string(label) << "  " << value << std::endl;
}

int fail(const utility::string_t& what, const std::exception& e) {
    std::cerr << utility::conversions::to_utf8string(what) << " failed: " << e.what() << std::endl;
    if (const auto* apiError = dynamic_cast<const api::ApiException*>(&e)) {
        if (auto body = apiError->getContent()) {
            std::stringstream text;
            text << body->rdbuf();
            std::cerr << "  body: " << text.str() << std::endl;
        }
    }
    return 1;
}

}  // namespace examples
}  // namespace hanzo
