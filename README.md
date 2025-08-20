```
░▒▓███████▓▒░ ░▒▓██████▓▒░░▒▓█▓▒░▒▓███████▓▒░        ░▒▓██████▓▒░░▒▓███████▓▒░░▒▓███████▓▒░
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░
░▒▓███████▓▒░░▒▓████████▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓███████▓▒░░▒▓███████▓▒░
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓█▓▒░
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓██▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓██▓▒░░▒▓██████▓▒░░▒▓█▓▒░      ░▒▓█▓▒░
```

# rain.cpp

make it rain in yr terminal wih the matrix x blade runner vibes.

## local hax

ensure you have a c++ compiler (like g++) and make.

### building

```bash
make
```

this will compile the code and drop an executable in the `bin` directory.

### (blade) running

```bash
./bin/rain
```

### cleaning

```bash
make clean
```

## docker hax

if you're all about that container life, you can use docker to build and run.

### image building

```bash
make docker-build
```

or, if you're not into make:

```bash
docker build -t rain-app .
```

### image running

```bash
make docker-run
```

or, the long way:

```bash
docker run -it --rm rain-app
```

you can pass in any of the command-line flags too:

```bash
docker run -it --rm rain-app --fps 30 --wind-direction right --wind-speed 1
```

## make hax

make targets for building and (blade) running

*   `make all`: build the application locally
*   `make clean`: remove build artifacts
*   `make docker-build`: build the docker image
*   `make docker-run`: run the application in a docker container
*   `make help`: show this help message

## flags

| flag                       | description                                           | default |
| -------------------------- | ----------------------------------------------------- | ------- |
| `--help`                   | show the help message and exit.                       |         |
| `--fps <value>`            | set the frames per second.                            | 90      |
| `--raindrop-length <value>`  | set the height of the raindrops.                      | 7       |
| `--puddle-lifetime <value>`  | set how many frames puddles last.                     | 30      |
| `--splash-lifetime <value>`  | set how many frames splashes last.                    | 3       |
| `--speed-min <value>`      | set the minimum speed threshold.                      | 1       |
| `--speed-max <value>`      | set the maximum speed threshold.                      | 8       |
| `--wind-direction <left/right>` | set the wind direction.                       |         |
| `--wind-speed <value>`     | set the wind speed.                                   |         |
| `--lightning`              | turn on the lightning effect.                         |         |
| `--lightning-duration <value>` | set how many frames the lightning flash lasts.        | 3       |
| `--density <value>`        | set the percentage chance of a new raindrop spawning. | 25      |


## themes

themes are now defined in files in the `themes` directory. each `.theme` file defines the colors and characters for a theme. you can create your own themes by creating a new `.theme` file in the `themes` directory.

the format of a `.theme` file is a simple key-value format. comments are allowed using `#`. here is an example:

```
# my cool theme

# colors
head_color: "\033[95m"
tail1_color: "\033[35m"
# ...

# characters
head_chars: "abc"
tail_chars: "def"
puddle_chars: "ghi"
```

you can use any of the themes in the `themes` directory by passing the file name (without the `.theme` extension) to the `--theme` option. for example, to use the `matrix.theme` file, you would use `--theme matrix`.

if you do not specify a theme to use, rain.cpp will use the default theme for the discerning hacker.

