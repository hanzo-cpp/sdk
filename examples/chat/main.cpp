// chat — one completion.
//
// Operation: ai_createChatCompletion (POST /v1/chat/completions).
//
// OpenAI-compatible request and response. Non-streaming on purpose: streaming
// is a different transport (SSE) that a generated client returns as an opaque
// body, so demonstrating it here would teach the wrong shape.
//
//   HANZO_API_KEY=hk-... ./build/examples/chat
#include <iostream>

#include "hanzo.h"
#include "hanzo/api/OpenAICompatibleApi.h"

int main() {
    using namespace hanzo;
    try {
        auto content = std::make_shared<model::AnyType>();
        content->fromJson(web::json::value::string(
            U("Name the three laws of thermodynamics in one line each.")));

        auto message = std::make_shared<model::Ai_ChatMessage>();
        message->setRole(model::Ai_ChatMessage::RoleEnum::USER);
        message->setContent(content);

        auto request = std::make_shared<model::Ai_ChatCompletionRequest>();
        request->setModel(examples::model());
        request->setMessages({message});
        request->setStream(false);

        api::OpenAICompatibleApi ai(examples::client());
        std::shared_ptr<model::Ai_ChatCompletionResponse> answer =
            ai.aiCreateChatCompletion(request).get();

        examples::print(U("model  "), answer->getModel());

        const auto choices = answer->getChoices();
        if (choices.empty()) {
            std::cerr << "no choices in the response" << std::endl;
            return 1;
        }
        // AnyType is a json carrier: content is `string` today and an array of
        // content parts in the multimodal shape, so the example prints what
        // arrived rather than asserting one of them.
        const auto reply = choices.front()->getMessage()->getContent()->toJson();
        examples::print(U("reply  "),
                        reply.is_string() ? reply.as_string() : reply.serialize());
    } catch (const std::exception& e) {
        return examples::fail(U("chat"), e);
    }
    return 0;
}
