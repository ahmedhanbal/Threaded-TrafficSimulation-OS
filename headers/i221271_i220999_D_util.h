#ifndef UTIL_H
#define UTIL_H

#include <SFML/Graphics.hpp>
#include <queue>
#include <memory>
#include <ctime>

// Constants for road and lane dimensions
#define LANE_WIDTH 26
#define ROAD_WIDTH (LANE_WIDTH * 2)  // 52
#define WIDTH 836
#define HEIGHT 896
#define CENTER_X (WIDTH / 2)   // 418
#define CENTER_Y (HEIGHT / 2)  // 448
#define SIMTIME 300 // Simulation time (5 mins)
#define MAX_VEHICLES_PER_LANE 10

// Time constants (in seconds since midnight)
const int TIME_7AM = 7 * 3600;
const int TIME_930AM = 9 * 3600 + 30 * 60;
const int TIME_430PM = 16 * 3600 + 30 * 60;
const int TIME_830PM = 20 * 3600 + 30 * 60;

// Intersection box dimensions
struct intersectionBox{
    const sf::Vector2f  dim = {122,113}; // Width and height
    const sf::Vector2f top = {358 ,391}; // Top left
    const sf::Vector2f centre = {416,448}; // Centre
};

struct SpawnPoint {
    float x, y;        // Base spawn position
    float rotation;    // Vehicle rotation in degrees
    struct Lanes {
        float lane1_offset;  // Offset for lane 1
        float lane2_offset;  // Offset for lane 2
    } lanes;
};

// Calculate lane offsets from center of each road
const float INNER_LANE_OFFSET = LANE_WIDTH / 2 + 4;        // 13
const float OUTER_LANE_OFFSET = LANE_WIDTH * 1.5 + 4;      // 39

// Direction definitions
const SpawnPoint SPAWN_POINTS[] = {
    { CENTER_X, 0, 180, { INNER_LANE_OFFSET, OUTER_LANE_OFFSET } },      // NORTH
    { 0, CENTER_Y, 90, { -INNER_LANE_OFFSET, -OUTER_LANE_OFFSET } },       // WEST
    { CENTER_X, HEIGHT, 0, { -INNER_LANE_OFFSET, -OUTER_LANE_OFFSET } },     // SOUTH
    { WIDTH, CENTER_Y, 270, { INNER_LANE_OFFSET, OUTER_LANE_OFFSET } }     // EAST
};

const SpawnPoint OUTGOING_SPAWN_POINTS[] = {
    { CENTER_X, HEIGHT, 270, { INNER_LANE_OFFSET, OUTER_LANE_OFFSET } },  // NORTH
    { 0, CENTER_Y, 0, { INNER_LANE_OFFSET, OUTER_LANE_OFFSET } },         // WEST
    { CENTER_X, 0, 90, { -INNER_LANE_OFFSET, -OUTER_LANE_OFFSET } },      // SOUTH
    { WIDTH, CENTER_Y, 180, { -INNER_LANE_OFFSET, -OUTER_LANE_OFFSET } }  // EAST
};

#endif