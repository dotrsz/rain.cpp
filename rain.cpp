#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <random>
#include <array>
#include <sys/ioctl.h> // for tiocgwinsz
#include <unistd.h>    // for stdout_fileno
#include <algorithm>   // for std::find
#include <map>         // for std::map

// ansi escape codes for terminal manipulation
#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define RESET_COLOR "\033[0m"

// ansi color codes
#define COLOR_WHITE "\033[97m"      // bright white
#define COLOR_LIGHT_BLUE "\033[94m" // bright blue
#define COLOR_DARK_BLUE "\033[34m"  // regular blue
#define COLOR_DARK_GREY "\033[90m"  // bright black
#define COLOR_BLACK "\033[30m"      // regular black
#define COLOR_PUDDLE "\033[36m"     // cyan for puddles
#define COLOR_SPLASH "\033[96m"     // bright cyan for splash

// --- configuration (now variables, with defaults) ---
int FRAME_DELAY_MS = 11; // milliseconds per frame (approx 90 fps)
int RAINDROP_LENGTH_GLOBAL = 7; // height of raindrops (global variable)
int PUDDLE_LIFETIME = 30; // how many frames a puddle lasts (increased)
const int SPLASH_LIFETIME = 3; // how many frames a splash lasts (kept const for array)
int THRESHOLD_MIN = 1; // minimum frames before a raindrop moves
int THRESHOLD_MAX = 8; // maximum frames before a raindrop moves

// --- character sets ---
const std::array<char, 5> HEAD_CHARS = {'@', 'O', 'o', '%', '*'};
const std::array<char, 13> TAIL_CHARS = {'i', 'u', 'l', 'x', '.', '`', '!', ';', ':', '\'', '|', '{', '}'};
const std::array<char, 3> PUDDLE_CHARS = {'~', '-', '='}; // characters for puddles
const std::array<std::string, SPLASH_LIFETIME> SPLASH_FRAMES = {
    "o", // frame 1
    "()", // frame 2
    "()" // frame 3 (same as frame 2, but could be different)
};

// --- raindrop structure ---
struct Raindrop {
    int x, y; // x, y represent the bottom-most character of the raindrop
    int speed; // how many rows it moves when it does move
    char head_char; // stored for persistence
    std::vector<char> tail_chars; // changed to std::vector for dynamic sizing
    int move_counter; // counts frames until next move
    int move_threshold; // how many frames before it moves

    Raindrop(int start_x, int start_y, int s, char h_char, std::mt19937& gen, std::uniform_int_distribution<>& tail_char_dist, std::uniform_int_distribution<>& threshold_dist, int raindrop_len)
        : x(start_x), y(start_y), speed(s), head_char(h_char), move_counter(0), move_threshold(threshold_dist(gen)) {
        // initialize tail characters
        tail_chars.resize(raindrop_len - 1); // resize vector based on dynamic length
        for (size_t i = 0; i < tail_chars.size(); ++i) {
            tail_chars[i] = TAIL_CHARS[tail_char_dist(gen)];
        }
    }

    void update() { // removed unused gen and tail_char_dist parameters
        move_counter++;
        if (move_counter >= move_threshold) {
            y += speed;
            move_counter = 0;
        }
    }

    bool is_offscreen(int terminal_height) const {
        // a raindrop is offscreen if its bottom-most character (head) is at or below the screen
        return y >= terminal_height;
    }
};

// --- puddle information ---
struct PuddleInfo {
    char character;
    int lifetime; // remaining frames until disappearance

    PuddleInfo() : character(' '), lifetime(0) {}
};

// --- splash information ---
struct Splash {
    int x, y;
    int frame; // current frame of the splash animation
    int lifetime; // remaining frames until disappearance

    Splash(int start_x, int start_y) : x(start_x), y(start_y), frame(0), lifetime(SPLASH_LIFETIME) {}

    bool update() {
        frame++;
        lifetime--;
        return lifetime > 0;
    }
};

// --- help message ---
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

// --- main simulation logic ---
int main(int argc, char* argv[]) {
    int wind_direction = 0; // -1 for left, 1 for right, 0 for no wind
    int wind_speed = 0;
    bool ENABLE_LIGHTNING = false;
    int lightning_flash_duration = 3;
    int lightning_counter = 0;

    // parse command-line arguments
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
                // splash_lifetime is const, so we can't change it directly.
                // if this needs to be configurable, splash_frames would also need to be dynamic.
                // for now, we'll just ignore this if it's passed, or error out.
                std::cerr << "Warning: --splash-lifetime is currently not configurable.\n";
                i++; // consume the value
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

    // disable buffering for immediate output
    std::ios_base::sync_with_stdio(false);
    std::cout.tie(NULL);

    // get terminal size dynamically
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        std::cerr << "Error getting terminal size. Using default 80x24." << std::endl;
        // fallback values if ioctl fails (e.g., not a tty)
        ws.ws_col = 80;
        ws.ws_row = 24;
    }
    int current_terminal_width = ws.ws_col;
    int current_terminal_height = ws.ws_row;

    // random number generation
    std::random_device rd;
    // combine random_device with high_resolution_clock for more robust seeding
    auto seed = rd() ^ std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);

    std::uniform_int_distribution<> head_char_dist(0, HEAD_CHARS.size() - 1);
    std::uniform_int_distribution<> tail_char_dist(0, TAIL_CHARS.size() - 1);
    std::uniform_int_distribution<> puddle_char_dist(0, PUDDLE_CHARS.size() - 1);
    std::uniform_int_distribution<> speed_dist(1, 3); // each move is 1 to 3 units (default variable speed)
    std::uniform_int_distribution<> threshold_dist(THRESHOLD_MIN, THRESHOLD_MAX); // move every 1 to 8 frames

    std::vector<Raindrop> raindrops;
    std::vector<bool> column_has_active_raindrop(current_terminal_width, false);
    std::vector<PuddleInfo> puddles(current_terminal_width); // store puddle info
    std::vector<Splash> splashes; // store active splashes

    // hide cursor
    std::cout << HIDE_CURSOR;

    // ensure cursor is shown and screen is cleared on exit
    atexit([]() {
        std::cout << SHOW_CURSOR;
        std::cout << CLEAR_SCREEN;
        std::cout << CURSOR_HOME;
        std::cout << RESET_COLOR; // reset color on exit
    });

    while (true) {
        if (ENABLE_LIGHTNING) {
            if (lightning_counter > 0) {
                lightning_counter--;
                if (lightning_counter == 0) {
                    std::cout << "\033[27m"; // invert off
                }
            } else {
                if (std::uniform_int_distribution<>(0, 1000)(gen) < 5) {
                    std::cout << "\033[7m"; // invert on
                    lightning_counter = lightning_flash_duration;
                }
            }
        }
        // clear screen and move cursor to home
        std::cout << CLEAR_SCREEN << CURSOR_HOME;

        // draw and update puddles
        std::cout << COLOR_PUDDLE;
        std::cout << "\033[" << current_terminal_height << ";1H"; // move to bottom row
        for (int x = 0; x < current_terminal_width; ++x) {
            if (puddles[x].lifetime > 0) {
                std::cout << puddles[x].character;
                puddles[x].lifetime--;
            } else {
                std::cout << ' '; // clear puddle if lifetime is 0
            }
        }
        std::cout << RESET_COLOR;

        // draw and update splashes
        std::cout << COLOR_SPLASH;
        for (auto it = splashes.begin(); it != splashes.end(); ) {
            if (it->update()) {
                // draw splash frame
                std::cout << "\033[" << it->y + 1 << ";" << it->x + 1 << "H" << SPLASH_FRAMES[it->frame -1];
                ++it;
            } else {
                // clear splash area
                std::cout << "\033[" << it->y + 1 << ";" << it->x + 1 << "H";
                for(size_t i = 0; i < SPLASH_FRAMES[it->frame -1].length(); ++i) {
                    std::cout << ' ';
                }
                it = splashes.erase(it);
            }
        }
        std::cout << RESET_COLOR;


        // spawn new raindrops in empty columns
        for (int x = 0; x < current_terminal_width; ++x) {
            if (!column_has_active_raindrop[x]) {
                char new_head_char = HEAD_CHARS[head_char_dist(gen)];
                int new_speed = speed_dist(gen);
                raindrops.emplace_back(x, -RAINDROP_LENGTH_GLOBAL, new_speed, new_head_char, gen, tail_char_dist, threshold_dist, RAINDROP_LENGTH_GLOBAL);
                column_has_active_raindrop[x] = true;
            }
        }

        // update and draw raindrops
        for (auto it = raindrops.begin(); it != raindrops.end(); ) {
            int original_x = it->x; // store x before potential erase

            it->update(); // call new update function
            if (wind_speed > 0) {
                it->x += (wind_direction * wind_speed);
                if (it->x < 0) it->x = 0;
                if (it->x >= current_terminal_width) it->x = current_terminal_width - 1;
            }

            if (it->is_offscreen(current_terminal_height)) {
                column_has_active_raindrop[original_x] = false; // mark column as free
                puddles[original_x].character = PUDDLE_CHARS[puddle_char_dist(gen)]; // create a puddle
                puddles[original_x].lifetime = PUDDLE_LIFETIME; // set puddle lifetime
                splashes.emplace_back(original_x, current_terminal_height -1); // create a splash
                it = raindrops.erase(it); // remove if offscreen
            } else {
                // draw each segment of the raindrop
                for (size_t i = 0; i < static_cast<size_t>(RAINDROP_LENGTH_GLOBAL); ++i) { // use raindrop_length_global
                    int current_y = it->y - i; // calculate y position for this segment

                    if (current_y >= 0 && current_y < current_terminal_height) { // use dynamic height
                        // move cursor to position
                        std::cout << "\033[" << current_y + 1 << ";" << it->x + 1 << "H";

                        // apply color and character based on segment position
                        if (i == 0) { // bottom-most character (head)
                            std::cout << COLOR_WHITE << it->head_char;
                        } else if (i > 0 && i <= it->tail_chars.size()) { // tail segments
                            // map i to tail_chars index (i-1)
                            if (i == 1) {
                                std::cout << COLOR_LIGHT_BLUE << it->tail_chars[i-1];
                            } else if (i == 2) {
                                std::cout << COLOR_DARK_BLUE << it->tail_chars[i-1];
                            } else if (i == 3) {
                                std::cout << COLOR_DARK_GREY << it->tail_chars[i-1];
                            } else { // for i >= 4, use black
                                std::cout << COLOR_BLACK << it->tail_chars[i-1];
                            }
                        }
                    }
                }
                std::cout << RESET_COLOR; // reset color after drawing each raindrop
                ++it;
            }
        }

        std::cout.flush(); // ensure all output is written

        // pause for animation
        std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_DELAY_MS));
    }

    return 0;
}
