#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sstream>
#include <map>


struct ChallanDetails {
    std::string challanId;
    std::string vehicleNumber;
    std::string vehicleType;
    std::string paymentStatus;
    float amount;
};

class UserPortal {
public:
    sf::RenderWindow window;
    sf::Font font;
    sf::Text displayText;
    sf::Text inputText;
    std::vector<ChallanDetails> challans;
    std::map<std::string, ChallanDetails> challanMap;
    std::string inputVehicleId;
    bool correct;

    UserPortal() {
        window.create(sf::VideoMode(600, 400), "User Portal");
        window.setPosition({1000, 100});
        
        if (!font.loadFromFile("res/CaskaydiaCove.ttf")) {
            std::cerr << "Error loading font!" << std::endl;
        }

        displayText.setFont(font);
        displayText.setCharacterSize(20);
        displayText.setFillColor(sf::Color::Black);

        inputText.setFont(font);
        inputText.setCharacterSize(20);
        inputText.setFillColor(sf::Color::Blue);
        inputText.setPosition(300,100);
    }

    void handleInput() {
        correct = false;
        sf::Event event;
        while (window.pollEvent(event) && !correct) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode < 128) {
                    if (event.text.unicode == '\b') { 
                        if (!inputVehicleId.empty()) {
                            inputVehicleId.pop_back();
                        }
                    } else {
                        inputVehicleId += char(event.text.unicode);
                    }
                }
            }
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
                processChallanPayment();
            }
        }
    }

    void processChallanPayment() {
        if (challanMap.find(inputVehicleId) != challanMap.end()) {
            ChallanDetails& challan = challanMap[inputVehicleId];
            correct = true;
            std::string command = "./stripepayment " + challan.challanId + " " + challan.vehicleNumber + " " + challan.vehicleType + " " + std::to_string(challan.amount);
            int pid = fork();
            if (pid == 0) {
                execlp("./stripepayment", "stripepayment", challan.challanId.c_str(), challan.vehicleNumber.c_str(), challan.vehicleType.c_str(), std::to_string(challan.amount).c_str(), nullptr);
            }
            else{
                wait(NULL);
                return;
            }
        } else {
            displayText.setString("Challan does not exist for vehicle ID: " + inputVehicleId);
            correct = false;
        }
    }

    void addChallan(const ChallanDetails& challan) {
        challans.push_back(challan);
        challanMap[challan.vehicleNumber] = challan;
    }

    void displayChallans() {
        std::stringstream ss;
        for (const auto& challan : challans) {
            ss << "Challan ID: " << challan.challanId << "\n";
        }
        displayText.setString(ss.str());
    }

    void run() {
        const char* userPortalFifoPath = "/tmp/userportal_fifo";
        const char* paymentFifoPath = "/tmp/userportal_payment_fifo";
        mkfifo(userPortalFifoPath, 0666);
        mkfifo(paymentFifoPath, 0666);

        int fd = open(userPortalFifoPath, O_RDONLY | O_NONBLOCK);
        int fdPayment = open(paymentFifoPath, O_RDONLY | O_NONBLOCK);

        if (fd == -1 || fdPayment == -1) {
            std::cerr << "Failed to open FIFO for reading." << std::endl;
            return;
        }

        char buffer[256];
        while (window.isOpen()) {
            handleInput();

            int bytesRead = read(fd, buffer, sizeof(buffer));
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string challanDetails(buffer);
                std::istringstream iss(challanDetails);
                std::string vehicleId, amount, issueDate, dueDate;
                std::getline(iss, vehicleId, ',');
                std::getline(iss, amount, ',');
                std::getline(iss, issueDate, ',');
                std::getline(iss, dueDate, ',');

                ChallanDetails challan = {vehicleId, vehicleId, "Unknown", "Unpaid", std::stof(amount)};
                addChallan(challan);
            }

            int paymentBytesRead = read(fdPayment, buffer, sizeof(buffer));
            if (paymentBytesRead > 0) {
                buffer[paymentBytesRead] = '\0';
                std::string paymentStatus(buffer);
                std::istringstream iss(paymentStatus);
                std::string challanId, vehicleNumber, status;
                std::getline(iss, challanId, ',');
                std::getline(iss, vehicleNumber, ',');
                std::getline(iss, status, ',');

                if (status == "Paid") {
                    challanMap.erase(vehicleNumber);
                    challans.erase(std::remove_if(challans.begin(), challans.end(),
                        [&vehicleNumber](const ChallanDetails& c) { return c.vehicleNumber == vehicleNumber; }),
                        challans.end());
                }
            }

            displayChallans();
            inputText.setString("Enter Vehicle ID: " + inputVehicleId);

            window.clear(sf::Color::White);
            window.draw(displayText);
            window.draw(inputText);
            window.display();
        }

        close(fd);
        close(fdPayment);
    }
};

int main() {
    UserPortal userPortal;
    userPortal.run();
    return 0;
}