#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <fcntl.h>   
#include <unistd.h>   
#include <sys/stat.h> 
#include <SFML/Graphics.hpp>

struct ChallanMessage {
    char vehicleId[50];
    float speed;
    bool isHeavy;
};

struct ActiveChallan {
    std::string vehicleId;
    float amount;
    std::string issueDate;
    std::string dueDate;
};

class Challan {
public:
    sf::RenderWindow window; 
    sf::Font font; 
    sf::Text totalChallansText; 
    sf::Text currentChallanText; 
    int totalChallans; 
    std::vector<ActiveChallan> activeChallans; 

    Challan() : totalChallans(0) {
        window.create(sf::VideoMode(400, 300), "Challan Tracker");
        window.setPosition({100,700});
        if (!font.loadFromFile("res/CaskaydiaCove.ttf")) {
            std::cerr << "Error loading font!" << std::endl;
        }
        totalChallansText.setFont(font);
        totalChallansText.setCharacterSize(20);
        totalChallansText.setFillColor(sf::Color::Black);

        currentChallanText.setFont(font);
        currentChallanText.setCharacterSize(20);
        currentChallanText.setFillColor(sf::Color::Red);
        currentChallanText.setPosition(0,30);
    }
    
    void calculateFine(bool isHeavy, float& fine, float& serviceCharge) {
        fine = isHeavy ? 7000.0f : 5000.0f;
        serviceCharge = fine * 0.17f; // 17% service charge
    }

    void setDates(std::string& issueDate, std::string& dueDate) {
        time_t now = time(0);
        struct tm* timeinfo = localtime(&now);

        // Issue date
        issueDate = std::asctime(timeinfo);
        issueDate.erase(issueDate.length() - 1); 

        // Due date
        timeinfo->tm_mday += 3; // Add 3 days
        mktime(timeinfo); 
        dueDate = std::asctime(timeinfo);
        dueDate.erase(dueDate.length() - 1); 
    }

    bool isChallanDuplicate(const std::string& vehicleId) {
        for (const auto& challan : activeChallans) {
            if (challan.vehicleId == vehicleId) {
                return true;
            }
        }
        return false;
    }

    void processChallan(const ChallanMessage& msg) {
        if (isChallanDuplicate(msg.vehicleId)) {
            return; // Ignore duplicate challans
        }

        float fine, serviceCharge;
        calculateFine(msg.isHeavy, fine, serviceCharge);

        std::string issueDate, dueDate;
        setDates(issueDate, dueDate);

        totalChallans++;

        currentChallanText.setString("Vehicle ID: " + std::string(msg.vehicleId) +
                                     "\nAmount: " + std::to_string(fine + serviceCharge) + " PKR");

        totalChallansText.setString("Total Challans: " + std::to_string(totalChallans));

        activeChallans.push_back({msg.vehicleId, fine + serviceCharge, issueDate, dueDate});

        // Send challan details to UserPortal
        const char* userPortalFifoPath = "/tmp/userportal_fifo";
        mkfifo(userPortalFifoPath, 0666);
        int fd = open(userPortalFifoPath, O_WRONLY);
        if (fd != -1) {
            std::string challanDetails = msg.vehicleId; 
            challanDetails += "," + std::to_string(fine + serviceCharge) + "," + issueDate + "," + dueDate;
            write(fd, challanDetails.c_str(), challanDetails.size());
            close(fd);
        }

        sf::Clock clock;
        sf::Event e;
        while (clock.getElapsedTime().asSeconds() < 2) {
            while(window.pollEvent(e)){
                if(e.type == sf::Event::Closed){
                    window.close();
                    return;
                }
            }
            window.clear(sf::Color::White);
            window.draw(totalChallansText);
            window.draw(currentChallanText);
            window.display();
        }
    }

    void run() {
        const char* fifoPath = "/tmp/challan_fifo";
        const char* paymentFifoPath = "/tmp/challan_payment_fifo";

        mkfifo(fifoPath, 0666);
        mkfifo(paymentFifoPath, 0666);

        int fd = open(fifoPath, O_RDONLY | O_NONBLOCK);
        int fdPayment = open(paymentFifoPath, O_RDONLY | O_NONBLOCK);

        if (fd == -1 || fdPayment == -1) {
            std::cerr << "Failed to open FIFO for reading." << std::endl;
            return;
        }

        ChallanMessage msg;
        char paymentBuffer[256];
        while (true) {
            int bytesRead = read(fd, &msg, sizeof(msg));
            if (bytesRead == sizeof(msg)) {
                processChallan(msg);
            }

            int paymentBytesRead = read(fdPayment, paymentBuffer, sizeof(paymentBuffer));
            if (paymentBytesRead > 0) {
                paymentBuffer[paymentBytesRead] = '\0';
                std::string paymentStatus(paymentBuffer);
                std::istringstream iss(paymentStatus);
                std::string challanId, vehicleNumber, status;
                std::getline(iss, challanId, ',');
                std::getline(iss, vehicleNumber, ',');
                std::getline(iss, status, ',');

                if (status == "Paid") {
                    activeChallans.erase(std::remove_if(activeChallans.begin(), activeChallans.end(),
                        [&vehicleNumber](const ActiveChallan& c) { return c.vehicleId == vehicleNumber; }),
                        activeChallans.end());
                    totalChallans--;
                }
            }

            usleep(100000); 
        }

        close(fd);
        close(fdPayment);
    }
};

int main() {
    Challan challan;
    challan.run();
    return 0;
}