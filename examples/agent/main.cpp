// agent — create an agent, run it, read the run back.
//
// Operations: cloud_post_v1_agents           (POST /v1/agents)
//             cloud_post_v1_agents_by_ref_run (POST /v1/agents/{ref}/run)
//             cloud_get_v1_agents_ref_runs    (GET  /v1/agents/{ref}/runs)
//
// `ref` takes the public id (agent_...) OR the org-unique name, which is why
// run and read can both use the name just created without waiting for an id.
// Names are org-unique, so this must not hardcode one.
//
// The last step is the RUN LIST, not the agent read: a run is asynchronous, so
// the example polls until the run it started is terminal.
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

        auto create = std::make_shared<model::Cloud_createAgentIn>();
        create->setName(name);
        create->setModel(examples::model());
        create->setInstructions(U("Answer in one short sentence."));

        std::shared_ptr<model::Cloud_agentView> agent =
            agents.cloudPostV1Agents(create).get();
        examples::print(U("agent   "), agent->getName());
        examples::print(U("id      "), agent->getId());

        auto input = std::make_shared<model::Agents_RunRequest>();
        input->setInput(U("What is the capital of Japan?"));

        std::shared_ptr<model::Agents_RunView> run =
            agents.cloudPostV1AgentsByRefRun(name, input).get();
        examples::print(U("run     "), run->getId());
        examples::print(U("status  "), run->fromStatusEnum(run->getStatus()));

        // Poll the run list until the run just started reaches a terminal state.
        for (int attempt = 0; attempt < 10; ++attempt) {
            std::shared_ptr<model::Cloud_runList> runs =
                agents.cloudGetV1AgentsRefRuns(name, 10).get();

            for (const auto& seen : runs->getRuns()) {
                if (seen->getId() != run->getId()) {
                    continue;
                }
                examples::print(U("polled  "), seen->getStatus());
                if (terminal(seen->getStatus())) {
                    examples::print(U("output  "), seen->getOutput());
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
