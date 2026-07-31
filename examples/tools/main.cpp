// tools — list the MCP tools this key can reach.
//
// Operation: mcp_rpc (POST /v1/mcp), method=tools/list.
//
// POST /v1/mcp is the fleet's one MCP door: it composes the typed product
// operations with the external MCP servers the caller's org has enabled. It is
// IN the document, which is what makes this a generated call rather than a
// hand-rolled HTTP request inside a generated client — the exact drift these
// SDKs exist to prevent. (GET /v1/mcp is 404; one verb is never a liveness
// probe.)
//
// JSON-RPC reports failure INSIDE a 200, so `error` is read before `result`.
//
//   HANZO_API_KEY=hk-... ./build/examples/tools
#include <iostream>
#include <variant>

#include "hanzo.h"
#include "hanzo/api/MCPApi.h"

int main() {
    using namespace hanzo;
    try {
        auto request = std::make_shared<model::Mcp_Request>();
        request->setJsonrpc(model::Mcp_Request::JsonrpcEnum::_2_0);
        request->setId(U("1"));
        request->setMethod(model::Mcp_Request::MethodEnum::TOOLS_LIST);

        api::MCPApi mcp(examples::client());
        std::shared_ptr<model::Mcp_Response> response = mcp.mcpRpc(request).get();

        // A JSON-RPC error arrives with HTTP 200, so this is the first thing
        // read — not the last.
        if (response->errorIsSet() && response->getError()) {
            const auto failure = response->getError();
            std::cerr << "mcp error " << failure->getCode() << ": "
                      << utility::conversions::to_utf8string(failure->getMessage())
                      << std::endl;
            return 1;
        }

        const auto result = response->getResult();
        if (!result) {
            std::cerr << "mcp returned neither result nor error" << std::endl;
            return 1;
        }

        // result is a oneOf: a catalogue for tools/list, an outcome for
        // tools/call, server info for initialize.
        const auto* catalog = std::get_if<model::Mcp_Catalog>(&result->getVariant());
        if (catalog == nullptr) {
            std::cerr << "tools/list did not answer with a catalogue: "
                      << utility::conversions::to_utf8string(result->toJson().serialize())
                      << std::endl;
            return 1;
        }

        const auto tools = catalog->getTools();
        if (tools.empty()) {
            std::cerr << "the catalogue is empty" << std::endl;
            return 1;
        }

        examples::print(U("tools  "), static_cast<int64_t>(tools.size()));
        for (std::size_t i = 0; i < tools.size() && i < 3; ++i) {
            examples::print(U("  "), tools[i]->getName());
        }
    } catch (const std::exception& e) {
        return examples::fail(U("mcp"), e);
    }
    return 0;
}
