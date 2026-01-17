#include "chat.h"
#include <iostream>
#include <limits>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstring>
#include <cstdio>
#include <map>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

using namespace std;

Chat::Chat() : currentUser(nullptr), clientSocket(-1), serverPort(0) {}

void Chat::registerUser() {
    clearScreen();
    cout << "\n=== Registration ===" << endl;
    
    if (!connectedToServer) {
        cout << "Not connected to server! Please connect first." << endl;
        return;
    }
    
    string login;
    do {
        cout << "Enter login (3-20 characters): ";
        cin >> login;
        if (login.length() < 3 || login.length() > 20) {
            cout << "Login must be 3-20 characters long!" << endl;
        }
    } while (login.length() < 3 || login.length() > 20);
    
    string password;
    do {
        cout << "Enter password (6+ characters): ";
        cin >> password;
        if (password.length() < 6) {
            cout << "Password must be at least 6 characters!" << endl;
        }
    } while (password.length() < 6);
    
    string name;
    cout << "Enter name: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, name);
    
    if (name.empty()) {
        name = login;
    }

    string request = "REGISTER\n" + login + "\n" + password + "\n" + name;
    string response = sendRequestToServer(request);
    
    string status, data;
    if (parseServerResponse(response, status, data)) {
        if (status == "SUCCESS") {
            cout << "\nRegistration successful! Welcome, " << name << "!" << endl;
        } else {
            cout << "\nRegistration failed: " << data << endl;
        }
    } else {
        cout << "\nFailed to communicate with server!" << endl;
    }
}

void Chat::login() {
    clearScreen();
    cout << "\n=== Login ===" << endl;
    
    if (!connectedToServer) {
        cout << "Not connected to server! Please connect first." << endl;
        return;
    }
    
    string login;
    cout << "Enter login: ";
    cin >> login;
    
    string password;
    cout << "Enter password: ";
    cin >> password;
    
    string request = "LOGIN\n" + login + "\n" + password;
    string response = sendRequestToServer(request);
    
    string status, data;
    if (parseServerResponse(response, status, data)) {
        if (status == "SUCCESS") {
            size_t usersPos = data.find("USERS:");
            size_t messagesPos = data.find("MESSAGES:");
            
            if (usersPos != string::npos && messagesPos != string::npos) {
                string usersData = data.substr(usersPos + 6, messagesPos - usersPos - 6);
                string messagesData = data.substr(messagesPos + 9);
                
                loadUsersFromServer(usersData);
                loadMessagesFromServer(messagesData);
            }
            
            auto it = users.find(login);
            if (it != users.end()) {
                currentUser = &(it->second);
                currentUser->setOnlineStatus(true);
                onlineUsers.insert(login);
                
                cout << "Welcome back, " << currentUser->getName() << "!" << endl;
                chatMenu();
            } else {
                cout << "\nFailed to load user data!" << endl;
            }
        } else {
            cout << "\nInvalid login or password!" << endl;
        }
    } else {
        cout << "\nFailed to communicate with server!" << endl;
    }
}

void Chat::logout() {
    if (currentUser) {
        string name = currentUser->getName();
        currentUser->setOnlineStatus(false);
        onlineUsers.erase(currentUser->getLogin());
        currentUser = nullptr;
        sendSystemMessage(name + " has logged out.");
        cout << "Logged out successfully!" << endl;
    }
}

void Chat::chatMenu() {
    int choice;
    while (currentUser) {
        clearScreen();
        cout << "\n=== Chat Menu ===" << endl;
        cout << "User: " << currentUser->getName() << " (Online: " << onlineUsers.size() << ")" << endl;
        cout << "1. Send public message" << endl;
        cout << "2. Send private message" << endl;
        cout << "3. View messages" << endl;
        cout << "4. Search messages" << endl;
        cout << "5. Show online users" << endl;
        cout << "6. Manage friends" << endl;
        cout << "7. User profile" << endl;
        cout << "8. Chat rooms" << endl;
        cout << "9. Statistics" << endl;
        cout << "10. Logout" << endl;
        cout << "Choose an action: ";
        
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            choice = 0;
        }
        
        switch (choice) {
            case 1:
                sendPublicMessage();
                break;
            case 2:
                sendPrivateMessage();
                break;
            case 3:
                showMessages();
                break;
            case 4:
                searchMessages();
                break;
            case 5:
                showOnlineUsers();
                break;
            case 6:
                manageFriends();
                break;
            case 7:
                showUserProfile();
                break;
            case 8:
                showChatRoomMenu();
                break;
            case 9:
                showStatistics();
                break;
            case 10:
                logout();
                return;
            default:
                cout << "Invalid choice. Try again." << endl;
        }
        
        if (currentUser) {
            cout << "\nPress Enter to continue...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();
        }
    }
}

void Chat::sendPublicMessage() {
    if (!currentUser || !connectedToServer) return;
    
    string text;
    cout << "\n=== Send Public Message ===" << endl;
    cout << "Enter message: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, text);
    
    if (text.empty()) {
        cout << "Message cannot be empty!" << endl;
        return;
    }
    
    string request = "SEND_MESSAGE\n" + currentUser->getLogin() + "\n\n" + text + "\nPUBLIC";
    string response = sendRequestToServer(request);
    
    string status, data;
    if (parseServerResponse(response, status, data)) {
        if (status == "SUCCESS") {
            Message message(currentUser, nullptr, text, MessageType::PUBLIC);
            if (text.find("!important") != string::npos) {
                message.addTag("important");
            }
            if (text.find("?") != string::npos) {
                message.addTag("question");
            }
            messages.push_back(message);
            updateUserMessageIndex(message);
            cout << "Message sent successfully!" << endl;
        } else {
            cout << "Failed to send message: " << data << endl;
        }
    } else {
        cout << "Failed to communicate with server!" << endl;
    }
}

void Chat::sendPrivateMessage() {
    if (!currentUser) return;
    
    cout << "\n=== Send Private Message ===" << endl;
    
    vector<string> availableUsers;
    for (const auto& pair : users) {
        if (pair.first != currentUser->getLogin()) {
            availableUsers.push_back(pair.first);
        }
    }
    
    if (availableUsers.empty()) {
        cout << "No other users available!" << endl;
        return;
    }
    
    cout << "Available users:" << endl;
    for (size_t i = 0; i < availableUsers.size(); ++i) {
        cout << (i + 1) << ". " << availableUsers[i];
        if (isUserOnline(availableUsers[i])) {
            cout << " (Online)";
        }
        cout << endl;
    }
    
    int choice;
    cout << "Choose recipient (1-" << availableUsers.size() << "): ";
    if (!(cin >> choice) || choice < 1 || choice > static_cast<int>(availableUsers.size())) {
        cout << "Invalid choice!" << endl;
        return;
    }
    
    string recipientLogin = availableUsers[choice - 1];
    string text;
    cout << "Enter message: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, text);
    
    if (text.empty()) {
        cout << "Message cannot be empty!" << endl;
        return;
    }
    
    auto it = users.find(recipientLogin);
    if (it != users.end()) {
        if (connectedToServer) {
            string request = "SEND_MESSAGE\n" + currentUser->getLogin() + "\n" + recipientLogin + "\n" + text + "\nPRIVATE";
            string response = sendRequestToServer(request);
            
            string status, data;
            if (parseServerResponse(response, status, data)) {
                if (status == "SUCCESS") {
                    Message message(currentUser, &(it->second), text, MessageType::PRIVATE);
                    messages.push_back(message);
                    updateUserMessageIndex(message);
                    cout << "Private message sent to " << it->second.getName() << "!" << endl;
                } else {
                    cout << "Failed to send message: " << data << endl;
                }
            } else {
                cout << "Failed to communicate with server!" << endl;
            }
        } else {
            Message message(currentUser, &(it->second), text, MessageType::PRIVATE);
            messages.push_back(message);
            updateUserMessageIndex(message);
            cout << "Private message sent to " << it->second.getName() << "!" << endl;
        }
    }
}

void Chat::showMessages() {
    if (!currentUser) return;
    
    cout << "\n=== Messages ===" << endl;
    
    vector<Message> userMessages = getMessagesForUser(currentUser);
    
    if (userMessages.empty()) {
        cout << "No messages to display." << endl;
        return;
    }
    
    sort(userMessages.begin(), userMessages.end(), 
         [](const Message& a, const Message& b) {
             return a.getTimestamp() < b.getTimestamp();
         });
    
    for (const auto& message : userMessages) {
        cout << message.toString() << endl;
        cout << string(50, '-') << endl;
    }
}

void Chat::searchMessages() {
    if (!currentUser) return;
    
    cout << "\n=== Search Messages ===" << endl;
    cout << "1. Search by text" << endl;
    cout << "2. Search by tag" << endl;
    cout << "3. Search by sender" << endl;
    cout << "Choose search type: ";
    
    int choice;
    cin >> choice;
    
    string searchTerm;
    vector<Message> results;
    
    switch (choice) {
        case 1:
            cout << "Enter search text: ";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(cin, searchTerm);
            
            copy_if(messages.begin(), messages.end(), back_inserter(results),
                    [&](const Message& msg) {
                        return msg.getText().find(searchTerm) != string::npos;
                    });
            break;
            
        case 2:
            cout << "Enter tag: ";
            cin >> searchTerm;
            
            copy_if(messages.begin(), messages.end(), back_inserter(results),
                    [&](const Message& msg) {
                        return msg.hasTag(searchTerm);
                    });
            break;
            
        case 3:
            cout << "Enter sender login: ";
            cin >> searchTerm;
            
            copy_if(messages.begin(), messages.end(), back_inserter(results),
                    [&](const Message& msg) {
                        return msg.getSender() && msg.getSender()->getLogin() == searchTerm;
                    });
            break;
            
        default:
            cout << "Invalid choice!" << endl;
            return;
    }
    
    if (results.empty()) {
        cout << "No messages found." << endl;
        return;
    }
    
    cout << "\nFound " << results.size() << " message(s):" << endl;
    for (const auto& message : results) {
        cout << message.toString() << endl;
        cout << string(30, '-') << endl;
    }
}

void Chat::showOnlineUsers() {
    cout << "\n=== Online Users ===" << endl;
    if (onlineUsers.empty()) {
        cout << "No users online." << endl;
        return;
    }
    
    for (const auto& login : onlineUsers) {
        auto it = users.find(login);
        if (it != users.end()) {
            cout << "- " << it->second.getName() << " (" << login << ")" << endl;
        }
    }
}

void Chat::manageFriends() {
    if (!currentUser) return;
    
    int choice;
    do {
        cout << "\n=== Friends Management ===" << endl;
        cout << "1. View friends (" << currentUser->getFriendCount() << ")" << endl;
        cout << "2. Add friend" << endl;
        cout << "3. Remove friend" << endl;
        cout << "4. Back to main menu" << endl;
        cout << "Choose action: ";
        cin >> choice;
        
        switch (choice) {
            case 1: {
                const auto& friends = currentUser->getFriends();
                if (friends.empty()) {
                    cout << "You have no friends yet." << endl;
                } else {
                    cout << "Your friends:" << endl;
                    for (const auto& friendLogin : friends) {
                        auto it = users.find(friendLogin);
                        if (it != users.end()) {
                            cout << "- " << it->second.getName() << " (" << friendLogin << ")";
                            if (isUserOnline(friendLogin)) {
                                cout << " [Online]";
                            }
                            cout << endl;
                        }
                    }
                }
                break;
            }
            case 2: {
                string friendLogin;
                cout << "Enter friend's login: ";
                cin >> friendLogin;
                
                if (friendLogin == currentUser->getLogin()) {
                    cout << "You cannot add yourself as a friend!" << endl;
                } else if (users.find(friendLogin) == users.end()) {
                    cout << "User not found!" << endl;
                } else if (currentUser->hasFriend(friendLogin)) {
                    cout << "This user is already your friend!" << endl;
                } else {
                    currentUser->addFriend(friendLogin);
                    cout << "Friend added successfully!" << endl;
                }
                break;
            }
            case 3: {
                string friendLogin;
                cout << "Enter friend's login to remove: ";
                cin >> friendLogin;
                
                if (currentUser->hasFriend(friendLogin)) {
                    currentUser->removeFriend(friendLogin);
                    cout << "Friend removed successfully!" << endl;
                } else {
                    cout << "This user is not your friend!" << endl;
                }
                break;
            }
        }
    } while (choice != 4);
}

void Chat::showUserProfile() {
    if (!currentUser) return;
    
    cout << "\n=== User Profile ===" << endl;
    cout << "Name: " << currentUser->getName() << endl;
    cout << "Login: " << currentUser->getLogin() << endl;
    cout << "Friends: " << currentUser->getFriendCount() << endl;
    cout << "Status: " << (currentUser->getOnlineStatus() ? "Online" : "Offline") << endl;
    
    auto userMessages = getMessagesForUser(currentUser);
    cout << "Messages sent: " << count_if(userMessages.begin(), userMessages.end(),
                                         [this](const Message& msg) {
                                             return msg.getSender() == currentUser;
                                         }) << endl;
}

void Chat::showChatRoomMenu() {
    if (!currentUser) return;
    
    int choice;
    do {
        cout << "\n=== Chat Rooms ===" << endl;
        cout << "1. Create new room" << endl;
        cout << "2. Join existing room" << endl;
        cout << "3. View room members" << endl;
        cout << "4. Back to main menu" << endl;
        cout << "Choose action: ";
        cin >> choice;
        
        switch (choice) {
            case 1:
                createChatRoom();
                break;
            case 2:
                joinChatRoom();
                break;
            case 3:
                showChatRoomMembers();
                break;
        }
    } while (choice != 4);
}

void Chat::createChatRoom() {
    if (!currentUser) return;
    
    string roomName;
    cout << "Enter room name: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, roomName);
    
    if (roomName.empty()) {
        cout << "Room name cannot be empty!" << endl;
        return;
    }
    
    if (chatRooms.find(roomName) != chatRooms.end()) {
        cout << "Room with this name already exists!" << endl;
        return;
    }
    
    chatRooms[roomName].insert(currentUser->getLogin());
    cout << "Chat room '" << roomName << "' created successfully!" << endl;
    sendSystemMessage("New chat room '" + roomName + "' created by " + currentUser->getName());
}

void Chat::joinChatRoom() {
    if (!currentUser) return;
    
    if (chatRooms.empty()) {
        cout << "No chat rooms available." << endl;
        return;
    }
    
    cout << "Available chat rooms:" << endl;
    int i = 1;
    for (const auto& room : chatRooms) {
        cout << i++ << ". " << room.first << " (" << room.second.size() << " members)" << endl;
    }
    
    int choice;
    cout << "Choose room to join: ";
    if (!(cin >> choice) || choice < 1 || choice > static_cast<int>(chatRooms.size())) {
        cout << "Invalid choice!" << endl;
        return;
    }
    
    auto it = chatRooms.begin();
    advance(it, choice - 1);
    
    if (it->second.find(currentUser->getLogin()) != it->second.end()) {
        cout << "You are already a member of this room!" << endl;
    } else {
        it->second.insert(currentUser->getLogin());
        cout << "Successfully joined room '" << it->first << "'!" << endl;
        sendSystemMessage(currentUser->getName() + " joined room '" + it->first + "'");
    }
}

void Chat::showChatRoomMembers() {
    if (chatRooms.empty()) {
        cout << "No chat rooms available." << endl;
        return;
    }
    
    cout << "Chat room members:" << endl;
    for (const auto& room : chatRooms) {
        cout << "\nRoom: " << room.first << endl;
        for (const auto& memberLogin : room.second) {
            auto it = users.find(memberLogin);
            if (it != users.end()) {
                cout << "  - " << it->second.getName() << " (" << memberLogin << ")";
                if (isUserOnline(memberLogin)) {
                    cout << " [Online]";
                }
                cout << endl;
            }
        }
    }
}

void Chat::sendSystemMessage(const string& text) {
    Message systemMessage(nullptr, nullptr, text, MessageType::SYSTEM);
    messages.push_back(systemMessage);
}

void Chat::updateUserMessageIndex(const Message& message) {
    if (message.getSender()) {
        userMessageIndex[message.getSender()->getLogin()].push_back(messages.size() - 1);
    }
    if (message.getRecipient()) {
        userMessageIndex[message.getRecipient()->getLogin()].push_back(messages.size() - 1);
    }
}

vector<Message> Chat::getMessagesForUser(const User* user) const {
    vector<Message> userMessages;
    
    for (const auto& message : messages) {
        if (message.getRecipient() == nullptr || 
            message.getRecipient() == user || 
            message.getSender() == user ||
            message.getType() == MessageType::SYSTEM) {
            userMessages.push_back(message);
        }
    }
    
    return userMessages;
}

vector<Message> Chat::getMessagesByTag(const string& tag) const {
    vector<Message> taggedMessages;
    
    copy_if(messages.begin(), messages.end(), back_inserter(taggedMessages),
            [&](const Message& msg) {
                return msg.hasTag(tag);
            });
    
    return taggedMessages;
}

void Chat::processMessageQueue() {
    while (!messageQueue.empty()) {
        Message msg = messageQueue.front();
        messageQueue.pop();
        messages.push_back(msg);
        updateUserMessageIndex(msg);
    }
}

bool Chat::isValidInput(const string& input) const {
    return !input.empty() && input.length() <= 1000;
}

void Chat::clearScreen() const {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void Chat::showStatistics() const {
    cout << "\n=== Chat Statistics ===" << endl;
    cout << "Total users: " << getUserCount() << endl;
    cout << "Online users: " << onlineUsers.size() << endl;
    cout << "Total messages: " << getMessageCount() << endl;
    cout << "Chat rooms: " << chatRooms.size() << endl;
    
    // Статистика по типам сообщений
    int publicCount = 0, privateCount = 0, systemCount = 0;
    for (const auto& msg : messages) {
        switch (msg.getType()) {
            case MessageType::PUBLIC: publicCount++; break;
            case MessageType::PRIVATE: privateCount++; break;
            case MessageType::SYSTEM: systemCount++; break;
        }
    }
    
    cout << "Public messages: " << publicCount << endl;
    cout << "Private messages: " << privateCount << endl;
    cout << "System messages: " << systemCount << endl;
}

size_t Chat::getUserCount() const {
    return users.size();
}

size_t Chat::getMessageCount() const {
    return messages.size();
}

const vector<string>& Chat::getOnlineUsers() const {
    static vector<string> result;
    result.clear();
    result.insert(result.end(), onlineUsers.begin(), onlineUsers.end());
    return result;
}

bool Chat::isUserOnline(const string& login) const {
    return onlineUsers.find(login) != onlineUsers.end();
} 


static int close_socket_portable(int s) {
#ifdef _WIN32
    return closesocket(s);
#else
    return close(s);
#endif
}

bool Chat::connectToServer(const string& host, uint16_t port) {
    if (connectedToServer) {
        disconnectFromServer();
    }
    
    serverHost = host;
    serverPort = port;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return false;
    }
#endif

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cerr << "Failed to create socket" << endl;
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
        // Попытка резолва через DNS (упрощенно)
        serverAddr.sin_addr.s_addr = inet_addr(host.c_str());
        if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
            cerr << "Invalid server address: " << host << endl;
            close_socket_portable(clientSocket);
            clientSocket = -1;
            return false;
        }
    }

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Failed to connect to server " << host << ":" << port << endl;
        close_socket_portable(clientSocket);
        clientSocket = -1;
        return false;
    }

    connectedToServer = true;
    cout << "Connected to server " << host << ":" << port << endl;
    return true;
}

void Chat::disconnectFromServer() {
    if (clientSocket >= 0) {
        close_socket_portable(clientSocket);
        clientSocket = -1;
    }
    connectedToServer = false;
#ifdef _WIN32
    WSACleanup();
#endif
}

string Chat::sendRequestToServer(const string& request) {
    if (!connectedToServer || clientSocket < 0) {
        return "STATUS:ERROR\nDATA:Not connected to server";
    }
    
    string fullRequest = request + "\nEND\n";
    if (send(clientSocket, fullRequest.c_str(), static_cast<int>(fullRequest.length()), 0) < 0) {
        return "STATUS:ERROR\nDATA:Failed to send request";
    }
    
    return receiveFromClient(clientSocket);
}

string Chat::receiveFromClient(int socket) {
    string response;
    char buffer[4096];
    
    while (true) {
        int bytesReceived = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) break;
        
        buffer[bytesReceived] = '\0';
        response += buffer;
        
        if (response.find("\nEND\n") != string::npos) {
            break;
        }
    }
    
    size_t endPos = response.find("\nEND\n");
    if (endPos != string::npos) {
        response = response.substr(0, endPos);
    }
    
    return response;
}

bool Chat::parseServerResponse(const string& response, string& status, string& data) {
    size_t statusPos = response.find("STATUS:");
    size_t dataPos = response.find("DATA:");
    
    if (statusPos == string::npos || dataPos == string::npos) {
        return false;
    }
    
    status = response.substr(statusPos + 7, dataPos - statusPos - 8);
    data = response.substr(dataPos + 5);
    
    while (!status.empty() && (status.back() == '\n' || status.back() == '\r')) {
        status.pop_back();
    }
    
    return true;
}

void Chat::loadUsersFromServer(const string& usersData) {
    if (usersData.empty()) return;
    
    stringstream ss(usersData);
    string userPair;
    
    while (getline(ss, userPair, '|')) {
        size_t colonPos = userPair.find(':');
        if (colonPos != string::npos) {
            string login = userPair.substr(0, colonPos);
            string name = userPair.substr(colonPos + 1);
            
            if (users.find(login) == users.end()) {
                users.emplace(login, User(login, "", name));
            }
        }
    }
}

void Chat::loadMessagesFromServer(const string& messagesData) {
    if (messagesData.empty()) return;
    
    messages.clear();
    stringstream ss(messagesData);
    string line;
    
    while (getline(ss, line)) {
        if (line.empty()) continue;
        
        stringstream msgStream(line);
        string senderLogin, recipientLogin, text, type, timestampStr;
        
        getline(msgStream, senderLogin, '|');
        getline(msgStream, recipientLogin, '|');
        getline(msgStream, text, '|');
        getline(msgStream, type, '|');
        getline(msgStream, timestampStr, '|');
        
        const User* sender = nullptr;
        const User* recipient = nullptr;
        
        if (!senderLogin.empty()) {
            auto it = users.find(senderLogin);
            if (it != users.end()) {
                sender = &(it->second);
            }
        }
        
        if (!recipientLogin.empty()) {
            auto it = users.find(recipientLogin);
            if (it != users.end()) {
                recipient = &(it->second);
            }
        }
        
        MessageType msgType = MessageType::PUBLIC;
        if (type == "PRIVATE") msgType = MessageType::PRIVATE;
        else if (type == "SYSTEM") msgType = MessageType::SYSTEM;
        
        Message message(sender, recipient, text, msgType);
        messages.push_back(message);
        updateUserMessageIndex(message);
    }
}