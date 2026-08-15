// money — what is left, then the usage that spent it.
//
// Operations: get_finance_credits (GET /v1/finance/credits), on FinanceApi
//             get_finance_usage   (GET /v1/finance/usage)
//
// Neither takes an org: both derive the tenant server-side from the JWT `owner`
// claim, so a key can only ever read its own money.
//
// NOT /v1/billing/balance and /v1/billing/usage. Those routes exist and this
// client carries them, but the document declares them with no parameters and no
// response schema, so the generated methods take nothing and return nothing.
// The /v1/finance pair is the typed one — a credit list with cents remaining,
// and a usage view whose window is a real parameter. An SDK reads a typed body
// here; it does not decode by hand.
//
//   HANZO_API_KEY=hk-... ./build/examples/money
#include "hanzo.h"
#include "hanzo/api/FinanceApi.h"

int main() {
    using namespace hanzo;
    try {
        api::FinanceApi finance(examples::client());

        const auto credits = finance.getFinanceCredits().get();
        int64_t remaining = 0;
        for (const auto& credit : credits) {
            remaining += credit->getRemainingCents();
        }
        examples::print(U("credits  "), static_cast<int64_t>(credits.size()));
        examples::print(U("remaining"), remaining);

        // The window: 24h, 7d, 30d or 90d. Anything else, absent included, is
        // 30d — so it is stated rather than left to the default.
        std::shared_ptr<model::FinanceUsageView> usage =
            finance.getFinanceUsage(boost::optional<utility::string_t>(U("7d"))).get();

        examples::print(U("from     "), usage->getStart());
        examples::print(U("to       "), usage->getEnd());
        examples::print(U("spent    "), static_cast<int64_t>(usage->getTotalCents()));

        const auto lines = usage->getLines();
        for (std::size_t i = 0; i < lines.size() && i < 5; ++i) {
            examples::print(U("  ") + lines[i]->getLabel(),
                            static_cast<int64_t>(lines[i]->getCents()));
        }
    } catch (const std::exception& e) {
        return examples::fail(U("finance"), e);
    }
    return 0;
}
