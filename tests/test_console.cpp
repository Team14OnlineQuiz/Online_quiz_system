#include "test.h"
#include "temp_directory.h"
#include "quiz/console.h"
#include <sstream>
TEST(console_registration_signin_and_logout_use_real_saved_accounts) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    quiz::AuthService auth(store, hasher);
    std::istringstream input("1\nalex\nlearner-password\n2\nalex\nlearner-password\n2\n3\n");
    std::ostringstream output;
    quiz::Console console(auth);
    REQUIRE(console.run(input, output) == 0);
    REQUIRE(output.str().find("Learner account saved.") != std::string::npos);
    REQUIRE(output.str().find("Learner menu") != std::string::npos);
    REQUIRE(output.str().find("Administrator menu") == std::string::npos);
    REQUIRE(output.str().find("learner-password") == std::string::npos);
    quiz::FileAccountStore restarted(temp.path);
    REQUIRE(restarted.find("alex") != nullptr);
}
TEST(invalid_credentials_never_display_authenticated_menus) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    quiz::AuthService auth(store, hasher);
    REQUIRE(auth.registerLearner("alex", "learner-password"));
    quiz::Console console(auth);
    for (const auto& credentials : {"missing\nlearner-password\n", "alex\nwrong-password\n"}) {
        std::istringstream input(std::string("2\n") + credentials + "3\n");
        std::ostringstream output;
        REQUIRE(console.run(input, output) == 0);
        REQUIRE(output.str().find("Invalid credentials.") != std::string::npos);
        REQUIRE(output.str().find("Learner menu") == std::string::npos);
        REQUIRE(output.str().find("Administrator menu") == std::string::npos);
    }
}
TEST(admin_actions_receive_an_authenticated_admin_session) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    quiz::AuthService auth(store, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    bool invoked = false;
    quiz::Console console(auth, {}, [&](const quiz::Session& s, std::istream&, std::ostream&) {
        REQUIRE(s.isAdmin()); invoked = true;
    });
    std::istringstream input("2\nowner\nadmin-password\n1\n2\n3\n");
    std::ostringstream output;
    REQUIRE(console.run(input, output) == 0);
    REQUIRE(invoked);
}
TEST(oversized_input_is_drained_and_eof_exits) {
    std::istringstream input(std::string(100000, 'x') + "\nA\n");
    REQUIRE(quiz::readLine(input, 16).state == quiz::InputState::Invalid);
    REQUIRE(quiz::readLine(input, 16).value == "A");
    REQUIRE(quiz::readLine(input, 16).state == quiz::InputState::End);
}
