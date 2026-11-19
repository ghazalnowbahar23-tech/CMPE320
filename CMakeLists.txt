cmake_minimum_required(VERSION 3.20)
project(CardClimb LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cardclimb_core
    backend/AnswerChecker.cpp
    backend/StudyTime.cpp
    backend/FlashcardManager.cpp
)

target_include_directories(cardclimb_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/backend"
)

# Apply warnings to the core and the executables that link it.
if(MSVC)
    target_compile_options(cardclimb_core PUBLIC /W4)
else()
    target_compile_options(cardclimb_core PUBLIC -Wall -Wextra)
endif()

include(FetchContent)
FetchContent_Declare(SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        3.3.2
)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SQLiteCpp)
target_link_libraries(cardclimb_core PUBLIC SQLiteCpp)

add_executable(cardclimb_cli backend/main.cpp)
target_link_libraries(cardclimb_cli PRIVATE cardclimb_core)
