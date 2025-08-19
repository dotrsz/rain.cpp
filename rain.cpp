#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <random>
#include <array>
#include <sys/ioctl.h> // For TIOCGWINSZ
#include <unistd.h>    // For STDOUT_FILENO
#include <algorithm>   // For std::find
#include <map>         // For std::map

// ANSI escape codes for terminal manipulation
#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define RESET_COLOR "\033[0m"

// ANSI color codes
#define COLOR_WHITE "\033[97m"      // Bright White
#define COLOR_LIGHT_BLUE "\033[94m" // Bright Blue
#define COLOR_DARK_BLUE "\033[34m"  // Regular Blue
#define COLOR_DARK_GREY "\033[90m"  // Bright Black
#define COLOR_BLACK "\033[30m"      // Regular Black
#define COLOR_PUDDLE "\033[36m"     // Cyan for puddles
#define COLOR_SPLASH "\033[96m"     // Bright Cyan for splash

// --- Configuration (now variables, with defaults) ---
int FRAME_DELAY_MS = 11; // Milliseconds per frame (approx 90 FPS)
int RAINDROP_LENGTH_GLOBAL = 7; // Height of raindrops (global variable)
int PUDDLE_LIFETIME = 30; // How many frames a puddle lasts (increased)
const int SPLASH_LIFETIME = 3; // How many frames a splash lasts (kept const for array)
int THRESHOLD_MIN = 1; // Minimum frames before a raindrop moves
int THRESHOLD_MAX = 8; // Maximum frames before a raindrop moves

// --- Character sets ---
const std::array<char, 5> HEAD_CHARS = {'@', 'O', 'o', '%', '*'};
const std::array<char, 13> TAIL_CHARS = {'i', 'u', 'l', 'x', '.', '`', '!', ';', ':', '\'', '|', '{', '}'};
const std::array<char, 3> PUDDLE_CHARS = {'~', '-', '='}; // Characters for puddles
const std::array<std::string, SPLASH_LIFETIME> SPLASH_FRAMES = {
    "o", // Frame 1
    "()", // Frame 2
    "()" // Frame 3 (same as frame 2, but could be different)
};

// --- Raindrop structure ---
struct Raindrop {
    int x, y; // x, y represent the bottom-most character of the raindrop
    int speed; // How many rows it moves when it does move
    char head_char; // Stored for persistence
    std::vector<char> tail_chars; // Changed to std::vector for dynamic sizing
    int move_counter; // Counts frames until next move
    int move_threshold; // How many frames before it moves

    Raindrop(int start_x, int start_y, int s, char h_char, std::mt19937& gen, std::uniform_int_distribution<>& tail_char_dist, std::uniform_int_distribution<>& threshold_dist, int raindrop_len)
        : x(start_x), y(start_y), speed(s), head_char(h_char), move_counter(0), move_threshold(threshold_dist(gen)) {
        // Initialize tail characters
        tail_chars.resize(raindrop_len - 1); // Resize vector based on dynamic length
        for (size_t i = 0; i < tail_chars.size(); ++i) {
            tail_chars[i] = TAIL_CHARS[tail_char_dist(gen)];
        }
    }

    void update() { // Removed unused gen and tail_char_dist parameters
        move_counter++;
        if (move_counter >= move_threshold) {
            y += speed;
            move_counter = 0;
        }
    }

    bool is_offscreen(int terminal_height) const {
        // A raindrop is offscreen if its bottom-most character (head) is at or below the screen
        return y >= terminal_height;
    }
};

// --- Puddle Information ---
struct PuddleInfo {
    char character;
    int lifetime; // Remaining frames until disappearance

    PuddleInfo() : character(' '), lifetime(0) {}
};

// --- Splash Information ---
struct Splash {
    int x, y;
    int frame; // Current frame of the splash animation
    int lifetime; // Remaining frames until disappearance

    Splash(int start_x, int start_y) : x(start_x), y(start_y), frame(0), lifetime(SPLASH_LIFETIME) {}

    bool update() {
        frame++;
        lifetime--;
        return lifetime > 0;
    }
};

// --- Help Message ---
void print_help() {
    std::cout << "Usage: rain [OPTIONS]\n";
    std::cout << "Simulates ASCII rain in the terminal.\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help                 Display this help message and exit.\n";
    std::cout << "  --fps <value>          Set the frames per second (default: 90).\n";
    std::cout << "  --raindrop-length <value> Set the height of raindrops (default: 7).\n";
    std::cout << "  --puddle-lifetime <value> Set how many frames puddles last (default: 30).\n";
    std::cout << "  --splash-lifetime <value> Set how many frames splashes last (default: 3).\n";
    std::cout << "  --speed-min <value>    Set the minimum speed threshold (default: 1).\n";
    std::cout << "  --speed-max <value>    Set the maximum speed threshold (default: 8).\n";
    std::cout << "  --wind-direction <left|right> Set the wind direction (required if --wind-speed is set).\n";
    std::cout << "  --wind-speed <value>   Set the wind speed (required if --wind-direction is set).\n";
    std::cout << "  --lightning            Enable lightning effect.\n";
    std::cout << "\nExample:\n";
    std::cout << "  rain --fps 60 --wind-direction right --wind-speed 2 --lightning\n";
}

// --- Main simulation logic ---
int main(int argc, char* argv[]) {
    int wind_direction = 0; // -1 for left, 1 for right, 0 for no wind
    int wind_speed = 0;
    bool ENABLE_LIGHTNING = false;
    int lightning_flash_duration = 3;
    int lightning_counter = 0;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            print_help();
            return 0;
        } else if (arg == "--fps") {
            if (i + 1 < argc) {
                FRAME_DELAY_MS = 1000 / std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --fps requires a value.\n";
                return 1;
            }
        } else if (arg == "--raindrop-length") {
            if (i + 1 < argc) {
                RAINDROP_LENGTH_GLOBAL = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --raindrop-length requires a value.\n";
                return 1;
            }
        } else if (arg == "--puddle-lifetime") {
            if (i + 1 < argc) {
                PUDDLE_LIFETIME = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --puddle-lifetime requires a value.\n";
                return 1;
            }
        } else if (arg == "--splash-lifetime") {
            if (i + 1 < argc) {
                // SPLASH_LIFETIME is const, so we can't change it directly.
                // If this needs to be configurable, SPLASH_FRAMES would also need to be dynamic.
                // For now, we'll just ignore this if it's passed, or error out.
                std::cerr << "Warning: --splash-lifetime is currently not configurable.\n";
                i++; // Consume the value
            } else {
                std::cerr << "Error: --splash-lifetime requires a value.\n";
                return 1;
            }
        } else if (arg == "--speed-min") {
            if (i + 1 < argc) {
                THRESHOLD_MIN = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --speed-min requires a value.\n";
                return 1;
            }
        } else if (arg == "--speed-max") {
            if (i + 1 < argc) {
                THRESHOLD_MAX = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --speed-max requires a value.\n";
                return 1;
            }
        } else if (arg == "--wind-direction") {
            if (i + 1 < argc) {
                std::string direction = argv[++i];
                if (direction == "left") {
                    wind_direction = -1;
                } else if (direction == "right") {
                    wind_direction = 1;
                } else {
                    std::cerr << "Error: --wind-direction must be 'left' or 'right'.\n";
                    return 1;
                }
            } else {
                std::cerr << "Error: --wind-direction requires a value.\n";
                return 1;
            }
        } else if (arg == "--wind-speed") {
            if (i + 1 < argc) {
                wind_speed = std::stoi(argv[++i]);
                if (wind_speed < 0) {
                    std::cerr << "Error: --wind-speed cannot be negative.\n";
                    return 1;
                }
            } else {
                std::cerr << "Error: --wind-speed requires a value.\n";
                return 1;
            }
        } else if (arg == "--lightning") {
            ENABLE_LIGHTNING = true;
        } else {
            std::cerr << "Error: Unknown option '" << arg << "'. Use --help for usage.\n";
            return 1;
        }
    }

    if ((wind_direction != 0 && wind_speed == 0) || (wind_direction == 0 && wind_speed != 0)) {
        std::cerr << "Error: Both --wind-direction and --wind-speed must be set if wind is desired.\n";
        return 1;
    }

    // Disable buffering for immediate output
    std::ios_base::sync_with_stdio(false);
    std::cout.tie(NULL);

    // Get terminal size dynamically
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        std::cerr << "Error getting terminal size. Using default 80x24." << std::endl;
        // Fallback values if ioctl fails (e.g., not a TTY)
        ws.ws_col = 80;
        ws.ws_row = 24;
    }
    int current_terminal_width = ws.ws_col;
    int current_terminal_height = ws.ws_row;

    // Random number generation
    std::random_device rd;
    // Combine random_device with high_resolution_clock for more robust seeding
    auto seed = rd() ^ std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);

    std::uniform_int_distribution<> head_char_dist(0, HEAD_CHARS.size() - 1);
    std::uniform_int_distribution<> tail_char_dist(0, TAIL_CHARS.size() - 1);
    std::uniform_int_distribution<> puddle_char_dist(0, PUDDLE_CHARS.size() - 1);
    std::uniform_int_distribution<> speed_dist(1, 3); // Each move is 1 to 3 units (default variable speed)
    std::uniform_int_distribution<> threshold_dist(THRESHOLD_MIN, THRESHOLD_MAX); // Move every 1 to 8 frames

    std::vector<Raindrop> raindrops;
    std::vector<bool> column_has_active_raindrop(current_terminal_width, false);
    std::vector<PuddleInfo> puddles(current_terminal_width); // Store puddle info
    std::vector<Splash> splashes; // Store active splashes

    // Hide cursor
    std::cout << HIDE_CURSOR;

    // Ensure cursor is shown and screen is cleared on exit
    atexit([]() {
        std::cout << SHOW_CURSOR;
        std::cout << CLEAR_SCREEN;
        std::cout << CURSOR_HOME;
        std::cout << RESET_COLOR; // Reset color on exit
    });

    while (true) {
        if (ENABLE_LIGHTNING) {
            if (lightning_counter > 0) {
                lightning_counter--;
                if (lightning_counter == 0) {
                    std::cout << "\033[27m"; // Invert off
                }
            } else {
                if (std::uniform_int_distribution<>(0, 1000)(gen) < 5) {
                    std::cout << "\033[7m"; // Invert on
                    lightning_counter = lightning_flash_duration;
                }
            }
        }
        // Clear screen and move cursor to home
        std::cout << CLEAR_SCREEN << CURSOR_HOME;

        // Draw and update puddles
        std::cout << COLOR_PUDDLE;
        std::cout << "\033[" << current_terminal_height << ";1H"; // Move to bottom row
        for (int x = 0; x < current_terminal_width; ++x) {
            if (puddles[x].lifetime > 0) {
                std::cout << puddles[x].character;
                puddles[x].lifetime--;
            } else {
                std::cout << ' '; // Clear puddle if lifetime is 0
            }
        }
        std::cout << RESET_COLOR;

        // Draw and update splashes
        std::cout << COLOR_SPLASH;
        for (auto it = splashes.begin(); it != splashes.end(); ) {
            if (it->update()) {
                // Draw splash frame
                std::cout << "\033[" << it->y + 1 << ";" << it->x + 1 << "H" << SPLASH_FRAMES[it->frame -1];
                ++it;
            } else {
                // Clear splash area
                std::cout << "\033[" << it->y + 1 << ";" << it->x + 1 << "H";
                for(size_t i = 0; i < SPLASH_FRAMES[it->frame -1].length(); ++i) {
                    std::cout << ' ';
                }
                it = splashes.erase(it);
            }
        }
        std::cout << RESET_COLOR;


        // Spawn new raindrops in empty columns
        for (int x = 0; x < current_terminal_width; ++x) {
            if (!column_has_active_raindrop[x]) {
                char new_head_char = HEAD_CHARS[head_char_dist(gen)];
                int new_speed = speed_dist(gen);
                raindrops.emplace_back(x, -RAINDROP_LENGTH_GLOBAL, new_speed, new_head_char, gen, tail_char_dist, threshold_dist, RAINDROP_LENGTH_GLOBAL);
                column_has_active_raindrop[x] = true;
            }
        }

        // Update and draw raindrops
        for (auto it = raindrops.begin(); it != raindrops.end(); ) {
            int original_x = it->x; // Store x before potential erase

            it->update(); // Call new update function
            if (wind_speed > 0) {
                it->x += (wind_direction * wind_speed);
                if (it->x < 0) it->x = 0;
                if (it->x >= current_terminal_width) it->x = current_terminal_width - 1;
            }

            if (it->is_offscreen(current_terminal_height)) {
                column_has_active_raindrop[original_x] = false; // Mark column as free
                puddles[original_x].character = PUDDLE_CHARS[puddle_char_dist(gen)]; // Create a puddle
                puddles[original_x].lifetime = PUDDLE_LIFETIME; // Set puddle lifetime
                splashes.emplace_back(original_x, current_terminal_height -1); // Create a splash
                it = raindrops.erase(it); // Remove if offscreen
            } else {
                // Draw each segment of the raindrop
                for (size_t i = 0; i < static_cast<size_t>(RAINDROP_LENGTH_GLOBAL); ++i) { // Use RAINDROP_LENGTH_GLOBAL
                    int current_y = it->y - i; // Calculate y position for this segment

                    if (current_y >= 0 && current_y < current_terminal_height) { // Use dynamic height
                        // Move cursor to position
                        std::cout << "\033[" << current_y + 1 << ";" << it->x + 1 << "H";

                        // Apply color and character based on segment position
                        if (i == 0) { // Bottom-most character (head)
                            std::cout << COLOR_WHITE << it->head_char;
                        } else if (i > 0 && i <= it->tail_chars.size()) { // Tail segments
                            // Map i to tail_chars index (i-1)
                            if (i == 1) {
                                std::cout << COLOR_LIGHT_BLUE << it->tail_chars[i-1];
                            } else if (i == 2) {
                                std::cout << COLOR_DARK_BLUE << it->tail_chars[i-1];
                            } else if (i == 3) {
                                std::cout << COLOR_DARK_GREY << it->tail_chars[i-1];
                            } else { // For i >= 4, use black
                                std::cout << COLOR_BLACK << it->tail_chars[i-1];
                            }
                        }
                    }
                }
                std::cout << RESET_COLOR; // Reset color after drawing each raindrop
                ++it;
            }
        }

        std::cout.flush(); // Ensure all output is written

        // Pause for animation
        std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_DELAY_MS));
    }

    return 0;
}
