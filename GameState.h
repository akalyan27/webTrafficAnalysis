/*
GameState.h
Defines the shared, thread-safe game world state for Flappy Bird.
*/

#pragma once

#include <mutex>
#include <vector>
#include <iostream>
#include <random>
#include <algorithm>
#include <chrono>
#include <SFML/Graphics.hpp>
#include "PlayerCommand.h" // Forward declaration wouldn't work, need full definition

// --- RENDER CONSTANTS ---
const float WINDOW_WIDTH = 800.0f;
const float WINDOW_HEIGHT = 600.0f;
// Scale factor to map physics coordinates (0-20) to screen coordinates (0-600)
const float SCALE_FACTOR = WINDOW_HEIGHT / 20.0f; 
const float BIRD_DRAW_SIZE = 20.0f; // Bird pixel size

// --- PHYSICS CONSTANTS ---
const float GAME_WIDTH = 80.0f;     // World width (used for pipe spawning)
const float PIPE_SPEED = -15.0f;    // Pipes move left (increased speed for drama)
const float GRAVITY = -40.0f;       // Stronger gravity for faster fall
const float FLAP_VELOCITY = 15.0f; // Instant upward velocity on flap
const float BIRD_RADIUS = 1.0f;     // World size of the bird (used for collision)

// --- GAME ENTITIES ---

struct PipeState {
    float x;        // Horizontal position (moves)
    float gap_y;    // Center vertical position of the gap
    float gap_size; // Height of the gap
    bool passed;    // Flag to score points
};

struct BirdState {
    float x = 20.0f;    // Fixed horizontal position (in world coordinates)
    float y = 10.0f;    // Vertical position
    float y_vel = 0.0f; // Vertical velocity
    bool is_alive = true;
    int score = 0;
};


class GameState {
private:
    mutable std::mutex m_mutex; // Must be mutable for const methods to lock it
    BirdState m_bird;
    std::vector<PipeState> m_pipes;
    float m_pipe_spawn_timer = 0.0f;
    std::default_random_engine m_rng{std::random_device{}()};
    
    // Helper function to convert world Y to screen Y
    float world_to_screen_y(float world_y) const {
        // In SFML, Y=0 is the top, so we must invert the Y axis and scale.
        return WINDOW_HEIGHT - (world_y * SCALE_FACTOR);
    }

    void spawn_pipe(float dt) {
        m_pipe_spawn_timer += dt;
        if (m_pipe_spawn_timer >= 1.8f) { // Spawn a new pipe every 1.8 seconds
            std::uniform_real_distribution<float> y_dist(5.0f, 15.0f);
            m_pipes.push_back({
                GAME_WIDTH, // Start far right
                y_dist(m_rng), // Random center gap Y
                6.0f, // Fixed gap size (in world units)
                false
            });
            m_pipe_spawn_timer = 0.0f;
        }
    }

    bool check_collision() {
        // Ground/Ceiling check (world Y is 0 to 20)
        if (m_bird.y <= BIRD_RADIUS || m_bird.y >= 20.0f - BIRD_RADIUS) { 
            return true;
        }

        // Pipe collision check
        for (const auto& pipe : m_pipes) {
            float pipe_width = 4.0f; // World width of pipe
            
            // X-axis check (is bird inside the pipe's horizontal bounds)
            if (m_bird.x + BIRD_RADIUS > pipe.x && m_bird.x - BIRD_RADIUS < pipe.x + pipe_width) {
                
                // Y-axis check (is bird outside the gap)
                float half_gap = pipe.gap_size / 2.0f;
                if (m_bird.y + BIRD_RADIUS > pipe.gap_y + half_gap || 
                    m_bird.y - BIRD_RADIUS < pipe.gap_y - half_gap) {
                    return true; // Collision detected!
                }
            }
        }
        return false;
    }

public:
    // --- Physics and Logic Updates ---

    /**
     * @brief The consumer task: applies the FLAP command by setting upward velocity.
     */
    void process_command(const PlayerCommand& command) {
        // Normal locking - workers will exit cleanly when queue stops
        std::lock_guard<std::mutex> lock(m_mutex); 
        
        if (command.type == ActionType::FLAP && m_bird.is_alive) {
            m_bird.y_vel = FLAP_VELOCITY; 
        }

        // Latency measurement is still critical for a low-latency project
        auto now = std::chrono::high_resolution_clock::now();
        auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(now - command.timestamp).count();
        if (latency_us > 1000) { 
            std::cout << "\n[WARN] Flap command latency: " << latency_us << "us\n";
        }
    }
    
    /**
     * @brief The main loop task: updates position, gravity, and checks collision.
     */
    void update_physics(float dt) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_bird.is_alive) return;

        // 1. Apply Bird Physics (Vertical)
        m_bird.y_vel += GRAVITY * dt;
        // Limit maximum falling speed (most negative velocity)
        const float MAX_FALL_SPEED = -50.0f;
        m_bird.y_vel = std::max(m_bird.y_vel, MAX_FALL_SPEED);
        m_bird.y += m_bird.y_vel * dt;

        // 2. Apply Pipe Movement (Horizontal)
        for (auto& pipe : m_pipes) {
            pipe.x += PIPE_SPEED * dt;
            
            // Score check 
            if (!pipe.passed && pipe.x < m_bird.x) {
                m_bird.score++;
                pipe.passed = true;
            }
        }

        // 3. Spawn and Cleanup Pipes
        spawn_pipe(dt);
        m_pipes.erase(
            std::remove_if(m_pipes.begin(), m_pipes.end(), 
                           [](const PipeState& p) { return p.x < -10.0f; }),
            m_pipes.end()
        );

        // 4. Collision Check
        if (check_collision()) {
            m_bird.is_alive = false;
        }
    }

    // For rendering and score display
    BirdState get_bird_state() const {
        try {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_bird;
        } catch (const std::system_error& e) {
            std::cerr << "[GameState] Warning: Mutex error in get_bird_state: " << e.what() << std::endl;
            // Return a default state if mutex fails (shouldn't happen in normal operation)
            return BirdState{};
        }
    }
    std::vector<PipeState> get_pipe_state() const {
        try {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_pipes;
        } catch (const std::system_error& e) {
            std::cerr << "[GameState] Warning: Mutex error in get_pipe_state: " << e.what() << std::endl;
            return std::vector<PipeState>{};
        }
    }
    
    // SFML Drawing Helpers
    float bird_screen_y() const {
        try {
            std::lock_guard<std::mutex> lock(m_mutex);
            return world_to_screen_y(m_bird.y) - BIRD_DRAW_SIZE / 2.0f;
        } catch (const std::system_error& e) {
            std::cerr << "[GameState] Warning: Mutex error in bird_screen_y: " << e.what() << std::endl;
            return 0.0f;
        }
    }

    // Convert pipe x/y to screen coordinates for drawing
    std::vector<sf::RectangleShape> get_drawable_pipes() const {
        try {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::vector<sf::RectangleShape> shapes;
            
            float pipe_screen_width = 4.0f * SCALE_FACTOR;
            
            for (const auto& pipe : m_pipes) {
                float screen_x = pipe.x * SCALE_FACTOR;
                float half_gap = pipe.gap_size / 2.0f;
                
                // --- Top Pipe ---
                float top_pipe_bottom_y_world = pipe.gap_y + half_gap;
                float top_pipe_height_world = 20.0f - top_pipe_bottom_y_world;
                
                sf::RectangleShape top_pipe(sf::Vector2f(pipe_screen_width, top_pipe_height_world * SCALE_FACTOR));
                top_pipe.setPosition(sf::Vector2f(screen_x, 0.0f)); // Top pipe starts at screen Y=0
                top_pipe.setFillColor(sf::Color(100, 200, 50)); // Green
                shapes.push_back(top_pipe);

                // --- Bottom Pipe ---
                float bottom_pipe_top_y_world = pipe.gap_y - half_gap;
                float bottom_pipe_height_world = bottom_pipe_top_y_world; // Distance from ground (Y=0)
                
                sf::RectangleShape bottom_pipe(sf::Vector2f(pipe_screen_width, bottom_pipe_height_world * SCALE_FACTOR));
                
                // SFML y-coordinate for the top of the bottom pipe
                float bottom_pipe_screen_y = world_to_screen_y(bottom_pipe_top_y_world); 
                
                bottom_pipe.setPosition(sf::Vector2f(screen_x, bottom_pipe_screen_y));
                bottom_pipe.setFillColor(sf::Color(100, 200, 50)); // Green
                shapes.push_back(bottom_pipe);
            }
            return shapes;
        } catch (const std::system_error& e) {
            std::cerr << "[GameState] Warning: Mutex error in get_drawable_pipes: " << e.what() << std::endl;
            return std::vector<sf::RectangleShape>{};
        }
    }
};