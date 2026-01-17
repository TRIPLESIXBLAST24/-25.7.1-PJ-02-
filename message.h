#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <chrono>
#include <vector>
#include "user.h"

using namespace std;

enum class MessageType {
    PUBLIC,
    PRIVATE,
    SYSTEM
};

class Message {
private:
    const User* sender;
    const User* recipient;
    string text;
    chrono::system_clock::time_point timestamp;
    MessageType type;
    vector<string> tags;

public:
    Message(const User* sender, const User* recipient, const string& text, MessageType type = MessageType::PUBLIC);
    
    const User* getSender() const;
    const User* getRecipient() const;
    const string& getText() const;
    const chrono::system_clock::time_point& getTimestamp() const;
    MessageType getType() const;
    const vector<string>& getTags() const;
    
    void addTag(const string& tag);
    void removeTag(const string& tag);
    
    string toString() const;
    string getFormattedTime() const;
    bool hasTag(const string& tag) const;
    bool isPublic() const;
    bool isPrivate() const;
    bool isSystem() const;
    
    static string typeToString(MessageType type);
};

#endif