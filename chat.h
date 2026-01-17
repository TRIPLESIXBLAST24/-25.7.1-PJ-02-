#ifndef CHAT_H
#define CHAT_H

#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <queue>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include "user.h"
#include "message.h"

using namespace std;

class Chat {
private:
    unordered_map<string, User> users;
    vector<Message> messages;
    User* currentUser;
    
    set<string> onlineUsers;
    map<string, vector<size_t>> userMessageIndex;
    queue<Message> messageQueue;
    unordered_map<string, set<string>> chatRooms;
    
    int clientSocket = -1;
    string serverHost;
    uint16_t serverPort;
    bool connectedToServer = false;
    
    void chatMenu();
    void sendPublicMessage();
    void sendPrivateMessage();
    void showMessages();
    void showUserProfile();
    void manageFriends();
    void showOnlineUsers();
    void searchMessages();
    void createChatRoom();
    void joinChatRoom();
    void showChatRoomMenu();
    void showChatRoomMembers();
    void sendSystemMessage(const string& text);
    
    void updateUserMessageIndex(const Message& message);
    vector<Message> getMessagesForUser(const User* user) const;
    vector<Message> getMessagesByTag(const string& tag) const;
    void processMessageQueue();
    bool isValidInput(const string& input) const;
    void clearScreen() const;
    
    string sendRequestToServer(const string& request);
    string receiveFromClient(int socket);
    bool parseServerResponse(const string& response, string& status, string& data);
    void loadUsersFromServer(const string& usersData);
    void loadMessagesFromServer(const string& messagesData);

public:
    Chat();
    
    void registerUser();
    void login();
    void logout();
    
    void showStatistics() const;
    void exportMessages(const string& filename) const;
    void importMessages(const string& filename);
    void backupUsers(const string& filename) const;
    void restoreUsers(const string& filename);
    
    bool connectToServer(const string& host, uint16_t port);
    void disconnectFromServer();
    bool isConnected() const { return connectedToServer; }
    
    size_t getUserCount() const;
    size_t getMessageCount() const;
    const vector<string>& getOnlineUsers() const;
    bool isUserOnline(const string& login) const;
};

#endif