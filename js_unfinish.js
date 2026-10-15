
    //this part is about edit card
    <script>
        // Save the user's question and answer
        function saveCard() {

            let question = document.getElementById("question").value;
            let answer = document.getElementById("answer").value;

            // Check that both input boxes are filled
            if (question.trim() === "" || answer.trim() === "") {
                alert("Please enter both a question and an answer.");
                return;
            }

            // Save the card in browser memory
            localStorage.setItem("cardQuestion", question);
            localStorage.setItem("cardAnswer", answer);

            // Hide the input boxes
            document.getElementById("cardForm").style.display = "none";

            // Show the saved question, answer, and GIF
            document.getElementById("savedCard").style.display = "block";

            document.getElementById("showQuestion").textContent = question;
            document.getElementById("showAnswer").textContent = answer;
        }

        // Allow user to edit the card again
        function editAgain() {
            document.getElementById("cardForm").style.display = "block";
            document.getElementById("savedCard").style.display = "none";
        }

        // Load previously saved information
        window.onload = function() {
            let question = localStorage.getItem("cardQuestion");
            let answer = localStorage.getItem("cardAnswer");

            if (question !== null) {
                document.getElementById("question").value = question;
            }

            if (answer !== null) {
                document.getElementById("answer").value = answer;
            }
        };
    </script>


//this part is about delete card

    <script>
        // Display the saved card
        function loadCard() {
            let question = localStorage.getItem("cardQuestion");
            let answer = localStorage.getItem("cardAnswer");

            if (question === null) {
                document.getElementById("message").textContent =
                    "No card has been saved.";
                document.getElementById("deleteButton").style.display = "none";
                return;
            }

            document.getElementById("question").textContent = question;
            document.getElementById("answer").textContent = answer;
        }

        // Delete the saved card
        function deleteCard() {
            localStorage.removeItem("cardQuestion");
            localStorage.removeItem("cardAnswer");

            document.getElementById("question").textContent = "";
            document.getElementById("answer").textContent = "";
            document.getElementById("message").textContent =
                "Card deleted successfully!";

            document.getElementById("deleteButton").style.display = "none";
        }

        window.onload = loadCard;
    </script>

</body>
</html>
