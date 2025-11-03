/*
main.cpp
The Concurrent Flappy Bird Server with SFML Frontend: 
1. Main Thread initializes SFML window and runs the low-latency game loop (Renderer/Input).
2. Worker Threads (ThreadPool) consume FLAP commands and update the shared GameState.
*/

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <variant>
#include <type_traits>
#include <SFML/Graphics.hpp>  // SFML Graphics (includes Window.hpp)

#include "SafeQueue.h" 
#include "ThreadPool.h" 
#include "PlayerCommand.h"
#include "GameState.h"

// The queue holds player commands
using CommandQueue = SafeQueue<PlayerCommand>; 

// Global atomic flag for shutdown coordination
std::atomic<bool> g_running(true);


// --- Main Thread: The Low-Latency Game Loop / Renderer / Input Handler (PRODUCER) ---
int main() {
    
    // CRITICAL FIX: Make GameState a local variable, not a global
    // This ensures proper destruction order with the ThreadPool
    GameState game_state;
    
    std::cout << "[System] GameState initialized successfully.\n";
    
    // 1. Setup SFML Window
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(static_cast<unsigned int>(WINDOW_WIDTH), static_cast<unsigned int>(WINDOW_HEIGHT))), 
                            "Concurrent Flappy Bird - Low Latency", 
                            sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60); // Set rendering limit to 60 FPS

    // 2. Setup Concurrency Components
    CommandQueue command_queue;
    size_t num_worker_threads = std::thread::hardware_concurrency();
    std::cout << "[System] Starting ThreadPool with " << num_worker_threads << " physics workers.\n";

    // 3. Create worker task lambda that captures game_state by reference
    auto worker_task = [&game_state](CommandQueue& cmd_queue) {
        PlayerCommand command;
        // Worker runs until pop() returns false (queue stopped AND empty)
        while (cmd_queue.pop(command)) {
            // Apply the FLAP velocity update
            game_state.process_command(command); 
        }
    };

    // 4. Start the ThreadPool (Consumers)
    // IMPORTANT: thread_pool must be declared here so it's destroyed AFTER command_queue
    ThreadPool thread_pool(num_worker_threads, command_queue, worker_task); 
    thread_pool.start();

    // Game loop timing setup
    sf::Clock clock;
    // Target update rate for physics integration (e.g., 60 Hz or 1/60th of a second)
    const float FIXED_TIMESTEP = 1.0f / 60.0f; 
    float accumulator = 0.0f; // Stores time since last physics update

    std::cout << "[Main Thread] SFML Window running. Use SPACE to FLAP.\n";
    
    // SFML Game Loop
    while (window.isOpen() && g_running.load()) {
        float frame_time = clock.restart().asSeconds();
        accumulator += frame_time;

        // --- PRODUCER (Input Handling) ---
        // SFML 3.0: pollEvent returns std::optional<Event>
        while (auto event_opt = window.pollEvent()) {
            auto& event = *event_opt;
            // Check event type using SFML 3.0 API
            if (event.is<sf::Event::Closed>()) {
                g_running.store(false);
                window.close();
            } else if (auto* key_pressed = event.getIf<sf::Event::KeyPressed>()) {
                if (key_pressed->code == sf::Keyboard::Key::Space || key_pressed->code == sf::Keyboard::Key::Up) {
                    // Create a PlayerCommand and push it to the queue (Producer action)
                    // Only process if bird is alive to avoid queuing unnecessary commands
                    auto bird_state = game_state.get_bird_state();
                    if (bird_state.is_alive) {
                        PlayerCommand cmd;
                        cmd.player_id = 1;
                        cmd.type = ActionType::FLAP;
                        cmd.timestamp = std::chrono::high_resolution_clock::now();
                        command_queue.push(std::move(cmd));
                    }
                } else if (key_pressed->code == sf::Keyboard::Key::Escape) {
                    // Allow ESC to quit the game
                    g_running.store(false);
                    window.close();
                }
            }
        }
        
        // --- INTEGRATOR (Physics Tick) ---
        // Run physics updates at a fixed rate, independent of rendering speed
        while (accumulator >= FIXED_TIMESTEP) {
            game_state.update_physics(FIXED_TIMESTEP);
            accumulator -= FIXED_TIMESTEP;
        }

        // --- RENDERER (Drawing the State) ---
        // Get all game state in one lock to minimize mutex contention
        auto bird = game_state.get_bird_state();
        
        // Check if the game is over BEFORE getting pipe shapes (avoid unnecessary work)
        if (!bird.is_alive) {
            g_running.store(false);
        }
        
        auto pipe_shapes = game_state.get_drawable_pipes();

        window.clear(sf::Color(135, 206, 235)); // Sky blue background

        // 1. Draw Bird
        sf::CircleShape bird_shape(BIRD_DRAW_SIZE / 2.0f);
        bird_shape.setFillColor(sf::Color::Yellow);
        bird_shape.setOutlineColor(sf::Color::Black);
        bird_shape.setOutlineThickness(2.0f);
        // Calculate screen Y from bird world Y (convert world to screen coordinates)
        float bird_screen_y = WINDOW_HEIGHT - (bird.y * SCALE_FACTOR) - BIRD_DRAW_SIZE / 2.0f;
        bird_shape.setPosition(sf::Vector2f(
            bird.x * SCALE_FACTOR - BIRD_DRAW_SIZE / 2.0f,
            bird_screen_y
        ));
        window.draw(bird_shape);

        // 2. Draw Pipes
        for (const auto& shape : pipe_shapes) {
            window.draw(shape);
        }
        
        // 3. Draw Score and Game Over Message
        // Font should be loaded once, but check if it exists
        static sf::Font font;
        static bool font_loaded = false;
        if (!font_loaded) {
            // Try multiple common font paths
            const char* font_paths[] = {
                "/System/Library/Fonts/Supplemental/Arial.ttf",
                "/System/Library/Fonts/Helvetica.ttc",
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
                "C:\\Windows\\Fonts\\arial.ttf",
                nullptr
            };
            for (int i = 0; font_paths[i]; ++i) {
                if (font.openFromFile(font_paths[i])) {
                    font_loaded = true;
                    break;
                }
            }
        }
        
        if (font_loaded) {
            sf::Text score_text(font, "Score: " + std::to_string(bird.score), 30);
            score_text.setFillColor(sf::Color::Black);
            score_text.setPosition(sf::Vector2f(WINDOW_WIDTH - 150, 10));
            window.draw(score_text);

            if (!bird.is_alive) {
                sf::Text game_over_text(font, "GAME OVER!\nFinal Score: " + std::to_string(bird.score) + "\n(Close Window)", 50);
                game_over_text.setFillColor(sf::Color::Red);
                game_over_text.setStyle(sf::Text::Style::Bold);
                // Center the text
                sf::FloatRect textRect = game_over_text.getLocalBounds();
                game_over_text.setOrigin(sf::Vector2f(textRect.size.x / 2.0f, textRect.size.y / 2.0f));
                game_over_text.setPosition(sf::Vector2f(WINDOW_WIDTH/4.0f, WINDOW_HEIGHT/2.0f));
                window.draw(game_over_text);
            }
        }

        window.display();
    }
    
    std::cout << "[System] Signaling workers to stop and joining threads...\n";
    
    // STEP 1: Stop accepting new commands - signals workers to exit their loops
    command_queue.stop();
    
    // STEP 2: Wait for all worker threads to fully exit
    // This ensures ALL mutex locks are released before we continue
    thread_pool.join(); 
    
    std::cout << "[System] All worker threads have exited.\n";
    
    // STEP 3: Now automatic destruction happens in the correct order:
    //   - thread_pool destructor runs (checks m_joined=true, does nothing)
    //   - command_queue destructor runs (safe, no threads using it)
    //   - game_state destructor runs (safe, no threads accessing it)
    
    std::cout << "Simulation finished. Concurrent resources joined safely.\n";

    return 0;
}