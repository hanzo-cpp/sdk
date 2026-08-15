// hello — prove the key works, and print the verdict the API returns.
//
// Operation: get_ai_account (GET /v1/ai/account), on AiApi.
//
// The route answers with an Envelope, and the document is explicit that `status`
// is the verdict and not the HTTP code: "a handled failure is still 200". So a
// hello that only caught exceptions would print success on a refusal. Both are
// checked here — the transport failure as a catch, the handled one as a status —
// because that is the shape every other call in this client has.
//
//   HANZO_API_KEY=hk-... ./build/examples/hello
#include "hanzo.h"
#include "hanzo/api/AiApi.h"

int main() {
    using namespace hanzo;
    try {
        api::AiApi ai(examples::client());
        std::shared_ptr<model::Envelope> res = ai.getAiAccount().get();

        examples::print(U("status "), res->fromStatusEnum(res->getStatus()));
        if (res->getStatus() != model::Envelope::StatusEnum::OK) {
            examples::print(U("reason "), res->getMsg());
            return 1;
        }
        examples::print(U("account"), res->getData()->toJson().serialize());
    } catch (const std::exception& e) {
        return examples::fail(U("whoami"), e);
    }
    return 0;
}
