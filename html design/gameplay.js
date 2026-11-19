

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

// Get the token saved when the player logged in
const token = sessionStorage.getItem("token");




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
        // Request the question using the player's login token
		const rspn = await fetch("/api/question", {
   			headers: {
        		"Authorization": "Bearer " + token
   		    }
        });

        if (rspn.status === 401) { sessionStorage.removeItem("token"); window.location.href = "/main"; return; }
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




// Each Play session begins on the grass, independently of saved lifetime progress.
let sessionStage = 0;

// Land the feet exactly on the top border of the matching stair.
// Only animate after an answer, never on the first page load.
function moveCharacter(sessionStep, animateJump = true) {
    const character = document.getElementById("character");
    const stageNumber = document.getElementById("stageNumber");
    const position = Math.max(0, Math.min(5, Math.floor(Number(sessionStep) || 0)));

    const landing = (n) => {
        if (n === 0) return { x: 0, y: 0 }; // Standing on the grass.
        const stair = document.getElementById("step" + n);
        // Character starts left:0, bottom:0 in the stair area.
        // offsetTop and offsetHeight are before the area's responsive scaling.
        const x = stair.offsetLeft + (stair.offsetWidth - character.offsetWidth) / 2;
        const characterTop = character.offsetTop;
        const y = stair.offsetTop - character.offsetHeight - characterTop;
        return { x, y };
    };

    const previous = Math.max(0, Math.min(5, Number(character.dataset.step || 0)));
    const start = landing(previous);
    const end = landing(position);
    const pose = (p) => `translate(${p.x}px, ${p.y}px)`;
    character.getAnimations().forEach(animation => animation.cancel());
    character.style.transform = pose(end);

    if (animateJump && previous !== position) {
        character.animate([
            { transform: pose(start), offset: 0 },
            { transform: pose({x:(start.x + end.x)/2, y:(start.y + end.y)/2 - 25}), offset: 0.5 },
            { transform: pose(end), offset: 1 }
        ], { duration: 500, easing: "ease-in-out" });
    }

    character.dataset.step = String(position);
    stageNumber.textContent = String(position);
}

// Display stats calculated by C++
function updateStats(result) {
    for (const name of ["score", "correctCount", "incorrectCount",
                       "streak", "dailyStreak", "characterStep"]) {
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
    	// Send the player's answer and login token to C++
    	const rspn = await fetch("/api/answer", {
        	method: "POST",
        	headers: {
           	 "Content-Type": "application/json",
           	 "Authorization": "Bearer " + token
        	},
        	body: JSON.stringify({ answer: answer })
    	});

        
		// Report an error if the server returns an unsuccessful response 
        if (!rspn.ok) {
            throw new Error("Answer request failed.");
        }

        // Read the server's result and update the score, streak, and position
        const rslt = await rspn.json();
                
        updateStats(rslt);
		sessionStage = Math.max(0, Math.min(5, sessionStage + (rslt.correct ? 1 : -1)));
        moveCharacter(sessionStage, true);

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

	// Start a study session for the logged-in player
	async function startSess() {
 	   // Return to the account page if no token is saved
  	  if (!token) {
       	 window.location.href = "/main";
       	 return;
   	 }

    ststxt.textContent = "Starting session...";
    enblAns(false);

    try {
        const rspn = await fetch("/api/session/start", {
            method: "POST",
            headers: {
                "Authorization": "Bearer " + token
            }
        });

        // Ask the player to log in again if the token is invalid
        if (rspn.status === 401) {
            sessionStorage.removeItem("token");
            window.location.href = "/main";
            return;
        }

        if (!rspn.ok) {
            throw new Error("Could not start session.");
        }

        
    // Get the player's saved progress
	const progressRspn = await fetch("/api/progress", {
   	 	headers: {
       	   "Authorization": "Bearer " + token
   		 }
	});

if (!progressRspn.ok) {
    throw new Error("Could not load progress.");
}

const progress = await progressRspn.json();

// Display the saved daily streak and character position
document.getElementById("dailyStreak").textContent = progress.dailyStreak;
document.getElementById("characterStep").textContent = progress.characterStep;
// The saved total stays in Character Step, while THIS run starts at Stage 0.
sessionStage = 0;
moveCharacter(0, false);

// Load the first question
await loadQstn();

        
    } catch (error) {
        ststxt.textContent = "Could not start the session.";
        showfdb("Check that the server is running, then refresh the page.", "error");
    }
}



// Load a question when Next or Retry is clicked
nxtButt.addEventListener("click", loadQstn);
rtryButt.addEventListener("click", loadQstn);

// Image loads asynchronously: re-align the feet on the top edge once ready.
const playerImage = document.getElementById("character");
playerImage.addEventListener("load", () => {
    moveCharacter(Number(playerImage.dataset.step || 0), false);
});

// Start the session when the page opens
startSess();


