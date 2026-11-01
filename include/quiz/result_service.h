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
};
class AttemptWriter {
public:
    virtual ~AttemptWriter() = default;
    virtual Result<Id> insert(AttemptRecord record) = 0;
};
class FileAttemptStore final : public AttemptStore, public AttemptWriter {
public:
    explicit FileAttemptStore(std::filesystem::path directory);
    void reload();
    const std::vector<AttemptRecord>& attempts() const override { return attempts_; }
    Result<Id> insert(AttemptRecord record) override;
private:
    bool persist(const std::vector<AttemptRecord>& staged) const;
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
struct LearnerHistoryRow {
    std::string quizTitle;
    std::string submittedAt;
    std::uint32_t correctCount;
    std::uint32_t totalQuestions;
    double percentage;
};
class LearnerHistoryService {
public:
    LearnerHistoryService(const AccountStore& accounts, const AttemptStore& attempts)
        : accounts_(accounts), attempts_(attempts) {}
    Result<std::vector<LearnerHistoryRow>> history(const Session& session) const;
    Result<std::vector<LearnerHistoryRow>> historyFor(const Session& session, Id ownerId) const;
    Error displayHistory(const Session& session, std::ostream& output) const;
private:
    const AccountStore& accounts_;
    const AttemptStore& attempts_;
};
class AttemptService {
public:
    explicit AttemptService(AttemptWriter& writer) : writer_(writer) {}
    Result<Id> submit(const Session& session, std::string quizTitle, std::string submittedAt,
        std::uint32_t correctCount, std::uint32_t totalQuestions);
private:
    AttemptWriter& writer_;
};
bool validSubmissionDate(const std::string& value);
}