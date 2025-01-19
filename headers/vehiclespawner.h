#include "vehicle.h"

struct PendingVehicle {
    std::string type;
    int direction;
    float priority;  // Higher number = higher priority
    
    bool operator<(const PendingVehicle& other) const {
        return priority < other.priority; 
    }
};

class VehicleSpawner {
private:
    std::vector<float> spawnTimers;
    std::vector<float> emergencyTimers;
    std::vector<float> heavyVehicleTimers;
    time_t currentTime;
    std::vector<std::vector<int>> laneCounts;  // [direction][lane]
    std::vector<std::priority_queue<PendingVehicle>> pendingVehicles;  // One queue per direction
    std::vector<int> queueCounts;  // Track number of vehicles in queue per direction
    static const int MAX_QUEUE_SIZE = MAX_VEHICLES_PER_LANE;  // Same as lane capacity

    bool isSpawnAreaClear(int direction, int lane) const {
        if (!vehicles[direction]) return true;  // Safety check
        
        const float SPAWN_SAFE_DISTANCE = 100.0f;  // Increased safe distance for spawning
        sf::Vector2f spawnPoint(SPAWN_POINTS[direction].x, SPAWN_POINTS[direction].y);
        
        //=the last spawned vehicle's position in this direction
        for (const auto& vehicle : *vehicles[direction]) {
            if (vehicle->lane != lane) continue;
            
            sf::Vector2f vehiclePos = vehicle->veh.getPosition();
            float distance = 0;
            
            switch(direction) {
                case 0: // North
                    distance = spawnPoint.y - vehiclePos.y;
                    break;
                case 1: // West
                    distance = vehiclePos.x - spawnPoint.x;
                    break;
                case 2: // South
                    distance = vehiclePos.y - spawnPoint.y;
                    break;
                case 3: // East
                    distance = spawnPoint.x - vehiclePos.x;
                    break;
            }
            
            if (distance < SPAWN_SAFE_DISTANCE && distance > -SPAWN_SAFE_DISTANCE) {
                return false;
            }
        }
        return true;
    }

public:
    std::vector<std::vector<Vehicle*>*> vehicles;  // Reference to vehicles from simulation

    VehicleSpawner() {
        spawnTimers = std::vector<float>(4, 0.0f);
        emergencyTimers = std::vector<float>(4, 0.0f);
        heavyVehicleTimers = std::vector<float>(4, 0.0f);
        laneCounts = std::vector<std::vector<int>>(4, std::vector<int>(2, 0));
        pendingVehicles = std::vector<std::priority_queue<PendingVehicle>>(4);
        currentTime = time(nullptr);
        queueCounts = std::vector<int>(4, 0);
        vehicles = std::vector<std::vector<Vehicle*>*>(4, nullptr);
    }

    void setVehicles(std::vector<std::vector<Vehicle*>*>& vehiclesList) {
        vehicles = vehiclesList;
    }

    void incrementLaneCount(int direction, int lane) {
        laneCounts[direction][lane-1]++;
    }

    void decrementLaneCount(int direction, int lane) {
        laneCounts[direction][lane-1]--;
    }

    bool isLaneAvailable(int direction, int lane) {
        return laneCounts[direction][lane-1] < MAX_VEHICLES_PER_LANE;
    }

    int getLeastOccupiedLane(int direction) {
        if (laneCounts[direction][0] == laneCounts[direction][1]) {
            return (rand() % 2) + 1;  // Random lane if equal
        }
        return (laneCounts[direction][0] < laneCounts[direction][1]) ? 1 : 2;
    }

    bool isHeavyVehicleAllowed() {
        // Check if current time is during peak hours
        struct tm* timeinfo = localtime(&currentTime);
        int timeOfDay = timeinfo->tm_hour * 3600 + timeinfo->tm_min * 60 + timeinfo->tm_sec;

        bool isMorningPeak = (timeOfDay >= TIME_7AM && timeOfDay <= TIME_930AM);
        bool isEveningPeak = (timeOfDay >= TIME_430PM && timeOfDay <= TIME_830PM);

        return !(isMorningPeak || isEveningPeak);
    }

    bool shouldSpawnHeavyVehicle(int direction, float deltaTime) {
        if (!isHeavyVehicleAllowed()) return false;

        heavyVehicleTimers[direction] += deltaTime;
        if(heavyVehicleTimers[direction] >= (15.0f + rand() % 10)) {
            heavyVehicleTimers[direction] = 0;
            if (isLaneAvailable(direction, 2) && isSpawnAreaClear(direction, 2)) {
                return true;
            } else if (!isQueueFull(direction)) {
                addToPendingQueue("Heavy", direction);
            }
        }
        return false;
    }

    bool shouldSpawnEmergency(int direction, float deltaTime) {
        emergencyTimers[direction] += deltaTime;
        
        float emergencyInterval;
        float emergencyChance;
        
        switch(direction) {
            case 0: // North
                emergencyInterval = 15.0f;
                emergencyChance = 0.20f;
                break;
            case 1: // West
                emergencyInterval = 15.0f;
                emergencyChance = 0.30f;
                break;
            case 2: // South
                emergencyInterval = 15.0f;
                emergencyChance = 0.05f;
                break;
            case 3: // East
                emergencyInterval = 20.0f;
                emergencyChance = 0.10f;
                break;
        }

        if(emergencyTimers[direction] >= emergencyInterval) {
            emergencyTimers[direction] = 0;
            if ((float(rand()) / RAND_MAX) < emergencyChance) {
                if (!isLaneAvailable(direction, 1) && !isLaneAvailable(direction, 2) && !isQueueFull(direction)) {
                    addToPendingQueue("Emergency", direction);
                }
                return (isLaneAvailable(direction, 1) || isLaneAvailable(direction, 2));
            }
        }
        return false;
    }

    bool shouldSpawnRegular(int direction, float deltaTime) {
        spawnTimers[direction] += deltaTime;
        
        float spawnInterval;
        switch(direction) {
            case 0: spawnInterval = 1.0f; break;
            case 1: spawnInterval = 2.0f; break;
            case 2: spawnInterval = 2.0f; break;
            case 3: spawnInterval = 1.5f; break;
        }

        if(spawnTimers[direction] >= spawnInterval) {
            spawnTimers[direction] = 0;
            if (!isLaneAvailable(direction, 1) && !isLaneAvailable(direction, 2) && !isQueueFull(direction)) {
                addToPendingQueue("Light", direction);
            }
            return (isLaneAvailable(direction, 1) || isLaneAvailable(direction, 2));
        }
        return false;
    }

    void addToPendingQueue(const std::string& type, int direction) {
        if (queueCounts[direction] < MAX_QUEUE_SIZE) {
            float priority = 1.0f;
            if (type == "Emergency") {
                priority = 3.0f;
            } else if (type == "Heavy") {
                priority = 2.0f;
            }
            pendingVehicles[direction].push({type, direction, priority});
            queueCounts[direction]++;
        }
    }

    bool hasPendingVehicles(int direction) {
        return !pendingVehicles[direction].empty();
    }

    PendingVehicle getNextPendingVehicle(int direction) {
        PendingVehicle vehicle = pendingVehicles[direction].top();
        pendingVehicles[direction].pop();
        queueCounts[direction]--;
        return vehicle;
    }

    bool isQueueFull(int direction) {
        return queueCounts[direction] >= MAX_QUEUE_SIZE;
    }

    void setCurrentTime(time_t time) {
        currentTime = time;
    }
};