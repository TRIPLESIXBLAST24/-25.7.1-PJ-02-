#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <set>
#include "database.h"

using namespace std;

class Server {
private:
    int serverSocket;
    uint16_t port;
    Database db;
    atomic<bool> running{false};
    thread serverThread;
    set<int> clientSockets;
    mutex clientsMutex;
    
    void serverLoop();
    void handleClient(int clientSocket);
    string processRequest(const string& request);
    string serializeResponse(const string& status, const string& data = "");
    void sendToClient(int clientSocket, const string& message);
    string receiveFromClient(int clientSocket);
    
    string handleRegister(const string& login, const string& password, const string& name);
    string handleLogin(const string& login, const string& password);
    string handleSendMessage(const string& senderLogin, const string& recipientLogin, 
                            const string& text, const string& type);
    string handleGetUsers();
    string handleGetMessages(const string& login);

public:
    Server(uint16_t port, const string& dbPath = "chat.db");
    ~Server();
    
    bool start();
    void stop();
    bool isRunning() const { return running.load(); }
};

#endif

