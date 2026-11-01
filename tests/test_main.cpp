#include "test.h"
#include "quiz/types.h"
#include <iostream>
TEST(default_session_has_no_privileges) {
    quiz::Session session;
    REQUIRE(!session.authenticated());
    REQUIRE(!session.isAdmin());
}
int main() {
    int failures = 0;
    for (const auto& item : test::cases()) {
        try { item.second(); std::cout << "PASS " << item.first << '\n'; }
        catch (const std::exception& e) { ++failures; std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; }
    }
    std::cout << test::cases().size() << " tests, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
