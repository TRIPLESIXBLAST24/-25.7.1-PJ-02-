#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include "user.h"
#include "message.h"

using namespace std;

struct UserData {
    string login;
    string password;
    string name;
    vector<string> friends;
};

struct MessageData {
    string senderLogin;
    string recipientLogin;
    string text;
    string type;
    long long timestamp;
    vector<string> tags;
};

class Database {
private:
    string dbPath;
    
    string getUsersFilePath() const;
    string getMessagesFilePath() const;
    
    string serializeUser(const UserData& user) const;
    UserData deserializeUser(const string& line) const;
    string serializeMessage(const MessageData& msg) const;
    MessageData deserializeMessage(const string& line) const;
    string escapeString(const string& s) const;
    string unescapeString(const string& s) const;

public:
    Database(const string& path = "chat.db");
    ~Database();
    
    bool initialize();
    
    bool addUser(const string& login, const string& password, const string& name);
    bool userExists(const string& login) const;
    bool checkUserPassword(const string& login, const string& password) const;
    UserData getUser(const string& login) const;
    vector<UserData> getAllUsers() const;
    bool updateUser(const UserData& user);
    
    bool addMessage(const MessageData& message);
    vector<MessageData> getAllMessages() const;
    vector<MessageData> getMessagesForUser(const string& login) const;
    
    bool addFriend(const string& userLogin, const string& friendLogin);
    bool removeFriend(const string& userLogin, const string& friendLogin);
    vector<string> getUserFriends(const string& login) const;
};

#endif

