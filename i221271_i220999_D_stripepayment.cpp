#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

class StripePayment {
public:
    sf::RenderWindow window;
    sf::Font font;
    sf::Text inputText;
    sf::Text statusText;
    sf::Text challanDetailsText;
    std::string challanId;
    std::string vehicleNumber;
    std::string vehicleType;
    float amount;
    std::string inputAmount;
    std::string creditCardNumber;
    bool isPaid;

    StripePayment(const std::string& challanId, const std::string& vehicleNumber, const std::string& vehicleType, float amount)
        : challanId(challanId), vehicleNumber(vehicleNumber), vehicleType(vehicleType), amount(amount),isPaid(false) {
        window.create(sf::VideoMode(400, 300), "Stripe Payment");
        window.setPosition({1000,500});
        if (!font.loadFromFile("res/CaskaydiaCove.ttf")) {
            std::cerr << "Error loading font!" << std::endl;
        }
        inputText.setFont(font);
        inputText.setCharacterSize(20);
        inputText.setFillColor(sf::Color::Black);
        inputText.setPosition(0, 100);

        statusText.setFont(font);
        statusText.setCharacterSize(20);
        statusText.setFillColor(sf::Color::Red);
        statusText.setPosition(0, 150);

        challanDetailsText.setFont(font);
        challanDetailsText.setCharacterSize(20);
        challanDetailsText.setFillColor(sf::Color::Black);
        challanDetailsText.setPosition(0, 50);
        challanDetailsText.setString("Challan ID: " + challanId + "\nAmount: " + std::to_string(amount) + " PKR");
    }
    void handleInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode < 128) {
                    if (event.text.unicode == '\b') {
                        if (!inputAmount.empty()) {
                            inputAmount.pop_back();
                        }
                    } else {
                        inputAmount += static_cast<char>(event.text.unicode);
                    }
                }
            }
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
                processPayment();
                if(isPaid){
                    return;
                }
            }
        }
    }

    void processPayment() {
        float paidAmount = std::stof(inputAmount);
        if (paidAmount >= amount) {
            statusText.setString("Payment Successful!");
            isPaid = true;
            notifyUserPortal(challanId, vehicleNumber);
        } else {
            statusText.setString("Insufficient Amount!");
        }
    }

    void notifyUserPortal(const std::string& challanId, const std::string& vehicleNumber) {
        const char* challanFifoPath = "/tmp/challan_payment_fifo";
        const char* userPortalFifoPath = "/tmp/userportal_payment_fifo";

        mkfifo(challanFifoPath, 0666);
        mkfifo(userPortalFifoPath, 0666);

        int fdChallan = open(challanFifoPath, O_WRONLY);
        int fdUserPortal = open(userPortalFifoPath, O_WRONLY);

        if (fdChallan != -1) {
            std::string paymentStatus = challanId + "," + vehicleNumber + ",Paid";
            write(fdChallan, paymentStatus.c_str(), paymentStatus.size());
            close(fdChallan);
        }

        if (fdUserPortal != -1) {
            std::string paymentStatus = challanId + "," + vehicleNumber + ",Paid";
            write(fdUserPortal, paymentStatus.c_str(), paymentStatus.size());
            close(fdUserPortal);
        }
    }



    void run() {
        while (window.isOpen() && ! isPaid) {
            handleInput();
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
            }
            inputText.setString("Enter Payment Amount: " + inputAmount);

            window.clear(sf::Color::White);
            window.draw(challanDetailsText);
            window.draw(inputText);
            window.draw(statusText);
            window.display();
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <challanId> <vehicleNumber> <vehicleType> <amount>" << std::endl;
        return 1;
    }

    std::string challanId = argv[1];
    std::string vehicleNumber = argv[2];
    std::string vehicleType = argv[3];
    float amount = std::stof(argv[4]);

    StripePayment stripePayment(challanId, vehicleNumber, vehicleType, amount);
    stripePayment.run();
    return 0;
}