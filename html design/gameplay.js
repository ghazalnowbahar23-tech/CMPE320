

// Find the question, answer form, and answer box
const qstn = document.getElementById("question");
const answrFrm = document.getElementById("answerForm");
const answrInpt = document.getElementById("cardAnswer");

// Find the submit, next, and retry buttons
const submitButt = document.getElementById("submitAnswer");
const nxtButt = document.getElementById("nextQuestion");
const rtryButt = document.getElementById("retryQuestion");

// Find the feedback and status messages
const fdb = document.getElementById("feedback");
const ststxt = document.getElementById("status");


// Track whether the player can answer, a request is running, or the session is over
let waitForAns = false;
let rqstInProg= false;
let sessFinished = false;

// Show feedback and set its state for styling
function showfdb(message, state = "") {
    fdb.textContent = message;
    fdb.dataset.state = state;
}

// Enable or disable the answer box and submit button
function enblAns(enabled) {
    answrInpt.disabled = !enabled;
    submitButt.disabled = !enabled;
}


// Get the current question from C++
async function loadQstn() {
    // Stop if request is running or session is finished
    if (rqstInProg || sessFinished) return;

    // Disable answers and hide buttons while loading
    rqstInProg = true;
    waitForAns = false;
    enblAns(false);
    nxtButt.hidden = true;
    rtryButt.hidden = true;
    ststxt.textContent = "Loading question...";
    showfdb("");

    try {
        // Request  current question from the server
        const rspn = await fetch("/api/question");

        if (!rspn.ok) {
            throw new Error("Question request failed.");
        }

        // Read the server's response
        const card = await rspn.json();

        if (card.finished) {
            fnshSess();
            return;
        }
        // Display  question and allow the player to answer
        qstn.textContent = card.question;
        answrInpt.value = "";
        waitForAns = true;
        enblAns(true);
        ststxt.textContent = "Type your answer, then press Submit or Enter.";
        answrInpt.focus();
        
    } catch (error) {
        // Show an error and let the player retry
        ststxt.textContent = "Could not load the question.";
        showfdb("Check that the C++ server is running, then retry.", "error");
        rtryButt.hidden = false;
    } finally {
        // Mark the request as complete, even if it failed
        rqstInProg = false;
    }
}


// Move the character to the step returned by C++
function moveCharacter(step) {
    const character = document.getElementById("character");
    const distance = step * 40;

    character.style.transform = "translateY(-" + distance + "px)";
}

// Display stats calculated by C++
function updateStats(result) {
    for (const name of ["score", "correctCount", "incorrectCount", "streak", "characterStep"]) {
        document.getElementById(name).textContent = result[name];
    }
}

// Stop accepting answers and display "Session complete!"
function fnshSess() {
    sessFinished = true;
    waitForAns = false;
    enblAns(false);
    answrFrm.hidden = true;
    nxtButt.hidden = true;
    rtryButt.hidden = true;
    qstn.textContent = "Session complete!";
    ststxt.textContent = "You have answered all the cards.";
}

// Handle the player submitting an answer
answrFrm.addEventListener("submit", async (event) => {
    event.preventDefault();

    // Stop if busy, not accepting answers, or finished
    if (rqstInProg || !waitForAns || sessFinished) return;

    const answer = answrInpt.value.trim();

    if (answer === "") {
        showfdb("Please type an answer first.", "error");
        answrInpt.focus();
        return;
    }

    // Disable answers while waiting for the server
    rqstInProg = true;
    waitForAns = false;
    enblAns(false);
    ststxt.textContent = "Checking your answer...";
    showfdb("");

    try {
        // Send the player's answer to the server 
        const rspn = await fetch("/api/answer", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ answer: answer })
        });
        
        
		// Report an error if the server returns an unsuccessful response 
        if (!rspn.ok) {
            throw new Error("Answer request failed.");
        }

        // Read the server's result and update the score, streak, and position
        const rslt = await rspn.json();
                
        updateStats(rslt);
		moveCharacter(rslt.characterStep);

        // Show whether the answer was correct
        if (rslt.correct) {
            showfdb("Correct! You climb one step.", "correct");
        } else {
            showfdb(
                "Incorrect. Your character falls one step (minimum zero).",
                "incorrect"
            );
        }

        // Finish the session or show the next question button
        if (rslt.finished) {
            fnshSess();
        } else {
            ststxt.textContent = "Answer checked. Continue when you are ready.";
            nxtButt.hidden = false;
            nxtButt.focus();
        }
    } catch (error) {
        // Show an error without sending the answer again
        ststxt.textContent = "Your answer could not be confirmed.";
        showfdb(
            "The server may have received it. Load the current question to continue; the displayed statistics may be out of date.",
            "error"
        );
        rtryButt.hidden = false;
    } finally {
        // Mark the request as complete, even if it failed
        rqstInProg = false;
    }
});

// Load question when next/retry is clicked
nxtButt.addEventListener("click", loadQstn);
rtryButt.addEventListener("click", loadQstn);

// Load first question when the page opens
loadQstn();



