# chip-8-emulator
Chip-8 emulator

Short video running SharpenedSpoon's chip-8 breakout game
https://github.com/user-attachments/assets/476b4bac-afa2-41ff-8989-684805327ad0

Command in terminal to run:
$ g++ emulator.cpp -IC:/msys64/mingw64/include/SDL2 -LC:/msys64/mingw64/lib -lmingw32 -lSDL2main -lSDL2 -mconsole -o emulator.exe -pthread

Change filename on line 14 to run different games (and include .ch8 file in folder)
