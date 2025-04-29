#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>

// Game settings
const char* SERVER_IP = "127.0.0.1";
const unsigned short SERVER_PORT = 53000;
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int PADDLE_WIDTH = 10;
const int PADDLE_HEIGHT = 100;
const int BALL_SIZE = 10;

int main() {
    sf::TcpSocket socket;
    if (socket.connect(SERVER_IP, SERVER_PORT) != sf::Socket::Done) {
        std::cerr << "Failed to connect to server.\n";
        return -1;
    }
    socket.setBlocking(true);
    std::cout << "Connected to server.\n";

    // Receive player ID
    sf::Packet packet;
    if (socket.receive(packet) != sf::Socket::Done) {
        std::cerr << "Failed to receive player ID.\n";
        return -1;
    }
    int playerID;
    packet >> playerID;
    std::cout << "You are Player " << playerID << "\n";

    socket.setBlocking(false);

    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Multiball Pong Multiplayer");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Failed to load font!\n";
        return -1;
    }

    sf::Text scoreText1, scoreText2;
    scoreText1.setFont(font);
    scoreText1.setCharacterSize(30);
    scoreText1.setFillColor(sf::Color::White);
    scoreText1.setPosition(WINDOW_WIDTH / 4, 20);

    scoreText2.setFont(font);
    scoreText2.setCharacterSize(30);
    scoreText2.setFillColor(sf::Color::White);
    scoreText2.setPosition(WINDOW_WIDTH * 3 / 4, 20);

    sf::Text winnerText;
    winnerText.setFont(font);
    winnerText.setCharacterSize(40);
    winnerText.setFillColor(sf::Color::White);
    winnerText.setPosition(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 100);

    sf::Text rematchText;
    rematchText.setFont(font);
    rematchText.setCharacterSize(30);
    rematchText.setFillColor(sf::Color::White);
    rematchText.setPosition(WINDOW_WIDTH / 2 - 180, WINDOW_HEIGHT / 2);

    sf::RectangleShape paddle(sf::Vector2f(PADDLE_WIDTH, PADDLE_HEIGHT));
    paddle.setFillColor(sf::Color::White);

    sf::RectangleShape opponent(sf::Vector2f(PADDLE_WIDTH, PADDLE_HEIGHT));
    opponent.setFillColor(sf::Color::White);

    sf::CircleShape ball(BALL_SIZE);
    ball.setFillColor(sf::Color::White);

    sf::CircleShape ball2(BALL_SIZE);
    ball2.setFillColor(sf::Color::White);

    if (playerID == 1) {
        paddle.setPosition(30.f, WINDOW_HEIGHT / 2 - PADDLE_HEIGHT / 2);
        opponent.setPosition(WINDOW_WIDTH - 40.f, WINDOW_HEIGHT / 2 - PADDLE_HEIGHT / 2);
    }
    else {
        paddle.setPosition(WINDOW_WIDTH - 40.f, WINDOW_HEIGHT / 2 - PADDLE_HEIGHT / 2);
        opponent.setPosition(30.f, WINDOW_HEIGHT / 2 - PADDLE_HEIGHT / 2);
    }

    bool isGameOver = false;
    bool waitingForRematch = false;
    bool waitingForServerRestart = false;
    bool answerSent = false;
    char playerAnswer = '\0';

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (waitingForRematch && !answerSent && event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Y) {
                    playerAnswer = 'Y';
                    sf::Packet answerPacket;
                    answerPacket << static_cast<sf::Int8>(playerAnswer);
                    socket.send(answerPacket);

                    answerSent = true;
                    waitingForServerRestart = true;
                }
                else if (event.key.code == sf::Keyboard::N) {
                    playerAnswer = 'N';
                    sf::Packet answerPacket;
                    answerPacket << static_cast<sf::Int8>(playerAnswer);
                    socket.send(answerPacket);

                    answerSent = true;
                    waitingForServerRestart = true;
                }
            }
        }

        // Handle different gameplay states
        if (!isGameOver && !waitingForRematch) {
            // Normal gameplay phase
            char move = 'N';
            if (playerID == 1) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
                    move = 'U';
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
                    move = 'D';
            }
            else {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                    move = 'U';
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                    move = 'D';
            }

            socket.send(&move, sizeof(move));

            sf::Packet recvPacket;
            if (socket.receive(recvPacket) == sf::Socket::Done) {
                float ballX, ballY, ball2X, ball2Y, p1Y, p2Y;
                int score1, score2;
                bool gameOverFlag;
                recvPacket >> ballX >> ballY >> ball2X >> ball2Y >> p1Y >> p2Y >> score1 >> score2 >> gameOverFlag;

                ball.setPosition(ballX, ballY);
                ball2.setPosition(ball2X, ball2Y);
                scoreText1.setString(std::to_string(score1));
                scoreText2.setString(std::to_string(score2));

                if (playerID == 1) {
                    paddle.setPosition(30.f, p1Y);
                    opponent.setPosition(WINDOW_WIDTH - 40.f, p2Y);
                }
                else {
                    paddle.setPosition(WINDOW_WIDTH - 40.f, p2Y);
                    opponent.setPosition(30.f, p1Y);
                }

                if (gameOverFlag)
                    isGameOver = true;
            }
        }
        else if (isGameOver && !waitingForRematch) {
            // First time match ended — wait for winner info
            sf::Packet winnerPacket;
            if (socket.receive(winnerPacket) == sf::Socket::Done) {
                sf::Int8 winnerInt;
                winnerPacket >> winnerInt;
                char winner = static_cast<char>(winnerInt);

                std::string winnerStr = "Player ";
                winnerStr += winner;
                winnerStr += " wins!";
                winnerText.setString(winnerStr);

                rematchText.setString("Press Y to play again or N to exit");

                waitingForRematch = true;
                answerSent = false;
            }
        }
        else if (waitingForRematch && waitingForServerRestart) {
            // Waiting for server's rematch signal
            sf::Packet signalPacket;
            if (socket.receive(signalPacket) == sf::Socket::Done) {
                sf::Int8 signal;
                signalPacket >> signal;

                if (signal == 'R') {
                    isGameOver = false;
                    waitingForRematch = false;
                    waitingForServerRestart = false;
                    answerSent = false;
                    playerAnswer = '\0';
                }
                else if (signal == 'E') {
                    window.close();
                }
            }
        }

        // Always draw something!
        window.clear(sf::Color::Black);

        if (!waitingForRematch) {
            window.draw(paddle);
            window.draw(opponent);
            window.draw(ball);
            window.draw(ball2);
            window.draw(scoreText1);
            window.draw(scoreText2);
        }
        else {
            window.draw(winnerText);
            window.draw(rematchText);
        }

        window.display();
    }

    socket.disconnect();
    return 0;
}
