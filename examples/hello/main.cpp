// hello — prove the key works, and print who it belongs to.
//
// Operation: bot_authMe (GET /v1/bot/auth/me).
//
// This is the call that SAYS NO. With no key, or a bogus one, the route answers
// 403 {"error":"no validated principal"} rather than a cheerful anonymous
// identity — which is why it, and not ai_getAccount, is what a hello is built
// on: /v1/ai/account answers 200 with type="anonymous-user" to a request
// carrying no key at all. The refusal is the point, so the failure is printed
// as deliberately as the success.
//
//   HANZO_API_KEY=hk-... ./build/examples/hello
#include "hanzo.h"
#include "hanzo/api/AuthApi.h"

int main() {
    using namespace hanzo;
    try {
        api::AuthApi auth(examples::client());
        std::shared_ptr<model::Bot_User> me = auth.botAuthMe().get();

        examples::print(U("id     "), me->getId());
        examples::print(U("handle "), me->getHandle());
        examples::print(U("name   "), me->getDisplayName());
        examples::print(U("email  "), me->getEmail());
        examples::print(U("role   "), me->fromRoleEnum(me->getRole()));
    } catch (const std::exception& e) {
        return examples::fail(U("whoami"), e);
    }
    return 0;
}
