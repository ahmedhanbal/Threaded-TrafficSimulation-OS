#include "vehiclespawner.h"
#include "trafficmanager.h"
#include <semaphore.h>
#include <iomanip>

struct ThreadData {
    std::vector<Vehicle*>* vehicles;
    VehicleSpawner* spawner;
    int direction;
    bool* running;
    pthread_mutex_t* mutex;
    float* simulationTime;
    TrafficManager* trafficManager;
    sem_t* intersectionSem;
};

class Simulation {
public:
    pthread_t threads[4];
    pthread_t trafficThread;
    pthread_mutex_t vehicleMutex;
    sem_t intersectionSemaphore;
    bool isRunning;
    ThreadData threadData[4];
    std::map<int, std::vector<Vehicle*>> directionVehicles;
    TrafficManager trafficManager;
    time_t simulationStartTime;
    float timeMultiplier;
    sf::Font font;
    sf::Text timeText;
    sf::VideoMode resolution;
    sf::RenderWindow window;
    VehicleSpawner spawner;
    sf::Clock clock;
    float simulationTime;

    Simulation() : 
        resolution(WIDTH, HEIGHT),
        window(resolution, "SmartTraffix"),
        simulationTime(0.0f),
        isRunning(true),
        trafficManager(&vehicleMutex) {
        
        // limit fps to 60, more than enough for traffic sim
        window.setFramerateLimit(60);
        pthread_mutex_init(&vehicleMutex, NULL);
        sem_init(&intersectionSemaphore, 0, 1);
        
        // setup data for each direction thread
        for(int i = 0; i < 4; i++) {
            directionVehicles[i] = std::vector<Vehicle*>();
            threadData[i].vehicles = &directionVehicles[i];
            threadData[i].spawner = &spawner;
            threadData[i].direction = i;
            threadData[i].running = &isRunning;
            threadData[i].mutex = &vehicleMutex;
            threadData[i].simulationTime = &simulationTime;
            threadData[i].trafficManager = &trafficManager;
            threadData[i].intersectionSem = &intersectionSemaphore;
        }
    }
    
    void initializeTime() {
        if (!font.loadFromFile("res/CaskaydiaCove.ttf")) {
            return;
        }
        // Setup mock time display
        sf::Text Mock;
        Mock.setFont(font);
        Mock.setCharacterSize(50);
        Mock.setFillColor(sf::Color::Black);
        Mock.setPosition(WIDTH / 2 - 150, 200);
        Mock.setString("MOCK TIME");
        // Instructions for user
        sf::Text instructions;
        instructions.setFont(font);
        instructions.setCharacterSize(20);
        instructions.setFillColor(sf::Color::Black);
        instructions.setPosition(150, HEIGHT / 2 - 100);
        instructions.setString( "Up/Down to set hours, Left/Right to set mins.\n"
                               "\t\tEnter to confirm.");
        // Display time selection
        sf::Text hourText, minuteText;
        hourText.setFont(font);
        hourText.setCharacterSize(30);
        hourText.setFillColor(sf::Color::Black);
        hourText.setPosition(180, HEIGHT / 2 - 150);
        minuteText.setFont(font);
        minuteText.setCharacterSize(30);
        minuteText.setFillColor(sf::Color::Black);
        minuteText.setPosition(hourText.getPosition().x + 250, HEIGHT / 2 - 150 );

        int hours = 0, minutes = 0;
        bool confirmed = false;

        while (window.isOpen() && !confirmed) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                    return;
                }
                // Handle keyboard input
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Up) {
                        hours = (hours + 1) % 24;
                    } else if (event.key.code == sf::Keyboard::Down) {
                        hours = (hours == 0) ? 23 : hours - 1;
                    } else if (event.key.code == sf::Keyboard::Right) {
                        minutes = (minutes + 1) % 60;
                    } else if (event.key.code == sf::Keyboard::Left) {
                        minutes = (minutes == 0) ? 59 : minutes - 1;
                    } else if (event.key.code == sf::Keyboard::Enter) {
                        confirmed = true;
                    }
                }
            }

            std::stringstream hourStream, minuteStream;
            hourStream << "Hours: " << std::setfill('0') << std::setw(2) << hours;
            minuteStream << "Minutes: " << std::setfill('0') << std::setw(2) << minutes;
            hourText.setString(hourStream.str());
            minuteText.setString(minuteStream.str());

            window.clear(sf::Color::White);
            window.draw(instructions);
            window.draw(hourText);
            window.draw(minuteText);
            window.draw(Mock);
            window.display();
        }

        // Set simulation time
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        timeinfo->tm_hour = hours;
        timeinfo->tm_min = minutes;
        timeinfo->tm_sec = 0;
        simulationStartTime = mktime(timeinfo);
        timeMultiplier = 60.0f;  // 1 sec = 1 min
        spawner.setCurrentTime(simulationStartTime);

        // Setup time display
        timeText.setFont(font);
        timeText.setCharacterSize(24);
        timeText.setFillColor(sf::Color::Black);
        timeText.setPosition(10, 10);
    }   

    void updateSimulationTime(float deltaTime) {
        // update the fake time - multiply by 60 bc 1sec = 1min
        simulationStartTime += time_t(deltaTime * timeMultiplier);
        spawner.setCurrentTime(simulationStartTime);
        
        // tome format
        struct tm* timeinfo = localtime(&simulationStartTime);
        std::stringstream ss;
        ss << "Time: " 
           << std::setfill('0') << std::setw(2) << timeinfo->tm_hour << ":"
           << std::setfill('0') << std::setw(2) << timeinfo->tm_min << ":"
           << std::setfill('0') << std::setw(2) << timeinfo->tm_sec;
        timeText.setString(ss.str());
    }

    static void* trafficControlThread(void* arg) {
        Simulation* sim = (Simulation*)arg;
        sf::Clock threadClock;
        
        // main loop for traffic lights
        while(sim->isRunning && sim->simulationTime < SIMTIME) {
            float deltaTime = threadClock.restart().asSeconds();
            
            // need mutex here to safely update traffic lights
            pthread_mutex_lock(&sim->vehicleMutex);
            sim->trafficManager.update(deltaTime);
            sim->trafficManager.updateAndRender(sim->directionVehicles);
            pthread_mutex_unlock(&sim->vehicleMutex);
            
            // small sleep to prevent thread from hogging cpu
            sf::sleep(sf::milliseconds(16));
        }
        return NULL;
    }


    static void updateVehicles(ThreadData* data, float deltaTime) {
        bool isGreenLight = data->trafficManager->isGreen(data->direction);
        
        auto it = data->vehicles->begin();
        while(it != data->vehicles->end()) {
            Vehicle* currentVehicle = *it;
            
            float minSafeSpeed = currentVehicle->maxSpeed;
            for(auto& otherVehicle : *(data->vehicles)) {
                if(currentVehicle != otherVehicle && 
                   currentVehicle->lane == otherVehicle->lane) {
                    sf::Vector2f pos1 = currentVehicle->veh.getPosition();
                    sf::Vector2f pos2 = otherVehicle->veh.getPosition();
                    float distance = 0;
                    
                    switch(data->direction) {
                        case 0:
                            distance = pos2.y - pos1.y;
                            break;
                        case 1:
                            distance = pos2.x - pos1.x;
                            break;
                        case 2:
                            distance = pos1.y - pos2.y;
                            break;
                        case 3:
                            distance = pos1.x - pos2.x;
                            break;
                    }
                    float SAFE_DISTANCE;
                    switch(currentVehicle->direction){
                        case 0:
                        case 2:
                            SAFE_DISTANCE = currentVehicle->veh.getGlobalBounds().height * 1.5f;
                            break;
                        case 1:
                        case 3:
                            SAFE_DISTANCE = currentVehicle->veh.getGlobalBounds().width * 1.5f;
                            break;
                    }
                    if (currentVehicle->isHeavy) {
                        SAFE_DISTANCE *= 0.75f;
                    }
                    
                    if(distance > 0 && distance < SAFE_DISTANCE) {
                        minSafeSpeed = std::min(minSafeSpeed, otherVehicle->currentSpeed * 0.5f);
                    }
                }
            }
            
            currentVehicle->currentSpeed = minSafeSpeed;
            currentVehicle->update(deltaTime, isGreenLight);
            
            sf::Vector2f pos = currentVehicle->veh.getPosition();
            if(pos.x < -50 || pos.x > WIDTH + 50 || 
               pos.y < -50 || pos.y > HEIGHT + 50) {
                data->spawner->decrementLaneCount(data->direction, currentVehicle->lane);
                
                if (!currentVehicle->isEmergency) {
                    std::string vehicleType = currentVehicle->isHeavy ? "Heavy" : "Light";
                    if (!data->spawner->isQueueFull(currentVehicle->direction)) {
                        data->spawner->addToPendingQueue(vehicleType, currentVehicle->direction);
                    }
                }
                
                delete currentVehicle;
                it = data->vehicles->erase(it);
            } else {
                ++it;
            }
        }
    }

    static void spawnVehicles(ThreadData* data, float deltaTime) {
        if (data->spawner->hasPendingVehicles(data->direction)) {
            PendingVehicle pending = data->spawner->getNextPendingVehicle(data->direction);
            int lane;
            
            if (pending.type == "Heavy") {
                if (data->spawner->isLaneAvailable(data->direction, 2)) {
                    Vehicle* vehicle = new Vehicle(pending.type, data->direction, 2);
                    data->vehicles->push_back(vehicle);
                    data->spawner->incrementLaneCount(data->direction, 2);
                } else {
                    data->spawner->addToPendingQueue(pending.type, data->direction);
                }
            } else {
                lane = data->spawner->getLeastOccupiedLane(data->direction);
                if (data->spawner->isLaneAvailable(data->direction, lane)) {
                    Vehicle* vehicle = new Vehicle(pending.type, data->direction, lane);
                    data->vehicles->push_back(vehicle);
                    data->spawner->incrementLaneCount(data->direction, lane);
                } else {
                    data->spawner->addToPendingQueue(pending.type, data->direction);
                }
            }
        }
        
        if(data->spawner->shouldSpawnHeavyVehicle(data->direction, deltaTime)) {
            if (data->spawner->isLaneAvailable(data->direction, 2)) {
                Vehicle* vehicle = new Vehicle("Heavy", data->direction, 2);
                data->vehicles->push_back(vehicle);
                data->spawner->incrementLaneCount(data->direction, 2);
            }
        }
        else if(data->spawner->shouldSpawnEmergency(data->direction, deltaTime)) {
            int lane = data->spawner->getLeastOccupiedLane(data->direction);
            if (data->spawner->isLaneAvailable(data->direction, lane)) {
                Vehicle* vehicle = new Vehicle("Emergency", data->direction, lane);
                data->vehicles->push_back(vehicle);
                data->spawner->incrementLaneCount(data->direction, lane);
            }
        }
        else if(data->spawner->shouldSpawnRegular(data->direction, deltaTime)) {
            int lane = data->spawner->getLeastOccupiedLane(data->direction);
            if (data->spawner->isLaneAvailable(data->direction, lane)) {
                Vehicle* vehicle = new Vehicle("Light", data->direction, lane);
                data->vehicles->push_back(vehicle);
                data->spawner->incrementLaneCount(data->direction, lane);
            }
        }
    }

    static void* directionThread(void* arg) {
        ThreadData* data = (ThreadData*)arg;
        sf::Clock threadClock;
        
        while(*(data->running) && *(data->simulationTime) < SIMTIME) {
            float deltaTime = threadClock.restart().asSeconds();
            
            pthread_mutex_lock(data->mutex);
            
            spawnVehicles(data, deltaTime);
            updateVehicles(data, deltaTime);
            
            pthread_mutex_unlock(data->mutex);
            
            sf::sleep(sf::milliseconds(16));
        }
        return NULL;
    }

    void start() {
        initializeTime();
        sf::Texture texBack;
        texBack.loadFromFile("res/background.jpg");
        sf::Sprite background(texBack);        
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&trafficThread, &attr, trafficControlThread, this);
        pthread_attr_destroy(&attr);
        
        for(int i = 0; i < 4; i++) {
            pthread_create(&threads[i], NULL, directionThread, &threadData[i]);
        }
        
        sf::Event e;
        while(window.isOpen() && simulationTime < SIMTIME) {
            float deltaTime = clock.restart().asSeconds();
            simulationTime += deltaTime;
            updateSimulationTime(deltaTime);
                 
            while(window.pollEvent(e)) {
                if(e.type == sf::Event::Closed) {
                    window.close();
                }
            }
            
            window.clear(sf::Color::White);
            window.draw(background);
            
            pthread_mutex_lock(&vehicleMutex);
            for(const auto& pair : directionVehicles) {
                for(const auto& vehicle : pair.second) {
                    window.draw(vehicle->veh);
                }
            }
            trafficManager.draw(window);
            pthread_mutex_unlock(&vehicleMutex);
            
            window.draw(timeText);
            
            window.display();
        }
    }

    ~Simulation() {
        isRunning = false;
        pthread_join(trafficThread, NULL);
        for(int i = 0; i < 4; i++) {
            pthread_join(threads[i], NULL);
        }
        
        pthread_mutex_destroy(&vehicleMutex);
        sem_destroy(&intersectionSemaphore);
        
        for(auto& pair : directionVehicles) {
            for(auto vehicle : pair.second) {
                delete vehicle;
            }
        }
    }
};


