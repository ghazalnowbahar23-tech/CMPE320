#include <crow.h>
#include "StudyTime.h"
#include "FlashcardManager.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "UserData.h"

using namespace std;

int main() {
    crow::SimpleApp app;
    

    // Show the account page
    CROW_ROUTE(app, "/main")
    ([]() {
        crow::response response;
        response.set_static_file_info_unsafe("../html design/main.html");
        return response;
    });

        // Show the flashcard page
    CROW_ROUTE(app, "/play")
    ([]() {
        crow::response response;
        response.set_static_file_info_unsafe("../html design/<FlashCardQ>.html");
        return response;
    });

    // Load the JavaScript
    CROW_ROUTE(app, "/gameplay.js")
    ([]() {
        crow::response response;
        response.set_static_file_info_unsafe("../html design/gameplay.js");
        return response;
    });

    // Load the flashcard GIF
    CROW_ROUTE(app, "/card_flip.gif")
    ([]() {
        crow::response response;
        response.set_static_file_info_unsafe("../html design/card_flip.gif");
        return response;
    });

    // Load the shared CSS
    CROW_ROUTE(app, "/css/global.css")
    ([]() {
        crow::response response;
        response.set_static_file_info_unsafe("../css/global.css");
        return response;
    });

    // Load the flashcard CSS
    CROW_ROUTE(app, "/css/flashcard.css")
    ([]() {
        crow::response response;
        response.set_static_file_info_unsafe("../css/flashcard.css");
        return response;
    });

     // Load the menu CSS
    CROW_ROUTE(app, "/css/menu.css")
    ([]() {
        crow::response response;
        response.set_static_file_info_unsafe("../css/menu.css");
        return response;
    });
    
    // Show the menu page
	CROW_ROUTE(app, "/menu")
	([]() {
 	  crow::response response;
  	  response.set_static_file_info_unsafe("../html design/<Menu>.html");
  	  return response;
	});
	
    // Show the import page
    CROW_ROUTE(app, "/import")
    ([]() {
    crow::response response;
    response.set_static_file_info_unsafe("../html design/import.html");
    return response;
    });

	// Load the character image
	CROW_ROUTE(app, "/cs320_character.gif")
	([]() {
   	 crow::response response;
   	 response.set_static_file_info_unsafe("../html design/cs320_character.gif");
   	 return response;
	});
    
    // Load the standing character for the staircase
    CROW_ROUTE(app, "/standing1:2.gif")
    ([]() {
    crow::response response;
    response.set_static_file_info_unsafe("../html design/standing1:2.gif");
    return response;
    });

    CROW_ROUTE(app, "/import.js")
    ([]() { crow::response r; r.set_static_file_info_unsafe("../html design/import.js"); return r; });

    // Fallback deck, used only if this account has not imported any cards.
    vector<Flashcard> deck = {
        {1, "What is 2 + 2?", {"4"}},
        {2, "What is the capital of France?", {"Paris"}},
        {3, "What language are we using?", {"C++"}}
    };

    UserData users("cardclimb.db");
    mutex gameMutex;
    unordered_map<int, unique_ptr<StudyTime>> games;
    auto tokenFrom = [](const crow::request& req) -> string {
        string header=req.get_header_value("Authorization");
        const string prefix="Bearer ";
        return header.compare(0,prefix.size(),prefix)==0 ? header.substr(prefix.size()) : "";
    };

    CROW_ROUTE(app, "/api/signup").methods(crow::HTTPMethod::POST)
    ([&users](const crow::request& req) {
        auto data=crow::json::load(req.body);
        if(!data||!data.has("username")||!data.has("password")||
           data["username"].t()!=crow::json::type::String||data["password"].t()!=crow::json::type::String)
            return crow::response(400,"Username and password required.");
        string error;
        if(!users.signup(data["username"].s(),data["password"].s(),error))return crow::response(400,error);
        crow::json::wvalue result;result["message"]="Account created";
        return crow::response(201,result);
    });
    CROW_ROUTE(app, "/api/login").methods(crow::HTTPMethod::POST)
    ([&users](const crow::request& req) {
        auto data=crow::json::load(req.body);
        if(!data||!data.has("username")||!data.has("password")||
           data["username"].t()!=crow::json::type::String||data["password"].t()!=crow::json::type::String)
            return crow::response(400,"Username and password required.");
        string token=users.login(data["username"].s(),data["password"].s());
        if(token.empty())return crow::response(401,"Invalid username or password.");
        crow::json::wvalue result;result["token"]=token;return crow::response(result);
    });
    CROW_ROUTE(app, "/api/logout").methods(crow::HTTPMethod::POST)
    ([&users,tokenFrom](const crow::request& req) {
        string token=tokenFrom(req);
        if(!users.userForToken(token))return crow::response(401,"Not logged in.");
        users.logout(token);return crow::response(200,"Logged out.");
    });
    CROW_ROUTE(app, "/api/progress")
    ([&users,tokenFrom](const crow::request& req) {
        int id=users.userForToken(tokenFrom(req));if(!id)return crow::response(401,"Not logged in.");
        auto p=users.progress(id);crow::json::wvalue r;
        r["totalCorrect"]=p.totalCorrect;r["totalIncorrect"]=p.totalIncorrect;
        r["totalSessions"]=p.totalSessions;r["characterStep"]=p.characterStep;
        r["dailyStreak"]=p.currentStreak;r["bestStreak"]=p.bestStreak;
        r["lastCompletedDay"]=p.lastCompletedDay;
        return crow::response(r);
    });
    CROW_ROUTE(app, "/api/history")
    ([&users,tokenFrom](const crow::request& req) {
        int id=users.userForToken(tokenFrom(req));if(!id)return crow::response(401,"Not logged in.");
        crow::json::wvalue::list items;
        for(const auto& h:users.history(id)){
            crow::json::wvalue r;r["date"]=h.date;r["correct"]=h.correct;
            r["incorrect"]=h.incorrect;r["score"]=h.score;r["characterStep"]=h.step;
            items.emplace_back(move(r));
        }
        crow::json::wvalue r;r["sessions"]=move(items);return crow::response(r);
    });
    CROW_ROUTE(app, "/api/session/start").methods(crow::HTTPMethod::POST)
    ([&users,&games,&gameMutex,&deck,tokenFrom](const crow::request& req) {
        int id=users.userForToken(tokenFrom(req));if(!id)return crow::response(401,"Not logged in.");
        auto imported=users.cardsForUser(id);
        lock_guard<mutex> lock(gameMutex);
        games[id]=make_unique<StudyTime>(imported.empty() ? deck : imported);
        crow::json::wvalue r;r["message"]="Session started";return crow::response(r);
    });

    // Each account has a separate study session; a login token is required.
    CROW_ROUTE(app, "/api/question")
    ([&users,&games,&gameMutex,tokenFrom](const crow::request& req) {
        int id=users.userForToken(tokenFrom(req));if(!id)return crow::response(401,"Not logged in.");
        lock_guard<mutex> lock(gameMutex);
        auto it=games.find(id);
        if(it==games.end())return crow::response(409,"Start a session first.");
        const Flashcard* card=it->second->getCurrentCard();
        crow::json::wvalue r;
        if(!card)r["finished"]=true;
        else {r["finished"]=false;r["id"]=card->id;r["question"]=card->question;}
        return crow::response(r);
    });
    CROW_ROUTE(app, "/api/answer").methods(crow::HTTPMethod::POST)
    ([&users,&games,&gameMutex,tokenFrom](const crow::request& req) {
        int id=users.userForToken(tokenFrom(req));if(!id)return crow::response(401,"Not logged in.");
        auto data=crow::json::load(req.body);
        if(!data||!data.has("answer")||data["answer"].t()!=crow::json::type::String||
           string(data["answer"].s()).find_first_not_of(" \t\r\n")==string::npos)
            return crow::response(400,"Please provide a non-empty answer.");
        lock_guard<mutex> lock(gameMutex);
        auto it=games.find(id);
        if(it==games.end())return crow::response(409,"Start a session first.");
        StudyTime& session=*it->second;
        if(session.isFinished())return crow::response(409,"Session already finished.");
        int before=session.getCorrect();
        session.submitAnswer(data["answer"].s());
        bool correct=session.getCorrect()>before;
        users.recordAnswer(id,correct);
        if(session.isFinished())users.completeSession(id,session.getCorrect(),session.getIncorrect(),session.getScore());
        auto progress=users.progress(id);
        crow::json::wvalue r;
        r["correct"]=correct;r["score"]=session.getScore();
        r["correctCount"]=session.getCorrect();r["incorrectCount"]=session.getIncorrect();
        r["streak"]=session.getCurrentStreak(); // consecutive correct answers in this session
        r["dailyStreak"]=progress.currentStreak;
        r["characterStep"]=progress.characterStep;
        r["finished"]=session.isFinished();
        return crow::response(r);
    });

    // Authenticated, persistent per-user CSV/JSON import (JSON parsed in the browser).
    CROW_ROUTE(app, "/api/cards/import").methods(crow::HTTPMethod::POST)
    ([&users,tokenFrom](const crow::request& req) {
        int id=users.userForToken(tokenFrom(req));
        if(!id)return crow::response(401,"Not logged in.");
        if(req.body.size()>1024*1024)return crow::response(413,"Import must be under 1 MB.");
        auto data=crow::json::load(req.body);
        if(!data || !data.has("cards") || data["cards"].t()!=crow::json::type::List)
            return crow::response(400,"Expected a cards array.");
        vector<Flashcard> batch;
        for(const auto& item:data["cards"]) {
            if(batch.size()>=500)return crow::response(400,"Maximum 500 cards per import.");
            if(item.t()!=crow::json::type::Object || !item.has("question") || !item.has("acceptedAnswers") ||
               item["question"].t()!=crow::json::type::String || item["acceptedAnswers"].t()!=crow::json::type::List)
                return crow::response(400,"Each card needs a question and acceptedAnswers array.");
            Flashcard card{}; card.question=string(item["question"].s());
            if(card.question.find_first_not_of(" \t\r\n")==string::npos || card.question.size()>1000)
                return crow::response(400,"Question must contain 1-1000 characters.");
            for(const auto& a:item["acceptedAnswers"]) {
                if(a.t()!=crow::json::type::String)
                    return crow::response(400,"Answers must be strings.");
                string answer=a.s();
                if(answer.find_first_not_of(" \t\r\n")==string::npos || answer.size()>1000)
                    return crow::response(400,"Answers must contain 1-1000 characters.");
                if(card.acceptedAnswers.size()>=20)
                    return crow::response(400,"Maximum 20 answers per card.");
                card.acceptedAnswers.push_back(move(answer));
            }
            if(card.acceptedAnswers.empty())return crow::response(400,"A card needs an answer.");
            batch.push_back(move(card));
        }
        if(batch.empty())return crow::response(400,"No cards found in file.");
        try {
            int count=users.importCards(id,batch);
            crow::json::wvalue result; result["imported"]=count;
            return crow::response(201,result);
        } catch(const std::exception&) {
            return crow::response(500,"Could not save imported cards.");
        }
    });

    FlashcardManager manager;
    // return all the flashcards as JSON
CROW_ROUTE(app, "/api/cards")
([&manager]() {
    crow::json::wvalue result;

    const vector<Flashcard>& cards = manager.getAllCards();

    // Create a JSON array for the flashcards
    crow::json::wvalue::list cardList;

    for (const Flashcard& card : cards) {
        crow::json::wvalue cardData;

        cardData["id"] = card.id;
        cardData["question"] = card.question;

        // Convert accepted answers into a JSON array
        crow::json::wvalue::list answerList;

        for (const string& answer : card.acceptedAnswers) {
            answerList.emplace_back(answer);
        }

        cardData["acceptedAnswers"] = move(answerList);
        cardList.emplace_back(move(cardData));
    }

    result["cards"] = move(cardList);

    return crow::response(result);
});


CROW_ROUTE(app, "/api/cards")
    .methods(crow::HTTPMethod::POST)
([&manager](const crow::request& req) {
    auto data = crow::json::load(req.body);
//validate the required fields
    if (!data ||
        !data.has("question") ||
        !data.has("acceptedAnswers") ||
        data["question"].t() != crow::json::type::String ||
        data["acceptedAnswers"].t() != crow::json::type::List) {
        return crow::response(
            400,
            "Provide a question and an array of accepted answers."
        );
    }
// extract the question and answers.
    string question = data["question"].s();
    vector<string> answers;

    for (const auto& answer : data["acceptedAnswers"]) {
        if (answer.t() != crow::json::type::String) {
            return crow::response(
                400,
                "Every accepted answer must be a string."
            );
        }

        answers.push_back(answer.s());
    }
// add the card
    int newId = manager.addCard(question, answers);

    if (newId == -1) {
        return crow::response(
            400,
            "Question and accepted answers cannot be empty."
        );
    }
// build the response
    crow::json::wvalue result;
    result["id"] = newId;
    result["question"] = question;

    crow::json::wvalue::list answerList;

    for (const string& answer : answers) {
        answerList.emplace_back(answer);
    }

    result["acceptedAnswers"] = move(answerList);

    return crow::response(201, result);
});

CROW_ROUTE(app, "/api/cards/<int>")
    .methods(crow::HTTPMethod::PUT)
([&manager](const crow::request& req, int id) {
    // parse the request JSON
    auto data = crow::json::load(req.body);

    if (!data ||
        !data.has("question") ||
        !data.has("acceptedAnswers") ||
        data["question"].t() != crow::json::type::String ||
        data["acceptedAnswers"].t() != crow::json::type::List) {
        return crow::response(
            400,
            "Provide a question and an array of accepted answers."
        );
    }
// EXTRACT the question and answers
    string question = data["question"].s();
    vector<string> answers;

    for (const auto& answer : data["acceptedAnswers"]) {
        if (answer.t() != crow::json::type::String) {
            return crow::response(
                400,
                "Every accepted answer must be a string."
            );
        }

        answers.push_back(answer.s());
    }
// update the card by its id
    bool updated = manager.updateCard(id, question, answers);
// handle an invalid id or empty id.
    if (!updated) {
        return crow::response(
            400,
            "Card could not be updated. Check the ID and make sure the question and answers are not empty."
        );
    }
// build the response
    crow::json::wvalue result;
    result["id"] = id;
    result["question"] = question;

    crow::json::wvalue::list answerList;

    for (const string& answer : answers) {
        answerList.emplace_back(answer);
    }

    result["acceptedAnswers"] = move(answerList);

    return crow::response(200, result);
});


CROW_ROUTE(app, "/api/cards/<int>")
    .methods(crow::HTTPMethod::DELETE)
([&manager](int id) {
    // Delete the card by ID.
    bool deleted = manager.deleteCard(id);

    // Handle a missing card.
    if (!deleted) {
        return crow::response(404, "Card not found.");
    }

    // Return the deletion confirmation.
    crow::json::wvalue result;
    result["message"] = "Card deleted successfully.";
    result["id"] = id;

    return crow::response(200, result);
});

    app.port(18080).run();

    return 0;
}
