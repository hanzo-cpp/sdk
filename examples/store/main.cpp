// store — KV round-trip: create a store, read it back, delete it.
//
// Operations: postKv        (POST   /v1/kv)
//             getKvByName   (GET    /v1/kv/{name})
//             deleteKvByName (DELETE /v1/kv/{name})
//
// The delete is in a scope guard, so a failed read still cleans up instead of
// leaving the store behind for the next run to collide with.
//
// This is the PROVISIONING plane, and it is the one that answers. The value
// plane (/v1/kv/keys/{key}) is authored in the document and mounted nowhere:
// auth middleware runs before the handler, so a route that exists refuses with
// 403 and one that does not is 404 — /v1/kv, /v1/kv/demo and /v1/kv/keys all
// answer 403 "X-Org-Id required" while /v1/kv/keys/x answers 404.
//
//   HANZO_API_KEY=hk-... HANZO_ORG_ID=... ./build/examples/store
#include <ctime>
#include <iostream>
#include <string>

#include "hanzo.h"
#include "hanzo/api/KvApi.h"

namespace {

// Runs must not collide, so the name is unique per run rather than fixed.
utility::string_t uniqueName() {
    return utility::conversions::to_string_t("hanzo-cpp-example-" +
                                             std::to_string(std::time(nullptr)));
}

// Delete on the way out, however we leave.
class Cleanup {
  public:
    Cleanup(const hanzo::api::KvApi& kv, utility::string_t name)
        : m_kv(kv), m_name(std::move(name)) {}
    ~Cleanup() {
        try {
            m_kv.deleteKvByName(m_name).get();
            hanzo::examples::print(U("deleted "), m_name);
        } catch (const std::exception& e) {
            std::cerr << "delete failed: " << e.what() << std::endl;
        }
    }

  private:
    const hanzo::api::KvApi& m_kv;
    utility::string_t m_name;
};

}  // namespace

int main() {
    using namespace hanzo;
    api::KvApi kv(examples::client());
    const utility::string_t name = uniqueName();

    try {
        auto request = std::make_shared<model::ProvisionRequest>();
        request->setName(name);

        std::shared_ptr<model::ProvisionResult> created =
            kv.postKv(request).get();
        examples::print(U("created "), created->getName());
        examples::print(U("kind    "), created->getKind());
        examples::print(U("status  "), created->getStatus());

        Cleanup cleanup(kv, name);

        std::shared_ptr<model::ProvisionedResource> read =
            kv.getKvByName(name).get();
        examples::print(U("read    "), read->getName());
        examples::print(U("host    "), read->getHost());
        examples::print(U("port    "), static_cast<int64_t>(read->getPort()));
    } catch (const std::exception& e) {
        return examples::fail(U("kv"), e);
    }
    return 0;
}
