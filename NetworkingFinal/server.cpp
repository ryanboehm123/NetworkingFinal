// server.cpp
#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <vector>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#define PORT 54000
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PADDLE_HEIGHT 100
#define BALL_SIZE 10

struct GameState {
    float ballX = SCREEN_WIDTH / 2;
    float ballY = SCREEN_HEIGHT / 2;
    float ballVX = 5.0f;
    float ballVY = 5.0f;
    float paddle1Y = SCREEN_HEIGHT / 2;
    float paddle2Y = SCREEN_HEIGHT / 2;
    int score1 = 0;
    int score2 = 0;
};

void sendToBoth(int client1, int client2, const std::string& msg) {
    send(client1, msg.c_str(), msg.size(), 0);
    send(client2, msg.c_str(), msg.size(), 0);
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 2);

    std::cout << "Waiting for players...\n";
    int client1 = accept(serverSocket, nullptr, nullptr);
    std::cout << "Player 1 connected!\n";
    int client2 = accept(serverSocket, nullptr, nullptr);
    std::cout << "Player 2 connected!\n";

    GameState game;

    char buffer[1024];

    while (game.score1 < 5 && game.score2 < 5) {
        // 1. Receive paddle movement
        memset(buffer, 0, sizeof(buffer));
        recv(client1, buffer, sizeof(buffer), 0);
        if (strlen(buffer) > 0) {
            game.paddle1Y = std::stof(buffer);
        }

        memset(buffer, 0, sizeof(buffer));
        recv(client2, buffer, sizeof(buffer), 0);
        if (strlen(buffer) > 0) {
            game.paddle2Y = std::stof(buffer);
        }

        // 2. Update ball
        game.ballX += game.ballVX;
        game.ballY += game.ballVY;

        // Bounce off top/bottom walls
        if (game.ballY < 0 || game.ballY > SCREEN_HEIGHT) {
            game.ballVY *= -1;
        }

        // Bounce off paddles
        if (game.ballX < 20 &&
            game.ballY > game.paddle1Y &&
            game.ballY < game.paddle1Y + PADDLE_HEIGHT) {
            game.ballVX *= -1;
        }
        if (game.ballX > SCREEN_WIDTH - 20 &&
            game.ballY > game.paddle2Y &&
            game.ballY < game.paddle2Y + PADDLE_HEIGHT) {
            game.ballVX *= -1;
        }

        // Scoring
        if (game.ballX < 0) {
            game.score2++;
            game.ballX = SCREEN_WIDTH / 2;
            game.ballY = SCREEN_HEIGHT / 2;
        }
        if (game.ballX > SCREEN_WIDTH) {
            game.score1++;
            game.ballX = SCREEN_WIDTH / 2;
            game.ballY = SCREEN_HEIGHT / 2;
        }

        // 3. Send game state
        std::string message = "BALL|" + std::to_string((int)game.ballX) + "|" + std::to_string((int)game.ballY) +
            "|PADDLE1|" + std::to_string((int)game.paddle1Y) +
            "|PADDLE2|" + std::to_string((int)game.paddle2Y) +
            "|SCORE|" + std::to_string(game.score1) + "|" + std::to_string(game.score2) + "\n";
        sendToBoth(client1, client2, message);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // Game Over
    std::string winMessage = "WIN|";
    winMessage += (game.score1 == 5) ? "1\n" : "2\n";
    sendToBoth(client1, client2, winMessage);

#ifdef _WIN32
    closesocket(serverSocket);
    closesocket(client1);
    closesocket(client2);
    WSACleanup();
#else
    close(serverSocket);
    close(client1);
    close(client2);
#endif

    return 0;
}