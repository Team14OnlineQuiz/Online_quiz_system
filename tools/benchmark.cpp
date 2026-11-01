#include "quiz/auth_service.h"
#include "quiz/console.h"
#include "quiz/question_catalog.h"
#include "quiz/result_service.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

using Clock = std::chrono::steady_clock;
static double milliseconds(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}
static double percentile95(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    return values[(values.size() * 95 + 99) / 100 - 1];
}
int main() {
    try {
        const auto data = std::filesystem::path("build") / ("benchmark-data-"
            + std::to_string(Clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(data);
        struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code e; std::filesystem::remove_all(path, e); } } cleanup{data};
        quiz::PasswordHasher hasher;
        quiz::FileAccountStore accounts(data);
        quiz::AuthService auth(accounts, hasher);
        if (!auth.provisionFirstAdmin("owner", "benchmark-only-password")) throw std::runtime_error("Benchmark setup failed");
        const auto learner = auth.registerLearner("learner", "benchmark-only-password");
        if (!learner) throw std::runtime_error("Benchmark account setup failed");
        {
            std::ofstream questions(data / "questions.db");
            questions << "QUIZ_QUESTIONS_V1\n";
            for (int i = 1; i <= 1000; ++i)
                questions << i << " 1 A \"Question " << i << "?\" \"one\" \"two\" \"three\" \"four\"\n";
            if (!questions) throw std::runtime_error("Question fixture write failed");
        }
        {
            std::ofstream attempts(data / "attempts.db");
            attempts << "QUIZ_ATTEMPTS_V1\n";
            for (int i = 1; i <= 10000; ++i)
                attempts << i << ' ' << *learner.value << " 1 2 3 \"Benchmark quiz\" \"2026-10-20T10:00:00Z\"\n";
            if (!attempts) throw std::runtime_error("Attempt fixture write failed");
        }
        quiz::QuestionCatalog catalog(data);
        quiz::FileAttemptStore attempts(data);
        quiz::ResultService results(accounts, attempts);
        if (catalog.size() != 1000 || attempts.attempts().size() != 10000) throw std::runtime_error("Dataset count mismatch");
        quiz::Console console(auth, {}, [&](const quiz::Session& session, std::istream&, std::ostream& output) {
            attempts.reload();
            if (results.displaySummary(session, output) != quiz::Error::None) throw std::runtime_error("Summary failed");
        });
        std::ofstream csv("build/performance.csv");
        csv << "operation,sample,milliseconds\n";
        std::vector<double> menuTimes, questionTimes;
        for (int i = 0; i < 40; ++i) {
            std::istringstream menuInput("2\nowner\nbenchmark-only-password\n1\n2\n3\n");
            std::ostringstream menuOutput;
            auto start = Clock::now();
            if (console.run(menuInput, menuOutput) != 0) throw std::runtime_error("Menu transition failed");
            menuTimes.push_back(milliseconds(start));
            if (menuOutput.str().find("2/3 | 66.7%") == std::string::npos) throw std::runtime_error("Summary was not rendered");
            std::istringstream answer("A\n");
            std::ostringstream questionOutput;
            start = Clock::now();
            const auto* prompt = catalog.find(static_cast<quiz::Id>((i * 29) % 1000 + 1));
            if (!prompt) throw std::runtime_error("Question missing");
            quiz::AnswerSession attempt({*prompt});
            if (!attempt.readNext(answer, questionOutput) || !attempt.complete()) throw std::runtime_error("Question load failed");
            questionTimes.push_back(milliseconds(start));
            csv << "menu," << i + 1 << ',' << std::setprecision(9) << menuTimes.back() << '\n';
            csv << "question," << i + 1 << ',' << questionTimes.back() << '\n';
        }
        csv.flush();
        if (!csv) throw std::runtime_error("Measurement report write failed");
        const auto menu95 = percentile95(menuTimes), question95 = percentile95(questionTimes);
        std::cout << "NFR-01: questions=1000 attempts=10000 samples_per_operation=40\n"
            << "Compiler: " << __VERSION__ << '\n'
#ifdef _WIN32
            << "OS: Windows\n"
#else
            << "OS: Linux\n"
#endif
            << "menu_p95_ms=" << menu95 << " question_p95_ms=" << question95 << '\n'
            << "Raw measurements: build/performance.csv\n";
        return menu95 <= 2000.0 && question95 <= 2000.0 ? 0 : 1;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
