#include "message.h"
#include <sstream>
#include <string>
#include <algorithm>
#include <iomanip>
#include <ctime>

using namespace std;

Message::Message(const User* sender, const User* recipient, const string& text, MessageType type)
    : sender(sender), recipient(recipient), text(text), type(type) {
    timestamp = chrono::system_clock::now();
}

const User* Message::getSender() const {
    return sender;
}

const User* Message::getRecipient() const {
    return recipient;
}

const string& Message::getText() const {
    return text;
}

const chrono::system_clock::time_point& Message::getTimestamp() const {
    return timestamp;
}

MessageType Message::getType() const {
    return type;
}

const vector<string>& Message::getTags() const {
    return tags;
}

void Message::addTag(const string& tag) {
    if (!hasTag(tag)) {
        tags.push_back(tag);
    }
}

void Message::removeTag(const string& tag) {
    auto it = find(tags.begin(), tags.end(), tag);
    if (it != tags.end()) {
        tags.erase(it);
    }
}

string Message::toString() const {
    stringstream ss;
    ss << "[" << getFormattedTime() << "] ";
    
    if (type == MessageType::SYSTEM) {
        ss << "[SYSTEM]: " << text;
    } else if (recipient == nullptr) {
        ss << sender->getName() << ": " << text;
    } else {
        ss << sender->getName() << " -> " << recipient->getName() << ": " << text;
    }
    
    if (!tags.empty()) {
        ss << " [Tags: ";
        for (size_t i = 0; i < tags.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << tags[i];
        }
        ss << "]";
    }
    
    return ss.str();
}

string Message::getFormattedTime() const {
    auto time_t = chrono::system_clock::to_time_t(timestamp);
    auto tm = *localtime(&time_t);
    
    stringstream ss;
    ss << setfill('0') << setw(2) << tm.tm_hour << ":"
       << setfill('0') << setw(2) << tm.tm_min << ":"
       << setfill('0') << setw(2) << tm.tm_sec;
    return ss.str();
}

bool Message::hasTag(const string& tag) const {
    return find(tags.begin(), tags.end(), tag) != tags.end();
}

bool Message::isPublic() const {
    return type == MessageType::PUBLIC;
}

bool Message::isPrivate() const {
    return type == MessageType::PRIVATE;
}

bool Message::isSystem() const {
    return type == MessageType::SYSTEM;
}

string Message::typeToString(MessageType type) {
    switch (type) {
        case MessageType::PUBLIC: return "PUBLIC";
        case MessageType::PRIVATE: return "PRIVATE";
        case MessageType::SYSTEM: return "SYSTEM";
        default: return "UNKNOWN";
    }
} 