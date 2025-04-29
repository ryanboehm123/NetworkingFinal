#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>

// Game settings
const unsigned short PORT = 53000;
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int PADDLE_HEIGHT = 100;
const int BALL_SIZE = 10;
const int WINNING_SCORE = 5;

struct GameState {
    float ballX, ballY;
    float ballVelX, ballVelY;
    float ball2X, ball2Y;
    float ball2VelX, ball2VelY;
    float paddle1Y, paddle2Y;
    int score1, score2;
    bool isGameOver;
};

void resetGame(GameState& state) {
    state.ballX = WINDOW_WIDTH / 2;
    state.ballY = WINDOW_HEIGHT / 2;
    state.ballVelX = 4.0f;
    state.ballVelY = 4.0f;
    state.ball2X = WINDOW_WIDTH / 2;
    state.ball2Y = WINDOW_HEIGHT / 2;
    state.ball2VelX = -4.0f;
    state.ball2VelY = 4.0f;
    state.paddle1Y = WINDOW_HEIGHT / 2 - PADDLE_HEIGHT / 2;
    state.paddle2Y = WINDOW_HEIGHT / 2 - PADDLE_HEIGHT / 2;
    state.score1 = 0;
    state.score2 = 0;
    state.isGameOver = false;
}

int main() {
    std::ofstream logFile("server_log.txt");
    logFile << "[Server] Starting server log...\n"; logFile.flush();

    sf::TcpListener listener;
    if (listener.listen(PORT) != sf::Socket::Done) {
        logFile << "[Server] Failed to start listener.\n"; logFile.flush();
        return -1;
    }

    sf::TcpSocket client1, client2;
    listener.accept(client1);
    listener.accept(client2);

    client1.setBlocking(false);
    client2.setBlocking(false);

    sf::SocketSelector selector;
    selector.add(client1);
    selector.add(client2);

    // Send player IDs
    sf::Packet idPacket;
    idPacket << 1;
    client1.send(idPacket);
    idPacket.clear();
    idPacket << 2;
    client2.send(idPacket);

    GameState state;
    resetGame(state);

    while (true) {
        while (!state.isGameOver) {
            if (selector.wait(sf::milliseconds(1))) {
                char move1;
                char move2;
                std::size_t received;
                if (selector.isReady(client1)) {
                    if (client1.receive(&move1, sizeof(move1), received) == sf::Socket::Done && received > 0) {
                        if (move1 == 'U') state.paddle1Y -= 5.f;
                        else if (move1 == 'D') state.paddle1Y += 5.f;
                    }
                }
                if (selector.isReady(client2)) {
                    if (client2.receive(&move2, sizeof(move2), received) == sf::Socket::Done && received > 0) {
                        if (move2 == 'U') state.paddle2Y -= 5.f;
                        else if (move2 == 'D') state.paddle2Y += 5.f;
                    }
                }
            }

            // Clamp paddles
            if (state.paddle1Y < 0.f) state.paddle1Y = 0.f;
            if (state.paddle1Y + PADDLE_HEIGHT > WINDOW_HEIGHT) state.paddle1Y = WINDOW_HEIGHT - PADDLE_HEIGHT;
            if (state.paddle2Y < 0.f) state.paddle2Y = 0.f;
            if (state.paddle2Y + PADDLE_HEIGHT > WINDOW_HEIGHT) state.paddle2Y = WINDOW_HEIGHT - PADDLE_HEIGHT;

            // Ball 1
            state.ballX += state.ballVelX;
            state.ballY += state.ballVelY;
            if (state.ballY <= 0 || state.ballY + BALL_SIZE >= WINDOW_HEIGHT) state.ballVelY = -state.ballVelY;
            if (state.ballX <= 40 && state.ballY + BALL_SIZE >= state.paddle1Y && state.ballY <= state.paddle1Y + PADDLE_HEIGHT) state.ballVelX = -state.ballVelX;
            if (state.ballX + BALL_SIZE >= WINDOW_WIDTH - 40 && state.ballY + BALL_SIZE >= state.paddle2Y && state.ballY <= state.paddle2Y + PADDLE_HEIGHT) state.ballVelX = -state.ballVelX;
            if (state.ballX < 0) {
                state.score2++;
                state.ballX = WINDOW_WIDTH / 2;
                state.ballY = WINDOW_HEIGHT / 2;
                state.ballVelX = 4.0f;
                state.ballVelY = 4.0f;
            }
            if (state.ballX > WINDOW_WIDTH) {
                state.score1++;
                state.ballX = WINDOW_WIDTH / 2;
                state.ballY = WINDOW_HEIGHT / 2;
                state.ballVelX = -4.0f;
                state.ballVelY = 4.0f;
            }

            // Ball 2
            state.ball2X += state.ball2VelX;
            state.ball2Y += state.ball2VelY;
            if (state.ball2Y <= 0 || state.ball2Y + BALL_SIZE >= WINDOW_HEIGHT) state.ball2VelY = -state.ball2VelY;
            if (state.ball2X <= 40 && state.ball2Y + BALL_SIZE >= state.paddle1Y && state.ball2Y <= state.paddle1Y + PADDLE_HEIGHT) state.ball2VelX = -state.ball2VelX;
            if (state.ball2X + BALL_SIZE >= WINDOW_WIDTH - 40 && state.ball2Y + BALL_SIZE >= state.paddle2Y && state.ball2Y <= state.paddle2Y + PADDLE_HEIGHT) state.ball2VelX = -state.ball2VelX;
            if (state.ball2X < 0) {
                state.score2++;
                state.ball2X = WINDOW_WIDTH / 2;
                state.ball2Y = WINDOW_HEIGHT / 2;
                state.ball2VelX = 4.0f;
                state.ball2VelY = 4.0f;
            }
            if (state.ball2X > WINDOW_WIDTH) {
                state.score1++;
                state.ball2X = WINDOW_WIDTH / 2;
                state.ball2Y = WINDOW_HEIGHT / 2;
                state.ball2VelX = -4.0f;
                state.ball2VelY = 4.0f;
            }

            if (state.score1 >= WINNING_SCORE || state.score2 >= WINNING_SCORE)
                state.isGameOver = true;

            // Send game state
            sf::Packet gameStatePacket;
            gameStatePacket << state.ballX << state.ballY << state.ball2X << state.ball2Y
                << state.paddle1Y << state.paddle2Y << state.score1 << state.score2 << state.isGameOver;
            client1.send(gameStatePacket);
            client2.send(gameStatePacket);

            sf::sleep(sf::milliseconds(16));
        }

        // Game Over: Announce Winner
        char winner = (state.score1 > state.score2) ? '1' : '2';
        sf::Packet winnerPacket;
        winnerPacket << static_cast<sf::Int8>(winner);
        client1.send(winnerPacket);
        client2.send(winnerPacket);

        sf::sleep(sf::seconds(1)); // Give clients time to display winner

        client1.setBlocking(true);
        client2.setBlocking(true);

        sf::Int8 ans1 = 'N', ans2 = 'N';
        bool received1 = false, received2 = false;

        while (!received1 || !received2) {
            if (!received1) {
                sf::Packet p1;
                if (client1.receive(p1) == sf::Socket::Done) {
                    sf::Int8 ans;
                    p1 >> ans;
                    ans1 = ans;
                    received1 = true;
                }
            }
            if (!received2) {
                sf::Packet p2;
                if (client2.receive(p2) == sf::Socket::Done) {
                    sf::Int8 ans;
                    p2 >> ans;
                    ans2 = ans;
                    received2 = true;
                }
            }
            sf::sleep(sf::milliseconds(10));
        }

        // Send restart signal
        sf::Packet signalPacket;
        if (ans1 == 'Y' && ans2 == 'Y') {
            signalPacket << static_cast<sf::Int8>('R'); // Restart signal
            client1.send(signalPacket);
            client2.send(signalPacket);

            resetGame(state);

            client1.setBlocking(false);
            client2.setBlocking(false);

            selector.clear();
            selector.add(client1);
            selector.add(client2);

            sf::sleep(sf::seconds(1));
        }
        else {
            signalPacket << static_cast<sf::Int8>('E'); // End game
            client1.send(signalPacket);
            client2.send(signalPacket);
            break;
        }
    }

    client1.disconnect();
    client2.disconnect();
    logFile << "[Server] Server shutdown.\n"; logFile.flush();
    return 0;
}
