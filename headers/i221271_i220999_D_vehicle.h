#ifndef VEHICLE_H
#define VEHICLE_H

#include "i221271_i220999_D_util.h"

class Vehicle {
public:
    sf::Texture tex;
    sf::Sprite veh;
    short maxSpeed;
    static int numVehicles;
    std::string numberPlate;
    int direction;  // Direction of the vehicle
    float currentSpeed;
    bool isEmergency;
    bool isHeavy;
    float speedUpdateTimer;
    int lane;  // Lane number
    bool hasChallan;
    bool hasViolation; // Flag for speed limit violation
    bool hasCollision; // Flag for collision

    Vehicle(std::string type, int direction, int lane) {
        // Initialize vehicle properties
        this->lane = lane;
        speedUpdateTimer = 0;
        isHeavy = false;
        isEmergency = false;
        hasChallan = false;
        //hasViolation = false; 
        hasCollision = false; 
        
        // Setup vehicle based on its type
        if(type == "Light") {
            std::string car ="res/Car_" + std::to_string(rand() % 4) + ".png";
            tex.loadFromFile(car);
            maxSpeed = 60;
            currentSpeed = 40 + (rand() % 21);
        }
        else if(type == "Heavy") {
            tex.loadFromFile("res/truck.png");
            maxSpeed = 40;
            isHeavy = true;
            currentSpeed = 20 + (rand() % 21);
        }
        else {
            tex.loadFromFile("res/Ambulance.png");
            maxSpeed = 80;
            isEmergency = true;
            currentSpeed = 60 + (rand() % 21);
        }
        
        
        veh.setTexture(tex);
        veh.setOrigin(veh.getLocalBounds().width/2, veh.getLocalBounds().height/2);
        this->direction = direction;
        numberPlate = type + std::to_string(numVehicles++);
        
        // ehicle position based on direction
        sf::Vector2f pos(SPAWN_POINTS[direction].x, SPAWN_POINTS[direction].y);
        float laneOffset = (lane == 1) ? 
            SPAWN_POINTS[direction].lanes.lane1_offset : 
            SPAWN_POINTS[direction].lanes.lane2_offset;

        switch(direction) {
            case 0: // North
                pos.x += laneOffset;
                break;
            case 1: // West
                pos.y += laneOffset;
                break;
            case 2: // South
                pos.x += laneOffset;
                break;
            case 3: // East
                pos.y += laneOffset;
                break;
        }
        
        veh.setPosition(pos);
        veh.setRotation(SPAWN_POINTS[direction].rotation);
    }

    //  box of the vehicle for collison detection
    sf::FloatRect getBoundingBox() const {
        return veh.getGlobalBounds();
    }

    private:
        bool isAtIntersection() const {
            intersectionBox box;
            sf::Vector2f pos = veh.getPosition();
            const float BUFFER = 20.0f;  // Buffer zone before intersection
            
            switch(direction) {
                case 0: // NORTH
                    if (pos.y > box.top.y ) return false;
                    return pos.y > box.top.y - BUFFER && pos.y < box.top.y + box.dim.y;
                case 1: // WEST
                    if (pos.x > box.top.x) return false;
                    return pos.x > box.top.x - BUFFER && pos.x < box.top.x + box.dim.x;
                case 2: // SOUTH
                    if (pos.y < box.top.y + box.dim.y ) return false;
                    return pos.y < box.top.y + box.dim.y + BUFFER && pos.y > box.top.y;
                case 3: // EAST
                    if (pos.x < box.top.x + box.dim.x) return false;
                    return pos.x < box.top.x + box.dim.x + BUFFER && pos.x > box.top.x;
            }
            return false;
        }

    public:
        void update(float deltaTime, bool isGreenLight) {
            // Update speed every 5 seconds 
            speedUpdateTimer += deltaTime;
            if(speedUpdateTimer >= 5.0f) {
                speedUpdateTimer = 0;
                if (!isAtIntersection() || isGreenLight || isEmergency) {
                    float newSpeed = currentSpeed + 5.0f;
                    if (rand() % 100 < 5) {
                        currentSpeed = std::min(newSpeed * 1.2f, (float)maxSpeed * 1.2f); // 20% increase
                    } else {
                        currentSpeed = std::min(newSpeed, (float)maxSpeed);
                    }
                }
            }

            // Stop at red light unless emergency vehicle
            if (isAtIntersection() && !isGreenLight && !isEmergency) {
                currentSpeed = 0;
                return;
            }

            // Move vehicle 
            float movement = currentSpeed * deltaTime;
            switch(direction) {
                case 0: // NORTH
                    veh.move(0, movement);  // Move down
                    break;
                case 1: // WEST
                    veh.move(movement, 0);  // Move right
                    break;
                case 2: // SOUTH
                    veh.move(0, -movement); // Move up
                    break;
                case 3: // EAST
                    veh.move(-movement, 0); // Move left
                    break;
            }
        }
};

int Vehicle::numVehicles = 0; 
#endif