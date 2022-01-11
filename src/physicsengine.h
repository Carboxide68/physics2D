#pragma once

#include "common.h"
#include "buffer.h"

#include <SoftBody>
#include <thread>
#include <vector>
#include <pair>
#include <glm/vec3.hpp>

class PhysicsEngine {
public:

    PhysicsEngine();
    ~PhysicsEngine();

    /**
     * @brief Starts the engine
     */
    void start(); //Start the engine on another thread
    /**
     * @brief Stops the engine
     */
    void stop(); //Stop the engine

    /**
     * @brief Adds a soft body to the engine
     *
     * @param nodes Node positions
     * @param connections Pairs of indices to the nodes
     */
    void addSoftBody(std::vector<glm::vec3> nodes, std::vector<std::pair<uint, uint>> connections);

    std::vector<SoftBody> soft_bodies;

private:

    std::thread physics_thread;

};
