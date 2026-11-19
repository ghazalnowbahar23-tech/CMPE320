#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <mutex>
#include "Flashcard.h"

struct UserProgress {
    int totalCorrect=0, totalIncorrect=0, totalSessions=0;
    int characterStep=0, currentStreak=0, bestStreak=0;
    std::string lastCompletedDay;
};
struct StudyRecord {
    std::string date;
    int correct=0, incorrect=0, score=0, step=0;
};
class UserData {
public:
    explicit UserData(const std::string& path);
    ~UserData();
    UserData(const UserData&)=delete;
    UserData& operator=(const UserData&)=delete;
    bool signup(const std::string& username, const std::string& password, std::string& error);
    std::string login(const std::string& username, const std::string& password);
    int userForToken(const std::string& token);
    void logout(const std::string& token);
    UserProgress progress(int userId);
    std::vector<StudyRecord> history(int userId, int limit=50);
    void recordAnswer(int userId, bool correct);
    void completeSession(int userId, int correct, int incorrect, int score);
    int importCards(int userId, const std::vector<Flashcard>& cards);
    std::vector<Flashcard> cardsForUser(int userId);
private:
    sqlite3* db=nullptr;
    std::mutex dbMutex;
    void exec(const char* sql);
};
