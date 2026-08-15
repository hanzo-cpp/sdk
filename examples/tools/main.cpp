// tools — list the MCP tools this key can reach.
//
// Operation: aiMCPTools (GET /v1/ai/mcp/tools), on AiApi.
//
// The surface answers with the tool COUNT, the apps that publish them, and —
// when `names` is asked for — every tool name. It is IN the document, which is
// what makes this a generated call rather than a hand-rolled HTTP request
// inside a generated client: the exact drift these SDKs exist to prevent.
//
//   HANZO_API_KEY=hk-... ./build/examples/tools
#include <iostream>

#include "hanzo.h"
#include "hanzo/api/AiApi.h"

int main() {
    using namespace hanzo;
    try {
        api::AiApi ai(examples::client());

        // `names` off returns the count and the apps only, so it is asked for.
        std::shared_ptr<model::AiMCPSurface> surface =
            ai.aiMCPTools(boost::optional<bool>(true)).get();

        const auto names = surface->getNames();
        examples::print(U("tools  "), static_cast<int64_t>(surface->getTools()));
        examples::print(U("apps   "), static_cast<int64_t>(surface->getApps().size()));

        if (names.empty()) {
            std::cerr << "the surface named no tools" << std::endl;
            return 1;
        }
        for (std::size_t i = 0; i < names.size() && i < 3; ++i) {
            examples::print(U("  "), names[i]);
        }
    } catch (const std::exception& e) {
        return examples::fail(U("mcp"), e);
    }
    return 0;
}
