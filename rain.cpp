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
#include <cmath>       // for round
#include <fstream>
#include <sstream>
#include <locale>

// ansi escape codes for terminal manipulation
#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define RESET_COLOR "\033[0m"

// --- configuration (now variables, with defaults) ---
const float WIND_SPEED_FACTOR = 4.0f;
const int LIGHTNING_CHANCE_DIVISOR = 500;
const int LIGHTNING_CHANCE = 5;
int FRAME_DELAY_MS = 11; // milliseconds per frame (approx 90 fps)
int RAINDROP_LENGTH_GLOBAL = 7; // height of raindrops (global variable)
int PUDDLE_LIFETIME = 30; // how many frames a puddle lasts (increased)
const int SPLASH_LIFETIME = 3; // how many frames a splash lasts (kept const for array)
int THRESHOLD_MIN = 1; // minimum frames before a raindrop moves
int THRESHOLD_MAX = 8; // maximum frames before a raindrop moves
int RAIN_DENSITY = 25; // percentage chance of a new raindrop spawning

// --- color themes ---
enum class ThemeEnum {
    DEFAULT,
    MATRIX,
    RUNNER,
    BLADE,
    METRO,
    JOHNNY,
    AKIRA,
    GHOST,
    DARK,
    STALKER
};

struct ColorTheme {
    std::string head;
    std::string tail1;
    std::string tail2;
    std::string tail3;
    std::string tail4;
    std::string puddle;
    std::string splash;
};

struct CharacterTheme {
    std::vector<std::string> head;
    std::vector<std::string> tail;
    std::vector<std::string> puddle;
};

struct ThemeData {
    ColorTheme colors;
    CharacterTheme chars;
};

ThemeData current_theme;

std::vector<std::string> to_utf8_chars(const std::string& s) {
    std::vector<std::string> chars;
    for (size_t i = 0; i < s.length();) {
        int len = 1;
        if ((s[i] & 0xf8) == 0xf0) len = 4;
        else if ((s[i] & 0xf0) == 0xe0) len = 3;
        else if ((s[i] & 0xe0) == 0xc0) len = 2;
        chars.push_back(s.substr(i, len));
        i += len;
    }
    return chars;
}

std::string unescape(const std::string& s) {
    std::string res;
    res.reserve(s.length());
    for (std::string::size_type i = 0; i < s.length(); ++i) {
        if (s[i] == '\\' && i + 1 < s.length()) {
            switch (s[++i]) {
                case '\\': res += '\\'; break;
                case '"': res += '"'; break;
                case 'n': res += '\n'; break;
                case 't': res += '\t'; break;
                case '0':
                    if (i + 2 < s.length() && s[i+1] == '3' && s[i+2] == '3') {
                        res += '\033';
                        i += 2;
                    } else {
                        res += s[i];
                    }
                    break;
                default:
                    res += s[i];
                    break;
            }
        } else {
            res += s[i];
        }
    }
    return res;
}

bool load_theme(const std::string& theme_name) {
    std::ifstream theme_file("themes/" + theme_name + ".theme");
    if (!theme_file.is_open()) {
        return false;
    }

    theme_file.imbue(std::locale(""));

    std::string line;
    while (std::getline(theme_file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, ':') && std::getline(iss, value)) {
            // trim leading whitespace from value
            size_t first = value.find_first_not_of(" ");
            if (std::string::npos != first) {
                value = value.substr(first);
            }
            // trim quotes
            if (value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }

            if (key == "head_color") current_theme.colors.head = unescape(value);
            else if (key == "tail1_color") current_theme.colors.tail1 = unescape(value);
            else if (key == "tail2_color") current_theme.colors.tail2 = unescape(value);
            else if (key == "tail3_color") current_theme.colors.tail3 = unescape(value);
            else if (key == "tail4_color") current_theme.colors.tail4 = unescape(value);
            else if (key == "puddle_color") current_theme.colors.puddle = unescape(value);
            else if (key == "splash_color") current_theme.colors.splash = unescape(value);
            else if (key == "head_chars") current_theme.chars.head = to_utf8_chars(value);
            else if (key == "tail_chars") current_theme.chars.tail = to_utf8_chars(value);
            else if (key == "puddle_chars") current_theme.chars.puddle = to_utf8_chars(value);
        }
    }

    return true;
}

// --- raindrop structure ---
struct Raindrop {
    int x, y; // x, y represent the bottom-most character of the raindrop
    float x_f; // floating point x for smooth wind
    int speed; // how many rows it moves when it does move
    std::string head_char; // stored for persistence
    std::vector<std::string> tail_chars; // changed to std::vector for dynamic sizing
    int move_counter; // counts frames until next move
    int move_threshold; // how many frames before it moves

    Raindrop(int start_x, int start_y, int s, std::string h_char, std::mt19937& gen, std::uniform_int_distribution<>& tail_char_dist, std::uniform_int_distribution<>& threshold_dist, int raindrop_len)
        : x(start_x), y(start_y), x_f(start_x), speed(s), head_char(h_char), move_counter(0), move_threshold(threshold_dist(gen)) {
        // initialize tail characters
        tail_chars.resize(raindrop_len - 1); // resize vector based on dynamic length
        for (size_t i = 0; i < tail_chars.size(); ++i) {
            tail_chars[i] = current_theme.chars.tail[tail_char_dist(gen)];
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
    std::string character;
    int lifetime; // remaining frames until disappearance

    PuddleInfo() : character(" "), lifetime(0) {}
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
    std::cout << "  --lightning-duration <value> Set how many frames the lightning flash lasts (default: 3).\n";
    std::cout << "  --density <value>      Set the percentage chance of a new raindrop spawning (default: 25).\n";
    std::cout << "  --theme <default|blade|runner|matrix|metro|johnny|akira|ghost|dark|stalker> Set the color theme (default: default).\n";
    std::cout << "\nExample:\n";
    std::cout << "  rain --fps 60 --wind-direction right --wind-speed 2 --lightning --theme matrix\n";
}

void handle_lightning(bool enabled, int& counter, int duration, std::uniform_int_distribution<>& dist, std::mt19937& gen) {
    if (enabled) {
        if (counter > 0) {
            counter--;
            if (counter == 0) {
                std::cout << "\033[39;49m"; // reset foreground and background color
            }
        } else {
            if (dist(gen) < LIGHTNING_CHANCE) {
                std::cout << "\033[97;107m"; // bright white foreground on white background
                counter = duration;
            }
        }
    }
}

void draw_puddles(std::vector<PuddleInfo>& puddles, int width, int height) {
    std::cout << current_theme.colors.puddle;
    std::cout << "\033[" << height << ";1H"; // move to bottom row
    for (int x = 0; x < width; ++x) {
        if (puddles[x].lifetime > 0) {
            std::cout << puddles[x].character;
            puddles[x].lifetime--;
        } else {
            std::cout << ' '; // clear puddle if lifetime is 0
        }
    }
    std::cout << RESET_COLOR;
}

void update_and_draw_splashes(std::vector<Splash>& splashes) {
    std::cout << current_theme.colors.splash;
    for (auto it = splashes.begin(); it != splashes.end(); ) {
        if (it->update()) {
            // draw splash frame
            std::cout << "\033[" << it->y + 1 << ";" << it->x + 1 << "H" << current_theme.chars.puddle[0];
            ++it;
        } else {
            // clear splash area
            std::cout << "\033[" << it->y + 1 << ";" << it->x + 1 << "H";
            for(size_t i = 0; i < 1; ++i) {
                std::cout << ' ';
            }
            it = splashes.erase(it);
        }
    }
    std::cout << RESET_COLOR;
}

void spawn_raindrops(
    std::vector<Raindrop>& raindrops,
    std::vector<bool>& column_has_active_raindrop,
    int width,
    std::mt19937& gen,
    std::uniform_int_distribution<>& head_char_dist,
    std::uniform_int_distribution<>& tail_char_dist,
    std::uniform_int_distribution<>& speed_dist,
    std::uniform_int_distribution<>& threshold_dist,
    std::uniform_int_distribution<>& density_dist
) {
    for (int x = 0; x < width; ++x) {
        if (!column_has_active_raindrop[x]) {
            if (density_dist(gen) <= RAIN_DENSITY) {
                std::string new_head_char = current_theme.chars.head[head_char_dist(gen)];
                int new_speed = speed_dist(gen);
                raindrops.emplace_back(x, -RAINDROP_LENGTH_GLOBAL, new_speed, new_head_char, gen, tail_char_dist, threshold_dist, RAINDROP_LENGTH_GLOBAL);
                column_has_active_raindrop[x] = true;
            }
        }
    }
}

void update_and_draw_raindrops(
    std::vector<Raindrop>& raindrops,
    std::vector<bool>& column_has_active_raindrop,
    std::vector<PuddleInfo>& puddles,
    std::vector<Splash>& splashes,
    int height,
    int width,
    int wind_speed,
    int wind_direction,
    std::uniform_int_distribution<>& puddle_char_dist,
    std::mt19937& gen
) {
    for (auto it = raindrops.begin(); it != raindrops.end(); ) {
        int original_x = it->x; // store x before potential erase

        it->update(); // call new update function
        if (wind_speed > 0) {
            it->x_f += (wind_direction * wind_speed) / WIND_SPEED_FACTOR;
            it->x = static_cast<int>(round(it->x_f));

            if (it->x < 0) {
                it->x = width - 1;
                it->x_f = it->x;
            } else if (it->x >= width) {
                it->x = 0;
                it->x_f = it->x;
            }
        }

        if (it->is_offscreen(height)) {
            column_has_active_raindrop[original_x] = false; // mark column as free
            puddles[original_x].character = current_theme.chars.puddle[puddle_char_dist(gen)]; // create a puddle
            puddles[original_x].lifetime = PUDDLE_LIFETIME; // set puddle lifetime
            splashes.emplace_back(original_x, height -1); // create a splash
            it = raindrops.erase(it); // remove if offscreen
        } else {
            // draw each segment of the raindrop
            for (size_t i = 0; i < static_cast<size_t>(RAINDROP_LENGTH_GLOBAL); ++i) { // use raindrop_length_global
                int current_y = it->y - i; // calculate y position for this segment

                if (current_y >= 0 && current_y < height) { // use dynamic height
                    // move cursor to position
                    std::cout << "\033[" << current_y + 1 << ";" << it->x + 1 << "H";

                    // apply color and character based on segment position
                    if (i == 0) { // bottom-most character (head) 
                        std::cout << current_theme.colors.head << it->head_char;
                    } else if (i > 0 && i <= it->tail_chars.size()) { // tail segments
                        // map i to tail_chars index (i-1)
                        if (i == 1) {
                            std::cout << current_theme.colors.tail1 << it->tail_chars[i-1];
                        } else if (i == 2) {
                            std::cout << current_theme.colors.tail2 << it->tail_chars[i-1];
                        } else if (i == 3) {
                            std::cout << current_theme.colors.tail3 << it->tail_chars[i-1];
                        } else { // for i >= 4, use black
                            std::cout << current_theme.colors.tail4 << it->tail_chars[i-1];
                        }
                    }
                }
            }
            std::cout << RESET_COLOR; // reset color after drawing each raindrop
            ++it;
        }
    }
}

// --- main simulation logic ---
int main(int argc, char* argv[]) {
    std::setlocale(LC_ALL, "");

    std::string theme_name = "default";
    int wind_direction = 0;
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
        } else if (arg == "--lightning-duration") {
            if (i + 1 < argc) {
                lightning_flash_duration = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --lightning-duration requires a value.\n";
                return 1;
            }
        } else if (arg == "--density") {
            if (i + 1 < argc) {
                RAIN_DENSITY = std::stoi(argv[++i]);
                if (RAIN_DENSITY < 1 || RAIN_DENSITY > 100) {
                    std::cerr << "Error: --density must be between 1 and 100.\n";
                    return 1;
                }
            } else {
                std::cerr << "Error: --density requires a value.\n";
                return 1;
            }
        } else if (arg == "--theme") {
            if (i + 1 < argc) {
                theme_name = argv[++i];
            } else {
                std::cerr << "Error: --theme requires a value.\n";
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option '" << arg << "'. Use --help for usage.\n";
            return 1;
        }
    }

    if (!load_theme(theme_name)) {
        std::cerr << "Error: could not load theme '" << theme_name << "'.\n";
        return 1;
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

    std::uniform_int_distribution<> head_char_dist(0, current_theme.chars.head.size() - 1);
    std::uniform_int_distribution<> tail_char_dist(0, current_theme.chars.tail.size() - 1);
    std::uniform_int_distribution<> puddle_char_dist(0, current_theme.chars.puddle.size() - 1);
    std::uniform_int_distribution<> speed_dist(1, 3); // each move is 1 to 3 units (default variable speed)
    std::uniform_int_distribution<> threshold_dist(THRESHOLD_MIN, THRESHOLD_MAX); // move every 1 to 8 frames
    std::uniform_int_distribution<> lightning_dist(0, LIGHTNING_CHANCE_DIVISOR); // for lightning
    std::uniform_int_distribution<> density_dist(1, 100);

    std::vector<Raindrop> raindrops;
    std::vector<bool> column_has_active_raindrop(current_terminal_width, false);
    std::vector<PuddleInfo> puddles(current_terminal_width);
    std::vector<Splash> splashes;

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
        handle_lightning(ENABLE_LIGHTNING, lightning_counter, lightning_flash_duration, lightning_dist, gen);

        // clear screen and move cursor to home
        std::cout << CLEAR_SCREEN << CURSOR_HOME;

        draw_puddles(puddles, current_terminal_width, current_terminal_height);
        update_and_draw_splashes(splashes);
        spawn_raindrops(raindrops, column_has_active_raindrop, current_terminal_width, gen, head_char_dist, tail_char_dist, speed_dist, threshold_dist, density_dist);
        update_and_draw_raindrops(raindrops, column_has_active_raindrop, puddles, splashes, current_terminal_height, current_terminal_width, wind_speed, wind_direction, puddle_char_dist, gen);

        std::cout.flush(); // ensure all output is written

        // pause for animation
        std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_DELAY_MS));
    }

    return 0;
}
