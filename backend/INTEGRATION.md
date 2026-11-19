# Card Climb — Ghazal backend update

## Included
- `UserData.h/.cpp`: SQLite-backed accounts, PBKDF2-HMAC-SHA256 password hashes with per-user random salts, random login tokens, progress, daily streaks, and session history.
- `server.cpp`: adds account and progress endpoints and gives each logged-in user a separate in-memory study session.
- Existing flashcard, answer-checker, and study classes are copied unchanged.

## Build
Requires C++17, Crow (plus its dependencies), SQLite3 and OpenSSL crypto. On a system where Crow headers are installed:

```bash
g++ -std=c++17 -pthread server.cpp StudyTime.cpp AnswerChecker.cpp FlashcardManager.cpp UserData.cpp -lsqlite3 -lcrypto -o server
./server
```

This build command has NOT been tested in this environment because Crow headers are not installed here. `UserData.cpp` and `testUserData.cpp` have been compiled and tested successfully with:

```bash
g++ -std=c++17 -Wall -Wextra UserData.cpp testUserData.cpp -lsqlite3 -lcrypto -o testUserData
./testUserData
```

The database is `cardclimb.db` in the server's current working directory. Keep it out of Git: add `cardclimb.db*`, executables, and secrets to `.gitignore`.

## HTTP API (JSON)
- `POST /api/signup` body `{"username":"student","password":"atLeast8Chars"}` -> 201
- `POST /api/login` same body -> `{"token":"..."}`; incorrect password -> 401
- `POST /api/logout` with `Authorization: Bearer TOKEN`
- `GET /api/progress` with token -> totalCorrect, totalIncorrect, totalSessions, characterStep, dailyStreak, bestStreak, lastCompletedDay
- `GET /api/history` with token -> sessions array (newest first, max 50)
- `POST /api/session/start` with token -> reset this user's in-memory study session
- `GET /api/question` with token -> next question
- `POST /api/answer` with token, body `{"answer":"4"}` -> correctness, score, current stats, persisted characterStep and dailyStreak

The frontend should retain the token for the current login session and include it in Authorization headers. Avoid exposing it in URLs or source control.

## Important limitations / next integration tasks
1. **Card CRUD is still global and unauthenticated**: existing `/api/cards` routes are unchanged. These MUST be restricted to the authenticated user and backed by per-user deck/card persistence before claiming that users cannot see each other's cards. Coordinate with Eunje's storage/scheduler work.
2. The test deck is still hardcoded in `server.cpp`. Integrate with the per-user due-card scheduler; current `StudyTime` moves forward after a wrong answer instead of retrying it.
3. Study sessions live in memory and reset on server restart. Completed-session history, totals and staircase position persist in SQLite, but unfinished session position does not.
4. A session is marked complete when the hardcoded deck is exhausted. The streak requirement specifies finishing all *due* cards; connect to the scheduler before final acceptance.
5. Daily streak uses the server's local calendar day; coordinate on time-zone behavior.
6. The current server serves static files with existing paths; verify these against the team's actual frontend directory.
7. No token expiry/rate limiting/HTTPS yet. This is a local-only prototype, not suitable for public deployment.
8. Do not compile multiple `main()` files together (`main.cpp`, `StudyTime_test.cpp`, `testFlashcardManager.cpp`, `testUserData.cpp`).
9. `UserData` unit tests passed; full Crow HTTP integration has not been compiled or run in this environment.

## Suggested GitLab workflow
```bash
git checkout -b feature/ghazal-accounts-progress
# Copy changed server.cpp and new UserData.* into the repository's actual backend folder.
# Add .gitignore entries for cardclimb.db*, build outputs, and secrets.
git add path/to/server.cpp path/to/UserData.cpp path/to/UserData.h
# Include testUserData.cpp and INTEGRATION.md as appropriate.
git commit -m "Add account authentication and persisted study progress"
git push -u origin feature/ghazal-accounts-progress
```
Open a merge request for teammates to review. Do not overwrite their concurrent changes to server.cpp; merge carefully.
