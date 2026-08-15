// agent — create an agent, run it, poll the run to a terminal state.
//
// Operations: postAgents          (POST /v1/agents)
//             postAgentsByRefRun  (POST /v1/agents/{ref}/run)
//             getAgentsByRefRuns  (GET  /v1/agents/{ref}/runs)
//
// `ref` takes the public id (agent_...) OR the org-unique name, which is why
// run and read can both use the name just created without waiting for an id.
// Names are org-unique, so this must not hardcode one.
//
// The run POST is declared with no request body and no response, so it starts
// the run and tells you nothing — the input cannot be passed through it and the
// run id does not come back from it. The RUN LIST is the typed one, so that is
// where the id, the status and the output are read, and polling it is how the
// example learns the run finished.
//
//   HANZO_API_KEY=hk-... HANZO_ORG_ID=... ./build/examples/agent
#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

#include "hanzo.h"
#include "hanzo/api/AgentsApi.h"

namespace {

bool terminal(const utility::string_t& status) {
    return status == U("ok") || status == U("error") || status == U("failed") ||
           status == U("succeeded") || status == U("cancelled");
}

}  // namespace

int main() {
    using namespace hanzo;
    try {
        api::AgentsApi agents(examples::client());

        const utility::string_t name = utility::conversions::to_string_t(
            "hanzo-cpp-example-" + std::to_string(std::time(nullptr)));

        auto create = std::make_shared<model::CreateAgentIn>();
        create->setName(name);
        create->setModel(examples::model());
        create->setInstructions(U("Answer in one short sentence."));

        std::shared_ptr<model::AgentView> agent = agents.postAgents(create).get();
        examples::print(U("agent   "), agent->getName());
        examples::print(U("id      "), agent->getId());

        agents.postAgentsByRefRun(name).get();
        examples::print(U("started "), name);

        // Poll the run list until this agent's newest run is terminal.
        for (int attempt = 0; attempt < 10; ++attempt) {
            std::shared_ptr<model::RunList> runs =
                agents.getAgentsByRefRuns(name, boost::optional<int32_t>(10)).get();

            const auto seen = runs->getRuns();
            if (!seen.empty()) {
                const auto& run = seen.front();
                examples::print(U("polled  "), run->getStatus());
                if (terminal(run->getStatus())) {
                    examples::print(U("run     "), run->getId());
                    examples::print(U("output  "), run->getOutput());
                    return 0;
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        std::cerr << "run did not reach a terminal state" << std::endl;
        return 1;
    } catch (const std::exception& e) {
        return examples::fail(U("agent"), e);
    }
}
