#include <iostream>
#include <limits>
#include <string>
#include "chat.h"
#include "server.h"
#include <vector>
#include <string>
#include <sstream>

using namespace std;

void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void showWelcomeMessage() {
    clearScreen();
    cout << "╔══════════════════════════════════════════════════════════════╗" << endl;
    cout << "║                    Welcome to STL Chat!                      ║" << endl;
    cout << "║                                                              ║" << endl;
    cout << "║  A modern chat application using Standard Template Library   ║" << endl;
    cout << "║                                                              ║" << endl;
    cout << "║  Features:                                                   ║" << endl;
    cout << "║  • User registration and authentication                      ║" << endl;
    cout << "║  • Public and private messaging                              ║" << endl;
    cout << "║  • Friend management                                         ║" << endl;
    cout << "║  • Chat rooms                                                ║" << endl;
    cout << "║  • Message search and tagging                                ║" << endl;
    cout << "║  • Real-time online status                                   ║" << endl;
    cout << "╚══════════════════════════════════════════════════════════════╝" << endl;
    cout << "\nPress Enter to continue...";
    cin.get();
}

int main(int argc, char** argv) {
    string mode;
    uint16_t serverPort = 8080;
    string serverHost = "127.0.0.1";
    uint16_t serverPortArg = 8080;
    
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--server" && i + 1 < argc) {
            mode = "server";
            serverPort = static_cast<uint16_t>(stoi(argv[++i]));
        } else if (arg == "--client" && i + 1 < argc) {
            mode = "client";
            string spec = argv[++i];
            size_t colon = spec.find(':');
            if (colon != string::npos) {
                serverHost = spec.substr(0, colon);
                serverPortArg = static_cast<uint16_t>(stoi(spec.substr(colon + 1)));
            } else {
                serverHost = spec;
            }
        }
    }
    
    if (mode == "server") {
        Server server(serverPort);
        if (!server.start()) {
            cerr << "Failed to start server!" << endl;
            return 1;
        }
        
        cout << "Server running on port " << serverPort << ". Press Enter to stop..." << endl;
        cin.get();
        
        server.stop();
        return 0;
    }
    
    if (mode.empty()) {
        mode = "client";
    }
    
    Chat chat;
    
    uint16_t clientPort = (mode == "client" && serverPortArg != 8080) ? serverPortArg : 8080;
    if (!chat.connectToServer(serverHost, clientPort)) {
        cout << "Failed to connect to server " << serverHost << ":" << clientPort << endl;
        cout << "Usage: --client <host:port> or --server <port>" << endl;
        return 1;
    }
    int choice;
    
    showWelcomeMessage();
    
    while (true) {
        clearScreen();
        cout << "\n╔════════════════════════════════════════════════════════════════╗" << endl;
        cout << "║                        Main Menu                               ║" << endl;
        cout << "╠════════════════════════════════════════════════════════════════╣" << endl;
        cout << "║  1. Registration                                               ║" << endl;
        cout << "║  2. Login                                                      ║" << endl;
        cout << "║  3. Exit                                                       ║" << endl;
        cout << "╚════════════════════════════════════════════════════════════════╝" << endl;
        cout << "\nChoose an action: ";
        
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "\nInvalid input! Please enter a number (1-3)." << endl;
            cout << "Press Enter to continue...";
            cin.get();
            continue;
        }
        
        switch (choice) {
            case 1:
                chat.registerUser();
                break;
            case 2:
                chat.login();
                break;
            case 3:
                clearScreen();
                cout << "\n╔════════════════════════════════════════════════════════════════╗" << endl;
                cout << "║                    Thank you for using STL Chat!               ║" << endl;
                cout << "║                                                                ║" << endl;
                cout << "║  Goodbye!                                                      ║" << endl;
                cout << "╚════════════════════════════════════════════════════════════════╝" << endl;
                chat.disconnectFromServer();
                return 0;
            default:
                cout << "\nInvalid choice! Please enter a number between 1 and 3." << endl;
                cout << "Press Enter to continue...";
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cin.get();
        }
        
        if (choice != 3) {
            cout << "\nPress Enter to return to main menu...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();
        }
    }
    
    return 0;
} 