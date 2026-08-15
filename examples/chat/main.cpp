// chat — ask one question, print the grounded answer and what it cited.
//
// Operation: research_web (POST /v1/ask/web), on AskApi.
//
// NOT /v1/chat/completions. That route is real and this client carries it, but
// the document declares it with no request body and no response schema, so the
// generated method takes nothing and returns nothing — there is no typed way to
// send a message or read a reply through it. A generated client can only be as
// typed as the document, and an example has to demonstrate a call that works.
// /v1/ask/web is the typed one: a question in, an answer with its sources out.
//
// Non-streaming on purpose: streaming is a different transport (SSE) that a
// generated client returns as an opaque body, so demonstrating it here would
// teach the wrong shape.
//
//   HANZO_API_KEY=hk-... ./build/examples/chat
#include <iostream>

#include "hanzo.h"
#include "hanzo/api/AskApi.h"

int main() {
    using namespace hanzo;
    try {
        auto question = std::make_shared<model::WebQuestion>();
        question->setQ(U("Name the three laws of thermodynamics in one line each."));
        question->setMode(U("search"));
        question->setMaxSources(3);

        api::AskApi ask(examples::client());
        std::shared_ptr<model::Report> report = ask.researchWeb(question).get();

        examples::print(U("model  "), report->getModel());
        examples::print(U("answer "), report->getAnswer());

        // Every citation is a page the call fetched, so the sources are part of
        // the answer rather than decoration.
        for (const auto& source : report->getSources()) {
            examples::print(U("source "), source->getUrl());
        }
    } catch (const std::exception& e) {
        return examples::fail(U("chat"), e);
    }
    return 0;
}
