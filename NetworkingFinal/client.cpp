// client.cpp
#include <iostream>
#include <thread>
#include <string>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#define SERVER_IP "127.0.0.1"
#define PORT 54000

void drawGame(int ballX, int ballY, int paddle1Y, int paddle2Y, int score1, int score2) {
    system("cls"); // "clear" for Linux/Mac
    std::cout << "Score: " << score1 << " - " << score2 << "\n";

    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 40; ++x) {
            if (x == ballX / 20 && y == ballY / 30) {
                std::cout << "O";
            }
            else if (x == 1 && y >= paddle1Y / 30 && y <= (paddle1Y + 100) / 30) {
                std::cout << "|";
            }
            else if (x == 38 && y >= paddle2Y / 30 && y <= (paddle2Y + 100) / 30) {
                std::cout << "|";
            }
            else {
                std::cout << " ";
            }
        }
        std::cout << "\n";
    }
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
    connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));

    char buffer[4096];

    float myPaddleY = 250;

    std::thread inputThread([&]() {
        while (true) {
            // Simple up/down control
            if (GetAsyncKeyState(VK_UP)) {
                myPaddleY -= 5;
            }
            if (GetAsyncKeyState(VK_DOWN)) {
                myPaddleY += 5;
            }
            std::string pos = std::to_string((int)myPaddleY) + "\n";
            send(sock, pos.c_str(), pos.size(), 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
        });

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) break;

        std::string msg(buffer);
        if (msg.find("WIN") != std::string::npos) {
            std::cout << "Game Over! ";
            if (msg.find("1") != std::string::npos) std::cout << "Player 1 wins!\n";
            else std::cout << "Player 2 wins!\n";
            break;
        }

        int ballX = 0, ballY = 0, paddle1Y = 0, paddle2Y = 0, score1 = 0, score2 = 0;
        sscanf_s(buffer, "BALL|%d|%d|PADDLE1|%d|PADDLE2|%d|SCORE|%d|%d",
            &ballX, &ballY, &paddle1Y, &paddle2Y, &score1, &score2);

        drawGame(ballX, ballY, paddle1Y, paddle2Y, score1, score2);
    }

    inputThread.detach();

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif

    return 0;
}