# Concurrent Game Engine Architecture (Flappy Bird Demo)

This project demonstrates a robust, concurrent architecture for a real-time game, using C++17, multithreading primitives, and the SFML library for rendering. The core design implements a Producer-Consumer pattern to separate low-latency input/rendering (Producer) from physics and game state updates (Consumer/Worker Threads).

This pattern is critical for maintaining a smooth, high-framerate graphical display even when the underlying game logic or physics calculations are computationally intensive. The provided code is structured as a concurrent Flappy Bird implementation.

## Architecture and Components

The system is split into two primary roles managed across multiple threads:

1. The Main Thread (Producer / Renderer)

- Role: Handles all input events (like a player "FLAP") and graphical rendering.

- Operation: Runs the high-framerate SFML game loop (60 FPS). It produces PlayerCommand tasks and pushes them onto the SafeQueue.

2. The Worker Threads (Consumers / Physics Engine)

- Role: Executes the physics and game state updates.

- Operation: A fixed set of threads managed by the ThreadPool continuously consumes PlayerCommand tasks from the queue and applies them to the shared, thread-safe GameState.

## Key Files and Modules


### main.cpp

Entry Point & Orchestrator. Initializes SFML, sets up the GameState, SafeQueue, and ThreadPool. It contains the main rendering loop and the function executed by worker threads (worker_task).

### GameState.h

Shared State. Defines the state of the game entities (Bird and Pipes), physics constants, and collision logic. Thread-safety is achieved through internal synchronization (e.g., using a mutex, although the worker task in main.cpp appears to be the central mechanism for state updates).

### SafeQueue.h

Thread-Safe Queue. A generic, condition-variable-based bounded buffer implementation. It safely decouples the low-latency producer (Main Thread) from the consumers (Worker Threads).

### ThreadPool.h / threadPool.cpp

Worker Manager. Manages a fixed pool of worker threads. It handles thread creation, execution of a provided task function, and safe shutdown using the join() mechanism.

### PlayerCommand.h

Task Object. Defines the structure for a single, discrete player action (e.g., FLAP) that is passed from the Main Thread to the Worker Threads.

## Dependencies and Compilation

This project requires the SFML (Simple and Fast Multimedia Library) library and a modern C++ compiler (supporting C++17 features like std::thread, std::mutex, and std::condition_variable).

Dependencies

- C++17 or newer.

- SFML (version 2.5 or later is recommended).

Compilation Example (using g++)

Assuming SFML is installed and linked correctly on your system, you can compile the project using a command similar to the following. Note the -lsfml-graphics, -lsfml-window, -lsfml-system flags for linking the SFML modules, and the -pthread flag for C++ concurrency support.

```bash
g++ -std=c++17 main.cpp threadPool.cpp -o flappy_bird -lsfml-graphics -lsfml-window -lsfml-system -pthread
```

Execution
```bash
./flappy_bird
```
