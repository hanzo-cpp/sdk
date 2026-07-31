// money — balance, then the usage that moved it.
//
// Operations: cloud_get_v1_billing_balance (GET /v1/billing/balance)
//             cloud_get_v1_billing_usage   (GET /v1/billing/usage)
//
// Neither takes an org: both derive the tenant server-side from the JWT `owner`
// claim, so a key can only ever read its own money.
//
// The window parameters are real. /v1/billing/usage takes `start` and `end`,
// and both — along with the typed response bodies read below — were restored to
// the document after a merge defect had been replacing the authored operation
// with an undescribed one. An SDK reads a typed body here; it does not decode
// by hand.
//
//   HANZO_API_KEY=hk-... ./build/examples/money
#include <ctime>

#include "hanzo.h"
#include "hanzo/api/BillingApi.h"

namespace {

// RFC 3339, `days` ago. The window is a week so the ledger has something in it.
utility::string_t daysAgo(int days) {
    const std::time_t when = std::time(nullptr) - static_cast<std::time_t>(days) * 86400;
    std::tm utc{};
    gmtime_r(&when, &utc);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return utility::conversions::to_string_t(buf);
}

}  // namespace

int main() {
    using namespace hanzo;
    try {
        api::BillingApi billing(examples::client());

        std::shared_ptr<model::Billing_Balance> balance =
            billing.cloudGetV1BillingBalance(boost::none).get();
        examples::print(U("balance  "), balance->getBalance());
        examples::print(U("holds    "), balance->getHolds());
        examples::print(U("available"), balance->getAvailable());

        std::shared_ptr<model::Billing_UsageLedger> ledger =
            billing.cloudGetV1BillingUsage(daysAgo(7), daysAgo(0)).get();

        const auto records = ledger->getUsage();
        examples::print(U("usage    "), static_cast<int64_t>(records.size()));
        for (std::size_t i = 0; i < records.size() && i < 5; ++i) {
            examples::print(U("  ") + records[i]->getCreatedAt(),
                            records[i]->getAmount());
        }
    } catch (const std::exception& e) {
        return examples::fail(U("billing"), e);
    }
    return 0;
}
