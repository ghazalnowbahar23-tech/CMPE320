#include "UserData.h"
#include <cassert>
#include <cstdio>
#include <iostream>

using namespace std;
int main(){
    const char* path="/tmp/cardclimb_userdata_test.db";
    remove(path);
    {
        UserData db(path);string error;
        assert(db.signup("ghazal","password123",error));
        assert(!db.signup("ghazal","password123",error));
        assert(db.login("ghazal","wrong").empty());
        auto token=db.login("ghazal","password123");assert(!token.empty());
        int id=db.userForToken(token);assert(id>0);
        assert(db.signup("friend","anotherPassword123",error));
        db.recordAnswer(id,true);db.recordAnswer(id,false);
        db.completeSession(id,1,1,5);
        auto p=db.progress(id);assert(p.totalCorrect==1&&p.totalIncorrect==1);
        assert(p.totalSessions==1&&p.currentStreak==1&&p.characterStep==5);
        assert(db.history(id).size()==1);
        int other=db.userForToken(db.login("friend","anotherPassword123"));
        assert(db.progress(other).totalCorrect==0&&db.history(other).empty());
        db.logout(token);assert(db.userForToken(token)==0);
    }
    {
        UserData db(path);auto token=db.login("ghazal","password123");
        assert(!token.empty());int id=db.userForToken(token);
        assert(db.progress(id).totalSessions==1&&db.history(id).size()==1);
    }
    remove(path);cout<<"All UserData tests passed\n";
}
