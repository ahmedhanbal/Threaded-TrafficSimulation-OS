#include "i221271_i220999_D_vehicle.h" 
#include <pthread.h>
#include <map>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <sys/stat.h> 
#include <fcntl.h>   
#include <string.h>


struct ChallanMessage {
    char vehicleId[50];
    float speed;
    bool isHeavy;
};

class TrafficManager {
public:
    enum LightState {
        NORTH = 0,
        WEST = 1,
        SOUTH = 2,
        EAST = 3
    };
    sf::CircleShape lights[4];
    const float LIGHT_INTERVAL = 10.0f;
    float timer;
    LightState currentGreen;
    pthread_mutex_t* mutex;
    const float LIGHT_SIZE = 10.0f;
    const float YELLOW_DURATION = 2.0f;
    bool isYellow;

    sf::RenderWindow statsWindow;
    sf::Font font;
    sf::Text statsText;

    void startChallanProcess() {
        pid_t pid = fork();
        if (pid == 0) {
            execlp("./challan", "challan", nullptr);
            std::cerr << "Failed to execute challan program." << std::endl;
            exit(1);
        } else if (pid < 0) {
            std::cerr << "Fork failed for challan process." << std::endl;
        }
    }

    void startUserPortalProcess() {
        pid_t pid = fork();
        if (pid == 0) {
            execlp("./userportal", "userportal", nullptr);
            std::cerr << "Failed to execute user portal program." << std::endl;
            exit(1);
        } else if (pid < 0) {
            std::cerr << "Fork failed for user portal process." << std::endl;
        }
    }

    void startStripePaymentProcess() {
        pid_t pid = fork();
        if (pid == 0) {
            execlp("./stripepayment", "stripepayment", nullptr);
            std::cerr << "Failed to execute stripe payment program." << std::endl;
            exit(1);
        } else if (pid < 0) {
            std::cerr << "Fork failed for stripe payment process." << std::endl;
        }
    }

    TrafficManager(pthread_mutex_t* simulationMutex) 
        : timer(0.0f), currentGreen(NORTH), mutex(simulationMutex), isYellow(false) {
        startChallanProcess();
        startUserPortalProcess();
        startStripePaymentProcess();
        // Initialize lights
        for (int i = 0; i < 4; i++) {
            lights[i].setRadius(LIGHT_SIZE);
            lights[i].setOrigin(LIGHT_SIZE, LIGHT_SIZE);
            lights[i].setFillColor(sf::Color::Red);

            switch (i) {
                case NORTH: lights[i].setPosition(449, 418); break;
                case WEST: lights[i].setPosition(385, 418); break;
                case SOUTH: lights[i].setPosition(385, 478); break;
                case EAST: lights[i].setPosition(449, 478); break;
            }
        }
        lights[currentGreen].setFillColor(sf::Color::Green);

        // Stats window
        statsWindow.create(sf::VideoMode(400, 300), "SmartTraffix");
        statsWindow.setPosition({100, 200});
        if (!font.loadFromFile("res/CaskaydiaCove.ttf")) {
            return;
        }
        statsText.setFont(font);
        statsText.setCharacterSize(20);
        statsText.setFillColor(sf::Color::Black);
    }

    void update(float deltaTime) {
        timer += deltaTime;

        if (isYellow && timer >= YELLOW_DURATION) {
            lights[currentGreen].setFillColor(sf::Color::Red);
            currentGreen = static_cast<LightState>((currentGreen + 1) % 4);
            lights[currentGreen].setFillColor(sf::Color::Green);
            isYellow = false;
            timer = 0;
        } else if (!isYellow && timer >= LIGHT_INTERVAL) {
            lights[currentGreen].setFillColor(sf::Color::Yellow);
            isYellow = true;
            timer = 0;
        }
    }

    void draw(sf::RenderWindow& window) {
        for (int i = 0; i < 4; i++) {
            window.draw(lights[i]);
        }
    }

    bool isGreen(int direction) const {
        return direction == currentGreen && !isYellow;
    }

    void updateStats(const std::map<int, std::vector<Vehicle*>>& vehicles) {
        int counts[4] = {0}; // North, East, South, West
        int lightCount = 0, heavyCount = 0, emergencyCount = 0, challanCount = 0;

        for (const auto& directionVehicles : vehicles) {
            int direction = directionVehicles.first;
            const auto& vehicleList = directionVehicles.second;

            counts[direction] += vehicleList.size();

            for (const auto& vehicle : vehicleList) {
                if (vehicle->hasChallan) {
                    challanCount++;
                }
                if (vehicle->isHeavy) {
                    heavyCount++;
                } else if (vehicle->isEmergency) {
                    emergencyCount++;
                } else {
                    lightCount++;
                }
            }
        }

        std::stringstream ss;
        ss << "Vehicles Count:\n"
           << "North: " << counts[0] << "\n"
           << "East: " << counts[1] << "\n"
           << "South: " << counts[2] << "\n"
           << "West: " << counts[3] << "\n\n"
           << "Light Vehicles: " << lightCount << "\n"
           << "Heavy Vehicles: " << heavyCount << "\n"
           << "Emergency Vehicles: " << emergencyCount << "\n"
           << "Active Challans: " << challanCount;

        statsText.setString(ss.str());
    }

    void renderStats() {
        statsWindow.clear(sf::Color::White);
        statsWindow.draw(statsText);
        statsWindow.display();
    }

    void run() {
        while (statsWindow.isOpen()) {
            sf::Event event;
            while (statsWindow.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    statsWindow.close();
                }
            }
            renderStats();
        }
    }

    void updateAndRender(const std::map<int, std::vector<Vehicle*>>& vehicles) {
        updateStats(vehicles);
        renderStats();

        for (const auto& directionVehicles : vehicles) {
            for (const auto& vehicle : directionVehicles.second) {
                if ((vehicle->isHeavy && vehicle->currentSpeed > 40) || 
                    (!vehicle->isEmergency && vehicle->currentSpeed > 60)) {
                    vehicle->hasChallan = true;
                    issueChallan(vehicle->numberPlate, vehicle->currentSpeed, vehicle->isHeavy);
                }

                // accident logic probelamtic
                // for (const auto& otherDirectionVehicles : vehicles) {
                //     for (const auto& otherVehicle : otherDirectionVehicles.second) {
                //         if (vehicle != otherVehicle && 
                //             vehicle->getBoundingBox().intersects(otherVehicle->getBoundingBox())) {
                //             vehicle->hasCollision = true;
                //         }
                //     }
                // }

                // if (vehicle->hasViolation ) { //|| vehicle->hasCollision) {
                    
                    
                //     vehicle->hasViolation = false;
                //     vehicle->hasCollision = false;
                // }
            }
        }
    }

    void issueChallan(const std::string& vehicleId, float speed, bool isHeavy) {
        const char* fifoPath = "/tmp/challan_fifo";

        mkfifo(fifoPath, 0666);

        int fd = open(fifoPath, O_WRONLY);
        if (fd == -1) {
            std::cerr << "Failed to open FIFO for writing." << std::endl;
            return;
        }

        ChallanMessage msg;
        strncpy(msg.vehicleId, vehicleId.c_str(), sizeof(msg.vehicleId) - 1);
        msg.vehicleId[sizeof(msg.vehicleId) - 1] = '\0';
        msg.speed = speed;
        msg.isHeavy = isHeavy;

        write(fd, &msg, sizeof(msg));
        close(fd);
    }
};
