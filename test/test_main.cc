#define DROGON_TEST_MAIN
#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include <thread>
#include <future>

// Include helpers under test
#include "../include/helpers/HtmlEscape.h"
#include "../include/helpers/Pagination.h"
#include "../include/helpers/CiLike.h"
#include "../include/RouteRegistry.h"

// ── Unit: HtmlEscape ──────────────────────────────────────────────────────────
DROGON_TEST(HtmlEscapeBasic) {
    CHECK(h("<script>") == "&lt;script&gt;");
    CHECK(h("a & b")    == "a &amp; b");
    CHECK(h("\"q\"")    == "&quot;q&quot;");
    CHECK(h("it's")     == "it&#39;s");
}

// ── Unit: Pagination ─────────────────────────────────────────────────────────
DROGON_TEST(PaginationMeta) {
    auto m = makePaginateMeta(100, 1, 10);
    CHECK(m.totalPages == 10);
    CHECK(!m.hasPrev);
    CHECK(m.hasNext);

    auto m2 = makePaginateMeta(100, 10, 10);
    CHECK(m2.hasPrev);
    CHECK(!m2.hasNext);

    auto m3 = makePaginateMeta(540, 27, 10);
    auto nums = m3.pageNumbers();
    CHECK(nums.front() == 1);
    CHECK(nums.back() == 54);
    bool hasEllipsis = false;
    for (int n : nums) if (n == -1) { hasEllipsis = true; break; }
    CHECK(hasEllipsis);
}

// ── Unit: CiLike ─────────────────────────────────────────────────────────────
DROGON_TEST(CiLikeHelper) {
    auto [sql, val] = ciLike("u.name", "alice");
    CHECK(sql == "LOWER(u.name) LIKE LOWER(?)");
    CHECK(val == "%alice%");
}

// ── Unit: RouteRegistry ───────────────────────────────────────────────────────
DROGON_TEST(RouteRegistryLookup) {
    auto &reg = RouteRegistry::instance();
    reg.add("admin.v1.access.user.index", "GET", "/admin/v1/access/user");
    reg.add("admin.v1.access.user.delete", "DELETE", "/admin/v1/access/user/{id}/delete");

    CHECK(reg.nameByMethodAndPath("GET", "/admin/v1/access/user")
          == "admin.v1.access.user.index");
    CHECK(reg.nameByMethodAndPath("DELETE", "/admin/v1/access/user/abc-123/delete")
          == "admin.v1.access.user.delete");
    CHECK(reg.nameByMethodAndPath("POST", "/admin/v1/access/user").empty());
}

int main(int argc, char **argv) {
    using namespace drogon;

    std::promise<void> p1;
    std::future<void>  f1 = p1.get_future();

    std::thread thr([&]() {
        app().getLoop()->queueInLoop([&p1]() { p1.set_value(); });
        app().run();
    });

    f1.get();
    int status = test::run(argc, argv);

    app().getLoop()->queueInLoop([]() { app().quit(); });
    thr.join();
    return status;
}
