#include "server.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>

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

static int close_socket_portable(int s) {
#ifdef _WIN32
    return closesocket(s);
#else
    return close(s);
#endif
}

Server::Server(uint16_t port, const string& dbPath) : serverSocket(-1), port(port), db(dbPath) {
}

Server::~Server() {
    stop();
}

bool Server::start() {
    if (running.load()) return true;
    
    if (!db.initialize()) {
        cerr << "Failed to initialize database!" << endl;
        return false;
    }
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return false;
    }
#endif

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cerr << "Failed to create socket" << endl;
        return false;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (::bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Failed to bind socket on port " << port << endl;
        close_socket_portable(serverSocket);
        serverSocket = -1;
        return false;
    }

    if (listen(serverSocket, 10) < 0) {
        cerr << "Failed to listen on socket" << endl;
        close_socket_portable(serverSocket);
        serverSocket = -1;
        return false;
    }

    running.store(true);
    serverThread = thread(&Server::serverLoop, this);
    
    cout << "Server started on port " << port << endl;
    return true;
}

void Server::stop() {
    if (!running.load()) return;
    
    running.store(false);
    
    if (serverSocket >= 0) {
        close_socket_portable(serverSocket);
        serverSocket = -1;
    }
    
    {
        lock_guard<mutex> lock(clientsMutex);
        for (int sock : clientSockets) {
            close_socket_portable(sock);
        }
        clientSockets.clear();
    }
    
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    cout << "Server stopped" << endl;
}

void Server::serverLoop() {
    while (running.load()) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            if (running.load()) {
                cerr << "Failed to accept client" << endl;
            }
            continue;
        }
        
        {
            lock_guard<mutex> lock(clientsMutex);
            clientSockets.insert(clientSocket);
        }
        
        thread clientThread(&Server::handleClient, this, clientSocket);
        clientThread.detach();
    }
}

void Server::handleClient(int clientSocket) {
    try {
        while (running.load()) {
            string request = receiveFromClient(clientSocket);
            if (request.empty()) break;
            
            string response = processRequest(request);
            sendToClient(clientSocket, response);
        }
    } catch (...) {
    }
    
    close_socket_portable(clientSocket);
    {
        lock_guard<mutex> lock(clientsMutex);
        clientSockets.erase(clientSocket);
    }
}

string Server::receiveFromClient(int clientSocket) {
    string request;
    char buffer[4096];
    
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) break;
        
        buffer[bytesReceived] = '\0';
        request += buffer;
        
        if (request.find("\nEND\n") != string::npos) {
            break;
        }
    }
    
    size_t endPos = request.find("\nEND\n");
    if (endPos != string::npos) {
        request = request.substr(0, endPos);
    }
    
    return request;
}

void Server::sendToClient(int clientSocket, const string& message) {
    string msg = message + "\nEND\n";
    send(clientSocket, msg.c_str(), static_cast<int>(msg.length()), 0);
}

string Server::serializeResponse(const string& status, const string& data) {
    return "STATUS:" + status + "\nDATA:" + data;
}

string Server::processRequest(const string& request) {
    stringstream ss(request);
    string command;
    getline(ss, command);
    
    if (command == "REGISTER") {
        string login, password, name;
        getline(ss, login);
        getline(ss, password);
        getline(ss, name);
        return handleRegister(login, password, name);
    }
    else if (command == "LOGIN") {
        string login, password;
        getline(ss, login);
        getline(ss, password);
        return handleLogin(login, password);
    }
    else if (command == "SEND_MESSAGE") {
        string senderLogin, recipientLogin, text, type;
        getline(ss, senderLogin);
        getline(ss, recipientLogin);
        getline(ss, text);
        getline(ss, type);
        return handleSendMessage(senderLogin, recipientLogin, text, type);
    }
    else if (command == "GET_USERS") {
        return handleGetUsers();
    }
    else if (command == "GET_MESSAGES") {
        string login;
        getline(ss, login);
        return handleGetMessages(login);
    }
    else {
        return serializeResponse("ERROR", "Unknown command");
    }
}

string Server::handleRegister(const string& login, const string& password, const string& name) {
    if (db.addUser(login, password, name)) {
        return serializeResponse("SUCCESS", "User registered");
    } else {
        return serializeResponse("ERROR", "User already exists");
    }
}

string Server::handleLogin(const string& login, const string& password) {
    if (db.checkUserPassword(login, password)) {
        string usersData = handleGetUsers();
        string messagesData = handleGetMessages(login);
        return serializeResponse("SUCCESS", "USERS:" + usersData + "\nMESSAGES:" + messagesData);
    } else {
        return serializeResponse("ERROR", "Invalid login or password");
    }
}

string Server::handleSendMessage(const string& senderLogin, const string& recipientLogin, 
                                 const string& text, const string& type) {
    MessageData msg;
    msg.senderLogin = senderLogin;
    msg.recipientLogin = recipientLogin;
    msg.text = text;
    msg.type = type;
    msg.timestamp = chrono::duration_cast<chrono::milliseconds>(
        chrono::system_clock::now().time_since_epoch()).count();
    
    if (db.addMessage(msg)) {
        return serializeResponse("SUCCESS", "Message sent");
    } else {
        return serializeResponse("ERROR", "Failed to send message");
    }
}

string Server::handleGetUsers() {
    vector<UserData> users = db.getAllUsers();
    stringstream ss;
    for (size_t i = 0; i < users.size(); ++i) {
        if (i > 0) ss << "|";
        ss << users[i].login << ":" << users[i].name;
    }
    return ss.str();
}

string Server::handleGetMessages(const string& login) {
    vector<MessageData> messages = db.getMessagesForUser(login);
    stringstream ss;
    for (size_t i = 0; i < messages.size(); ++i) {
        if (i > 0) ss << "\n";
        ss << messages[i].senderLogin << "|"
           << messages[i].recipientLogin << "|"
           << messages[i].text << "|"
           << messages[i].type << "|"
           << messages[i].timestamp;
    }
    return ss.str();
}

