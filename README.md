# SDL Pong
Pong, made using SDL2 and half a day's worth of motivation

## Controls
* `Enter` to start each round
* `Space` to play again after a player has reached 10 points
### Left Player
* `W` to move up
* `D` to move down
### Right Player
* `Up Arrow` to move up
* `Down Arrow` to move down

## Running
Just run `pong.exe` from the game's folder 👍  
<br />
<u>Note:</u> if you're building from source, run `bin/pong.exe` from the root folder, so it can pull from `assets` properly

## Building
Place your [SDL2](https://github.com/libsdl-org/SDL/releases/) folder in `include/`, and `SDL2.dll` in `bin/`  
Then run `make` from the project's root directory (with either `opt` or `debug` modes, and with `exe` to delete the `main.o` file)