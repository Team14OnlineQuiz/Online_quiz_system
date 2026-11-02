#include "quiz/result_service.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
namespace quiz {
bool validSubmissionDate(const std::string& value) {
    if (value.size() != 20 || value[4] != '-' || value[7] != '-' || value[10] != 'T'
        || value[13] != ':' || value[16] != ':' || value[19] != 'Z') return false;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16 || i == 19) continue;
        if (!std::isdigit(static_cast<unsigned char>(value[i]))) return false;
    }
    const int year = std::stoi(value.substr(0, 4)), month = std::stoi(value.substr(5, 2));
    const int day = std::stoi(value.substr(8, 2));
    if (year == 0 || month < 1 || month > 12) return false;
    int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) days[1] = 29;
    return day >= 1 && day <= days[month - 1] && std::stoi(value.substr(11, 2)) <= 23
        && std::stoi(value.substr(14, 2)) <= 59 && std::stoi(value.substr(17, 2)) <= 59;
}
FileAttemptStore::FileAttemptStore(std::filesystem::path directory) : file_(std::move(directory) / "attempts.db") { reload(); }
void FileAttemptStore::reload() {
    if (!std::filesystem::exists(file_)) { attempts_.clear(); return; }
    std::ifstream input(file_);
    if (!input) throw std::runtime_error("Cannot read attempts.db; repair the data directory");
    std::string line;
    if (!std::getline(input, line) || line != "QUIZ_ATTEMPTS_V1") throw std::runtime_error("Invalid attempts.db header");
    std::vector<AttemptRecord> staged;
    std::unordered_set<Id> ids;
    while (std::getline(input, line)) {
        if (line.size() > 4096) throw std::runtime_error("Oversized attempt record");
        long long id = 0, owner = 0, correct = 0, total = 0;
        int completed = -1;
        std::string title, date;
        std::istringstream record(line);
        if (!(record >> id >> owner >> completed >> correct >> total >> std::quoted(title) >> std::quoted(date)))
            throw std::runtime_error("Malformed attempt record; restore a valid backup");
        record >> std::ws;
        if (!record.eof() || id <= 0 || owner <= 0 || (completed != 0 && completed != 1)
            || correct < 0 || total <= 0 || correct > total || total > std::numeric_limits<std::uint32_t>::max()
            || title.empty() || title.size() > 512 || !validSubmissionDate(date)
            || std::any_of(title.begin(), title.end(), [](unsigned char c) { return c < 32 || c == 127; })
            || !ids.insert(static_cast<Id>(id)).second)
            throw std::runtime_error("Invalid attempt record; restore a valid backup");
        staged.push_back({static_cast<Id>(id), static_cast<Id>(owner), std::move(title), std::move(date),
            static_cast<std::uint32_t>(correct), static_cast<std::uint32_t>(total), completed == 1});
    }
    if (input.bad()) throw std::runtime_error("Attempt read failed");
    attempts_.swap(staged);
}
Result<std::vector<SummaryRow>> ResultService::summary(const Session& session) const {
    if (!session.isAdmin()) return failure<std::vector<SummaryRow>>(Error::AccessDenied);
    std::unordered_map<Id, std::string> usernames;
    for (const auto& user : accounts_.users()) usernames.emplace(user.id, user.username);
    std::vector<SummaryRow> rows;
    for (const auto& attempt : attempts_.attempts()) {
        const auto owner = usernames.find(attempt.userId);
        if (owner == usernames.end() || attempt.id == 0
            || attempt.totalQuestions == 0
            || attempt.correctCount > attempt.totalQuestions
            || attempt.quizTitle.empty() || attempt.quizTitle.size() > 512
            || !validSubmissionDate(attempt.submittedAt))
            return failure<std::vector<SummaryRow>>(Error::Storage);

        if (!attempt.completed) continue;
        rows.push_back({owner->second, attempt.quizTitle, attempt.submittedAt, attempt.correctCount,
            attempt.totalQuestions, 100.0 * attempt.correctCount / attempt.totalQuestions});
    }
    return success(std::move(rows));
}
Error ResultService::displaySummary(const Session& session, std::ostream& output) const {
    const auto result = summary(session);
    if (!result) {
        output << (result.error == Error::AccessDenied ? "Access denied.\n" : "Results unavailable; repair saved data.\n");
        return result.error;
    }
    if (result.value->empty()) { output << "No completed attempts.\n"; return Error::None; }
    const auto flags = output.flags();
    const auto precision = output.precision();
    output << "Username | Quiz title | Date (UTC) | Score | Percentage\n";
    for (const auto& row : *result.value)
        output << std::quoted(row.username) << " | " << std::quoted(row.quizTitle) << " | " << row.submittedAt
            << " | " << row.correctCount << '/' << row.totalQuestions << " | "
            << std::fixed << std::setprecision(1) << row.percentage << "%\n";
    output.flags(flags); output.precision(precision);
    return Error::None;
}
}
