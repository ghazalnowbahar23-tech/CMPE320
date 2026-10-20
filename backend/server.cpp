#include <crow.h>
#include "StudyTime.h"
#include "FlashcardManager.h"
#include <vector>

using namespace std;

int main() {
    crow::SimpleApp app;
    
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
    
    // Show the menu page
	CROW_ROUTE(app, "/menu")
	([]() {
 	  crow::response response;
  	  response.set_static_file_info_unsafe("../html design/<Menu>.html");
  	  return response;
	});
	
	// Load the character image
	CROW_ROUTE(app, "/cs320_character.gif")
	([]() {
   	 crow::response response;
   	 response.set_static_file_info_unsafe("../html design/cs320_character.gif");
   	 return response;
	});
    
    // Temporary flashcards for testing the server.
    vector<Flashcard> deck = {
        {1, "What is 2 + 2?", {"4"}},
        {2, "What is the capital of France?", {"Paris"}},
        {3, "What language are we using?", {"C++"}}
    };

    StudyTime session(deck);

    // Send the current question to the browser.
    CROW_ROUTE(app, "/api/question")
    ([&session]() {
        crow::json::wvalue result;

        const Flashcard* card = session.getCurrentCard();

        if (card == nullptr) {
            result["finished"] = true;
        } else {
            result["finished"] = false;
            result["id"] = card->id;
            result["question"] = card->question;
        }

        return crow::response(result);
    });

    // Receive an answer and return the updated game statistics.
    CROW_ROUTE(app, "/api/answer")
        .methods(crow::HTTPMethod::POST)
    ([&session](const crow::request& req) {
        auto data = crow::json::load(req.body);

        if (!data || !data.has("answer") ||
            data["answer"].t() != crow::json::type::String) {
            return crow::response(400, "Please provide an answer.");
        }

        if (session.isFinished()) {
            return crow::response(400, "The game has finished.");
        }

        string answer = data["answer"].s();

        int previousCorrect = session.getCorrect();
        session.submitAnswer(answer);
        bool correct = session.getCorrect() > previousCorrect;

        crow::json::wvalue result;
        result["correct"] = correct;
        result["score"] = session.getScore();
        result["correctCount"] = session.getCorrect();
        result["incorrectCount"] = session.getIncorrect();
        result["streak"] = session.getCurrentStreak();
        result["characterStep"] = session.getCharacterStep();
        result["finished"] = session.isFinished();

        return crow::response(result);
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

        cardData["acceptedAnswers"] = std::move(answerList);
        cardList.emplace_back(std::move(cardData));
    }

    result["cards"] = std::move(cardList);

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

    result["acceptedAnswers"] = std::move(answerList);

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

    result["acceptedAnswers"] = std::move(answerList);

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
