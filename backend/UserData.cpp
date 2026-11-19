#include "UserData.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <stdexcept>
#include <ctime>
#include <cctype>
#include <memory>

using namespace std;

namespace {
using Statement = unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
Statement stmt(sqlite3* db, const char* sql) {
    sqlite3_stmt* p=nullptr;
    if(sqlite3_prepare_v2(db,sql,-1,&p,nullptr)!=SQLITE_OK) throw runtime_error(sqlite3_errmsg(db));
    return Statement(p,sqlite3_finalize);
}
void bindText(sqlite3_stmt* s,int n,const string& value) { sqlite3_bind_text(s,n,value.c_str(),-1,SQLITE_TRANSIENT); }
string hex(const unsigned char* p,size_t n) {
    const char* digits="0123456789abcdef"; string out; out.reserve(n*2);
    for(size_t i=0;i<n;++i){out+=digits[p[i]>>4];out+=digits[p[i]&15];}return out;
}
string randomHex(size_t bytes) {
    vector<unsigned char> b(bytes);
    if(RAND_bytes(b.data(),static_cast<int>(bytes))!=1) throw runtime_error("Random generator failed");
    return hex(b.data(),b.size());
}
string hashPassword(const string& password,const string& salt) {
    unsigned char result[32];
    if(PKCS5_PBKDF2_HMAC(password.c_str(),static_cast<int>(password.size()),
        reinterpret_cast<const unsigned char*>(salt.data()),static_cast<int>(salt.size()),
        210000,EVP_sha256(),sizeof(result),result)!=1) throw runtime_error("Password hash failed");
    return hex(result,sizeof(result));
}
string today() {
    time_t now=time(nullptr); tm t{};
#ifdef _WIN32
    localtime_s(&t,&now);
#else
    localtime_r(&now,&t);
#endif
    char buf[11];strftime(buf,sizeof(buf),"%Y-%m-%d",&t);return buf;
}
string yesterday() {
    time_t now=time(nullptr)-86400;tm t{};
#ifdef _WIN32
    localtime_s(&t,&now);
#else
    localtime_r(&now,&t);
#endif
    char buf[11];strftime(buf,sizeof(buf),"%Y-%m-%d",&t);return buf;
}
}
void UserData::exec(const char* sql){char* err=nullptr;if(sqlite3_exec(db,sql,nullptr,nullptr,&err)!=SQLITE_OK){string msg=err?err:"SQLite error";sqlite3_free(err);throw runtime_error(msg);}}
UserData::UserData(const string& path){
    if(sqlite3_open(path.c_str(),&db)!=SQLITE_OK) throw runtime_error("Cannot open database");
    sqlite3_busy_timeout(db,3000);
    exec("PRAGMA foreign_keys=ON;");
    exec("CREATE TABLE IF NOT EXISTS users(id INTEGER PRIMARY KEY, username TEXT NOT NULL UNIQUE COLLATE NOCASE, salt TEXT NOT NULL, password_hash TEXT NOT NULL);");
    exec("CREATE TABLE IF NOT EXISTS tokens(token TEXT PRIMARY KEY, user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE);");
    exec("CREATE TABLE IF NOT EXISTS progress(user_id INTEGER PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE, correct INTEGER NOT NULL DEFAULT 0, incorrect INTEGER NOT NULL DEFAULT 0, sessions INTEGER NOT NULL DEFAULT 0, step INTEGER NOT NULL DEFAULT 0, streak INTEGER NOT NULL DEFAULT 0, best_streak INTEGER NOT NULL DEFAULT 0, last_day TEXT NOT NULL DEFAULT '');");
    exec("CREATE TABLE IF NOT EXISTS history(id INTEGER PRIMARY KEY, user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE, day TEXT NOT NULL, correct INTEGER NOT NULL, incorrect INTEGER NOT NULL, score INTEGER NOT NULL, step INTEGER NOT NULL);");
    exec("CREATE TABLE IF NOT EXISTS imported_cards(id INTEGER PRIMARY KEY, user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE, question TEXT NOT NULL);");
    exec("CREATE TABLE IF NOT EXISTS imported_answers(card_id INTEGER NOT NULL REFERENCES imported_cards(id) ON DELETE CASCADE, answer TEXT NOT NULL);");
}
UserData::~UserData(){if(db)sqlite3_close(db);}
bool UserData::signup(const string& username,const string& password,string& error){
    if(username.size()<3||username.size()>32){error="Username must be 3-32 characters.";return false;}
    for(unsigned char c:username)if(!isalnum(c)&&c!='_'&&c!='-'){error="Username may contain letters, numbers, _ and -.";return false;}
    if(password.size()<8||password.size()>256){error="Password must be 8-256 characters.";return false;}
    lock_guard<mutex> lock(dbMutex);
    auto s=stmt(db,"INSERT INTO users(username,salt,password_hash) VALUES(?,?,?)");
    auto salt=randomHex(16);auto hash=hashPassword(password,salt);
    bindText(s.get(),1,username);bindText(s.get(),2,salt);bindText(s.get(),3,hash);
    if(sqlite3_step(s.get())!=SQLITE_DONE){error="Username already taken.";return false;}
    int id=static_cast<int>(sqlite3_last_insert_rowid(db));
    auto p=stmt(db,"INSERT INTO progress(user_id) VALUES(?)");sqlite3_bind_int(p.get(),1,id);
    if(sqlite3_step(p.get())!=SQLITE_DONE)throw runtime_error("Could not initialize progress");
    return true;
}
string UserData::login(const string& username,const string& password){
    lock_guard<mutex> lock(dbMutex);
    auto s=stmt(db,"SELECT id,salt,password_hash FROM users WHERE username=?");bindText(s.get(),1,username);
    if(sqlite3_step(s.get())!=SQLITE_ROW)return "";
    int id=sqlite3_column_int(s.get(),0);
    string salt=reinterpret_cast<const char*>(sqlite3_column_text(s.get(),1));
    string stored=reinterpret_cast<const char*>(sqlite3_column_text(s.get(),2));
    string actual=hashPassword(password,salt);
    if(actual.size()!=stored.size()||CRYPTO_memcmp(actual.data(),stored.data(),actual.size())!=0)return "";
    string token=randomHex(32);
    auto t=stmt(db,"INSERT INTO tokens(token,user_id) VALUES(?,?)");bindText(t.get(),1,token);sqlite3_bind_int(t.get(),2,id);
    if(sqlite3_step(t.get())!=SQLITE_DONE)throw runtime_error("Token insert failed");
    return token;
}
int UserData::userForToken(const string& token){
    if(token.empty())return 0;
    lock_guard<mutex> lock(dbMutex);
    auto s=stmt(db,"SELECT user_id FROM tokens WHERE token=?");bindText(s.get(),1,token);
    return sqlite3_step(s.get())==SQLITE_ROW?sqlite3_column_int(s.get(),0):0;
}
void UserData::logout(const string& token){lock_guard<mutex> lock(dbMutex);auto s=stmt(db,"DELETE FROM tokens WHERE token=?");bindText(s.get(),1,token);sqlite3_step(s.get());}
UserProgress UserData::progress(int id){
    lock_guard<mutex> lock(dbMutex);
    auto s=stmt(db,"SELECT correct,incorrect,sessions,step,streak,best_streak,last_day FROM progress WHERE user_id=?");sqlite3_bind_int(s.get(),1,id);
    UserProgress p;
    if(sqlite3_step(s.get())==SQLITE_ROW){p.totalCorrect=sqlite3_column_int(s.get(),0);p.totalIncorrect=sqlite3_column_int(s.get(),1);p.totalSessions=sqlite3_column_int(s.get(),2);p.characterStep=sqlite3_column_int(s.get(),3);p.currentStreak=sqlite3_column_int(s.get(),4);p.bestStreak=sqlite3_column_int(s.get(),5);p.lastCompletedDay=reinterpret_cast<const char*>(sqlite3_column_text(s.get(),6));}
    if(!p.lastCompletedDay.empty()&&p.lastCompletedDay!=today()&&p.lastCompletedDay!=yesterday())p.currentStreak=0;
    return p;
}
vector<StudyRecord> UserData::history(int id,int limit){
    lock_guard<mutex> lock(dbMutex);
    auto s=stmt(db,"SELECT day,correct,incorrect,score,step FROM history WHERE user_id=? ORDER BY id DESC LIMIT ?");
    sqlite3_bind_int(s.get(),1,id);sqlite3_bind_int(s.get(),2,limit);
    vector<StudyRecord> out;
    while(sqlite3_step(s.get())==SQLITE_ROW){StudyRecord r;r.date=reinterpret_cast<const char*>(sqlite3_column_text(s.get(),0));r.correct=sqlite3_column_int(s.get(),1);r.incorrect=sqlite3_column_int(s.get(),2);r.score=sqlite3_column_int(s.get(),3);r.step=sqlite3_column_int(s.get(),4);out.push_back(r);}return out;
}
void UserData::recordAnswer(int id,bool correct){
    lock_guard<mutex> lock(dbMutex);
    auto s=stmt(db,correct?"UPDATE progress SET correct=correct+1,step=step+1 WHERE user_id=?":"UPDATE progress SET incorrect=incorrect+1,step=MAX(0,step-1) WHERE user_id=?");
    sqlite3_bind_int(s.get(),1,id);if(sqlite3_step(s.get())!=SQLITE_DONE)throw runtime_error("Failed to save answer");
}
void UserData::completeSession(int id,int correct,int incorrect,int score){
    lock_guard<mutex> lock(dbMutex);
    exec("BEGIN IMMEDIATE;");
    try {
        auto s=stmt(db,"SELECT step,streak,best_streak,last_day FROM progress WHERE user_id=?");sqlite3_bind_int(s.get(),1,id);
        if(sqlite3_step(s.get())!=SQLITE_ROW)throw runtime_error("User missing");
        int step=sqlite3_column_int(s.get(),0),streak=sqlite3_column_int(s.get(),1),best=sqlite3_column_int(s.get(),2);
        string last=reinterpret_cast<const char*>(sqlite3_column_text(s.get(),3));
        string day=today();
        if(last!=day){streak=(last==yesterday())?streak+1:1;}
        if(streak>best)best=streak;
        // A completed session earns five bonus steps. Do not award twice on the same day.
        if(last!=day)step+=5;
        auto p=stmt(db,"UPDATE progress SET sessions=sessions+1,step=?,streak=?,best_streak=?,last_day=? WHERE user_id=?");
        sqlite3_bind_int(p.get(),1,step);sqlite3_bind_int(p.get(),2,streak);sqlite3_bind_int(p.get(),3,best);bindText(p.get(),4,day);sqlite3_bind_int(p.get(),5,id);
        if(sqlite3_step(p.get())!=SQLITE_DONE)throw runtime_error("Progress update failed");
        auto h=stmt(db,"INSERT INTO history(user_id,day,correct,incorrect,score,step) VALUES(?,?,?,?,?,?)");
        sqlite3_bind_int(h.get(),1,id);bindText(h.get(),2,day);sqlite3_bind_int(h.get(),3,correct);sqlite3_bind_int(h.get(),4,incorrect);sqlite3_bind_int(h.get(),5,score);sqlite3_bind_int(h.get(),6,step);
        if(sqlite3_step(h.get())!=SQLITE_DONE)throw runtime_error("History insert failed");
        exec("COMMIT;");
    }catch(...){exec("ROLLBACK;");throw;}
}

// Imported decks belong to the account that uploaded them and survive restarts.
int UserData::importCards(int userId, const vector<Flashcard>& cards) {
    lock_guard<mutex> lock(dbMutex);
    if (cards.empty()) return 0;
    exec("BEGIN IMMEDIATE;");
    try {
        auto insCard=stmt(db,"INSERT INTO imported_cards(user_id,question) VALUES(?,?)");
        auto insAnswer=stmt(db,"INSERT INTO imported_answers(card_id,answer) VALUES(?,?)");
        for(const auto& card: cards) {
            sqlite3_reset(insCard.get()); sqlite3_clear_bindings(insCard.get());
            sqlite3_bind_int(insCard.get(),1,userId);
            bindText(insCard.get(),2,card.question);
            if(sqlite3_step(insCard.get())!=SQLITE_DONE) throw runtime_error("Card insert failed");
            int cardId=static_cast<int>(sqlite3_last_insert_rowid(db));
            for(const auto& answer: card.acceptedAnswers) {
                sqlite3_reset(insAnswer.get()); sqlite3_clear_bindings(insAnswer.get());
                sqlite3_bind_int(insAnswer.get(),1,cardId);
                bindText(insAnswer.get(),2,answer);
                if(sqlite3_step(insAnswer.get())!=SQLITE_DONE) throw runtime_error("Answer insert failed");
            }
        }
        exec("COMMIT;");
        return static_cast<int>(cards.size());
    } catch(...) { exec("ROLLBACK;"); throw; }
}

vector<Flashcard> UserData::cardsForUser(int userId) {
    lock_guard<mutex> lock(dbMutex);
    auto cardsQuery=stmt(db,"SELECT id,question FROM imported_cards WHERE user_id=? ORDER BY id");
    sqlite3_bind_int(cardsQuery.get(),1,userId);
    auto answersQuery=stmt(db,"SELECT answer FROM imported_answers WHERE card_id=? ORDER BY rowid");
    vector<Flashcard> result;
    while(sqlite3_step(cardsQuery.get())==SQLITE_ROW) {
        Flashcard card;
        card.id=sqlite3_column_int(cardsQuery.get(),0);
        card.question=reinterpret_cast<const char*>(sqlite3_column_text(cardsQuery.get(),1));
        sqlite3_reset(answersQuery.get()); sqlite3_clear_bindings(answersQuery.get());
        sqlite3_bind_int(answersQuery.get(),1,card.id);
        while(sqlite3_step(answersQuery.get())==SQLITE_ROW)
            card.acceptedAnswers.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(answersQuery.get(),0)));
        result.push_back(move(card));
    }
    return result;
}
