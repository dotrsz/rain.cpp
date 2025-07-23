```
░▒▓███████▓▒░ ░▒▓██████▓▒░░▒▓█▓▒░▒▓███████▓▒░        ░▒▓██████▓▒░░▒▓███████▓▒░░▒▓███████▓▒░  
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░ 
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░ 
░▒▓███████▓▒░░▒▓████████▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓███████▓▒░░▒▓███████▓▒░  
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓█▓▒░        
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓██▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░        
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓██▓▒░░▒▓██████▓▒░░▒▓█▓▒░      ░▒▓█▓▒░        
```


A simple C++ program that creates an ASCII art rain simulation in your terminal.

## Description

This program simulates falling rain in the terminal using ASCII characters. It's a fun, lightweight application that can be customized with various command-line options to change the appearance and behavior of the rain.

## Building the Program

To build the program, you'll need a C++ compiler (like g++) and `make`.

1.  **Clone the repository or download the source code.**
2.  **Navigate to the project directory.**
3.  **Run the `make` command:**

    ```bash
    make
    ```

    This will compile the `rain.cpp` file and create an executable named `rain`.

## Running the Program

To run the program, simply execute the compiled binary:

```bash
./rain
```

### Options

You can customize the rain simulation with the following command-line options:

| Option                | Description                                           | Default |
| --------------------- | ----------------------------------------------------- | ------- |
| `--help`              | Display the help message and exit.                    |         |
| `--fps <value>`       | Set the frames per second.                            | 90      |
| `--raindrop-length <value>` | Set the height of the raindrops.                      | 7       |
| `--puddle-lifetime <value>` | Set how many frames puddles last.                     | 30      |
| `--splash-lifetime <value>` | Set how many frames splashes last.                    | 3       |
| `--speed-min <value>`   | Set the minimum speed threshold.                      | 1       |
| `--speed-max <value>`   | Set the maximum speed threshold.                      | 8       |
| `--wind-direction <left/right>` | Set the wind direction.                       |         |
| `--wind-speed <value>`  | Set the wind speed.                                   |         |
| `--lightning`         | Enable the lightning effect.                          |         |

### Examples

*   **Run with default settings:**

    ```bash
    ./rain
    ```

*   **Run with a lower frame rate and wind:**

    ```bash
    ./rain --fps 30 --wind-direction right --wind-speed 1
    ```

*   **Run with longer raindrops and lightning:**

    ```bash
    ./rain --raindrop-length 10 --lightning
    ```

## Cleaning Up

To remove the compiled executable, run:

```bash
make clean
```
