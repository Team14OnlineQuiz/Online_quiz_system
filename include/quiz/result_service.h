#pragma once
#include "account_store.h"
#include <ostream>
namespace quiz {
struct AttemptRecord {
    Id id;
    Id userId;
    std::string quizTitle;
    std::string submittedAt;
    std::uint32_t correctCount;
    std::uint32_t totalQuestions;
    bool completed;
};
class AttemptStore {
public:
    virtual ~AttemptStore() = default;
    virtual const std::vector<AttemptRecord>& attempts() const = 0;
    virtual Result<Id> append(AttemptRecord attempt) {
        (void)attempt;
        return failure<Id>(Error::Storage);
    }
};
class FileAttemptStore final : public AttemptStore {
public:
    explicit FileAttemptStore(std::filesystem::path directory);
    void reload();
    const std::vector<AttemptRecord>& attempts() const override { return attempts_; }
    Result<Id> append(AttemptRecord attempt) override;
private:
    std::filesystem::path file_;
    std::vector<AttemptRecord> attempts_;
};
struct SummaryRow {
    std::string username;
    std::string quizTitle;
    std::string submittedAt;
    std::uint32_t correctCount;
    std::uint32_t totalQuestions;
    double percentage;
};
class ResultService {
public:
    ResultService(const AccountStore& accounts, const AttemptStore& attempts)
        : accounts_(accounts), attempts_(attempts) {}
    Result<std::vector<SummaryRow>> summary(const Session& session) const;
    Error displaySummary(const Session& session, std::ostream& output) const;
private:
    const AccountStore& accounts_;
    const AttemptStore& attempts_;
};
bool validSubmissionDate(const std::string& value);
}
