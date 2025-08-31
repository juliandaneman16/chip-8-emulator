#include <SDL2/SDL.h>
#undef main
#include <iostream>
#include <cstdint>
#include <stack>
#include <fstream>
#include <thread>
#include <random>
using namespace std;

const int WIDTH = 64;
const int HEIGHT = 32;
const int DISPLAYSCALE = 10;
const char* FILENAME = "br8kout.ch8";
const int INSTFREQ = 1000;

//modifiable instructions
const bool newShift = false;
const bool newJump = false;
const bool newMemory = false;

//class to handle main emulation
class Emulator {
    private:
        uint8_t memory [4096]; //4KB of RAM memory
        bool display [WIDTH*HEIGHT]; //current state of display (64x32 monochrome)
        uint16_t PC; //program counter
        uint16_t I; //index register
        std::stack<uint16_t> stack; //address stack
        uint8_t delay; //delay timer
        uint8_t sound; //sound timer
        uint8_t registers[16]; //general purpose variable registers
        uint8_t keyPress; //stores last pressed key
        random_device rd; //random generator
        mt19937 gen;
        uniform_int_distribution<uint8_t> distrib;

    public:
        Emulator(): gen(rd()), distrib(0, 255){
            //store font data in memory from 050-09F
            uint8_t font[80] = {
                0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
                0x20, 0x60, 0x20, 0x20, 0x70, // 1
                0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
                0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
                0x90, 0x90, 0xF0, 0x10, 0x10, // 4
                0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
                0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
                0xF0, 0x10, 0x20, 0x40, 0x40, // 7
                0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
                0xF0, 0x90, 0xF0, 0x90, 0x90, // A
                0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
                0xF0, 0x80, 0x80, 0x80, 0xF0, // C
                0xE0, 0x90, 0x90, 0x90, 0xE0, // D
                0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
                0xF0, 0x80, 0xF0, 0x80, 0x80  // F
            };
            for(int i = 0; i < 80; i++){
                memory[0x50+i] = font[i];
            }

            //load timers at max
            delay = 255;
            sound = 255;

            //load empty screen
            for(int i = 0; i<WIDTH*HEIGHT; i++)
                display[i] = false;

            //initialize pointers
            PC = 0x200;
            I = 0x0;
            for(int i = 0; i<16; i++)
                registers[i] = 0x0;
            
            //initialize keyPress as unpressed
            keyPress = 0xFF;
        }

        //load ROM from file into memory
        bool load(const char* filename){
            //create file object
            ifstream file(filename, ios::binary | ios::ate);
            if(!file.is_open()){
                cerr << "Failed to open File" << endl;
                return false;
            }
            
            // get file size and navigate to beginning
            streamsize size = file.tellg();
            file.seekg(0, ios::beg);

            //check if size fits in memory
            if (size > (4096 - 0x200)){
                cerr << "File too big for memory" << endl;
                return false;
            }

            //read into memory
            file.read(reinterpret_cast<char*>(&memory[0x200]), size);

            return true;
        }

        bool* getDisplay(){
            return display;
        }

        void decrementTimers(){
            if (delay > 0) delay--;
            if (sound > 0) sound--;
        }

        //read instruction that PC is currently pointing at
        uint16_t fetch(){
            uint16_t instruct;
            instruct = memory[PC]*0x100 + memory[PC+1];
            PC += 2;
            return instruct;
        }

        //decode instruction (without executing)
        void debugDecode(uint16_t instruct){
            //extract bytes and nibbles
            uint8_t first   = (instruct & 0xF000) >> 12;
            uint8_t X       = (instruct & 0x0F00) >> 8;
            uint8_t Y       = (instruct & 0x00F0) >> 4;
            uint8_t N       = (instruct & 0x000F);
            uint8_t NN      = (instruct & 0x00FF);
            uint8_t NNN     = (instruct & 0x0FFF);

            /*
            std::cout << std::hex << "Instruction: " << instruct << std::dec << endl;
            std::cout << std::hex << "Masked: " << (instruct & 0xF000) << std::dec << endl;
            std::cout << std::hex << "Shifted: " << ((instruct & 0xF000) >> 12) << std::dec << endl;
            std::cout << std::hex << "Broken down: " << (int)first << " " << (int)X << " " << (int)Y << " " << (int)N << std::dec << endl;
            */
        
            //decode based on nibbles/bytes
            switch(first){
                case 0x0:
                    switch(NNN){
                        //clear screen
                        case 0x0E0:
                            cout << "clear screen" << endl;
                            break;

                        //return from subroutine
                        case 0x0EE:
                            cout << "return from subroutine" << endl;
                            break;

                        //otherwise print error message
                        default:
                            std::cerr << std::hex << "Unrecongized instruction " << (int)first << " " << (int)X << " " << (int)Y << " " << (int)N << std::dec << endl;
                    }
                    break;
                
                //jump PC to NNN
                case 0x1:
                    cout << "jump to " << NNN << endl;
                    break;
                
                //jump PC to NNN and push old PC to stack
                case 0x2:
                    cout << "execute subroutine at " << NNN << endl;
                    break;

                //set register VX to value NN
                case 0x6:
                    cout << "set register" << endl;
                    break;
                
                //add value NN to register VX
                case 0x7:
                    cout << "add to register" << endl;
                    break;
                
                //set index register to value NNN
                case 0xA:
                    cout << "set index register" << endl;
                    break;
                
                //draw sprite
                case 0xD:
                    cout << "draw sprite" << endl;
                    break;

                default:
                    std::cerr << std::hex << "Unrecongized instruction " << (int)first << " " << (int)X << " " << (int)Y << " " << (int)N << std::dec << endl;
            }
        }

        //decode instruction
        void decode(uint16_t instruct){
            //extract bytes and nibbles
            uint8_t first   = (instruct & 0xF000) >> 12;
            uint8_t X       = (instruct & 0x0F00) >> 8;
            uint8_t Y       = (instruct & 0x00F0) >> 4;
            uint8_t N       = (instruct & 0x000F);
            uint8_t NN      = (instruct & 0x00FF);
            uint16_t NNN    = (instruct & 0x0FFF);
            
            //decode based on nibbles/bytes
            switch(first){
                case 0x0:
                    switch(NNN){
                        //clear screen
                        case 0x0E0:
                            cout << "clear screen" << endl;
                            for(int i = 0; i<WIDTH*HEIGHT; i++)
                                display[i] = false;
                            break;

                        //return from subroutine
                        case 0x0EE:
                            cout << "return from subroutine" << endl;
                            PC = stack.top();
                            stack.pop();
                            break;

                        //otherwise print error message
                        default:
                            std::cerr << std::hex << "Unrecongized instruction " << (int)first << " " << (int)X << " " << (int)Y << " " << (int)N << std::dec << endl;
                    }
                    break;
                
                //jump PC to NNN
                case 0x1:
                    cout <<  std::hex << "jump to " << (int)NNN << std::dec << endl;
                    PC = NNN;
                    break;
                
                //jump PC to NNN and push old PC to stack
                case 0x2:
                    cout << "execute subroutine" << endl;
                    stack.push(PC);
                    PC = NNN;
                    break;
                
                //skip next instruction if VX is equal to NN
                case 0x3:
                    cout << "skip next instruction if VX is equal to NN" << endl;
                    if(registers[X] == NN){
                        PC += 2;
                    }
                    break;

                //skip next instruction if VX isn't equal to NN
                case 0x4:
                    cout << "skip next instruction if VX isn't equal to NN" << endl;
                    if(registers[X] != NN){
                        PC += 2;
                    }
                    break;
                
                //skip next instruction if VX equals VY
                case 0x5:
                    cout << "skip next instruction if VX equals VY" << endl;
                    if(registers[X] == registers[Y]){
                        PC += 2;
                    }
                    break;

                //set register VX to value NN
                case 0x6:
                    cout << "set register" << endl;
                    registers[X] = NN;
                    break;
                
                //add value NN to register VX
                case 0x7:
                    cout << "add to register" << endl;    
                    registers[X] += NN;
                    break;
                
                //logic and arithmetic
                case 0x8:
                    switch(N){
                        //set VX to VY
                        case 0x0:
                            cout << "set VX to VY" << endl;
                            registers[X] = registers[Y];
                            break;
                        
                        //binary OR
                        case 0x1:
                            cout << "binary OR" << endl;
                            registers[X] = registers[X] | registers[Y];
                            break;

                        //binary AND
                        case 0x2:
                            cout << "binary AND" << endl;
                            registers[X] = registers[X] & registers[Y];
                            break;

                        //binary XOR
                        case 0x3:
                            cout << "binary XOR" << endl;
                            registers[X] = registers[X] ^ registers[Y];
                            break;

                        //Add
                        case 0x4: {
                            cout << "add" << endl;
                            uint8_t vx = registers[X];
                            uint8_t vy = registers[Y];

                            registers[X] = vx + vy;

                            if ((int)vx + (int)vy > 255){registers[0xF] = 1;} 
                            else {registers[0xF] = 0;}
                            
                            break;
                        }

                        //Subtract VX-VY
                        case 0x5: {
                            cout << "subtract VX-VY" << endl; 

                            uint8_t vx = registers[X];
                            uint8_t vy = registers[Y];

                            registers[X] = vx - vy;

                            if ((int)vx >= (int)vy){registers[0xF] = 1;} 
                            else {registers[0xF] = 0;}
                            
                            break;
                        }
                        
                        //Shift right
                        case 0x6: {
                            cout << "shift right" << endl;
                            if(!newShift){
                                registers[X] = registers[Y];
                            }
                            uint8_t vx = registers[X];
                            registers[X] = vx >> 1;
                            registers[0xF] = vx & 0x01;
                            break;
                        }

                        //Subtract VY-VX
                        case 0x7: {
                            cout << "subtract VY-VX" << endl;

                            uint8_t vx = registers[X];
                            uint8_t vy = registers[Y];

                            registers[X] = vy - vx;

                            if ((int)vy >= (int)vx){registers[0xF] = 1;} 
                            else {registers[0xF] = 0;}

                            break;
                        }

                        //Shift left
                        case 0xE: {
                            cout << "shift left" << endl;
                            if(!newShift){
                                registers[X] = registers[Y];
                            }
                            uint8_t vx = registers[X];
                            registers[X] = vx << 1;
                            registers[0xF] = (vx & 0x80) >> 7;
                            break;
                        }
                    }
                    break;

                //skip next instruction if VX doesn't equal VY
                case 0x9:
                    cout << "skip next instruction if VX doesn't equal VY" << endl;
                    if(registers[X] != registers[Y]){
                        PC += 2;
                    }
                    break;

                //set index register to value NNN
                case 0xA:
                    cout << "set index register" << endl;
                    I = NNN;
                    break;
                
                //jump with offset
                case 0xB:
                    cout << "jump with offset" << endl;
                    PC = NNN;
                    if (newJump){
                        PC += registers[X];
                    }
                    break;

                //random
                case 0xC:
                    cout << "random" << endl;
                    registers[X] = distrib(gen) & NN;
                    break;

                //draw sprite
                case 0xD: {
                    cout << "draw sprite" << endl;
                    uint8_t yCor = registers[Y]%HEIGHT;
                    registers[0xF] = 0;

                    for(int i = 0; i<N; i++){
                        uint8_t xCor = registers[X]%WIDTH;
                        uint8_t rowData = memory[I+i];
                        for(int j = 0; j<8; j++){
                            uint8_t pixel = rowData & (0x80 >> j);
                            if(pixel){
                                if(display[yCor*WIDTH + xCor]){
                                    registers[0xF] = 1;
                                }
                                display[yCor*WIDTH + xCor] ^= 1;
                            }
                            
                            xCor++;
                            if (xCor >= WIDTH){
                                break;
                            }
                        }
                        yCor++;
                        if (yCor >= HEIGHT){
                            break;
                        }
                    }
                    break;
                }

                //skip if key
                case 0xE:
                    switch(NN){
                        //skip if key is pressed
                        case 0x9E:
                            if(registers[X] == keyPress)
                                PC += 2;
                            break;
                        
                        //skip if key isn't pressed
                        case 0xA1:
                            if(registers[X] != keyPress)
                                PC += 2;
                            break;
                    }
                    break;
                
                //timers
                case 0xF:
                    switch(NN){
                        //set VX to delay timer value
                        case 0x07:
                            registers[X] = delay;
                            break;

                        //set delay timer to VX
                        case 0x15:
                            delay = registers[X];
                            break;

                        //set sound timer to VX
                        case 0x18:
                            sound = registers[X];
                            break;
                        
                        //add to index
                        case 0x1E: {
                            uint8_t vx = registers[X];
                            if((int)I+vx > 255)
                                registers[0xF] = 1;
                            I = I + vx;
                            break;
                        }

                        //get key
                        case 0x0A:
                            if(keyPress > 0x0F){
                                PC -= 2;
                            }
                            break;

                        //font char
                        case 0x29:
                            I = 0x50 + (registers[X])*5;
                            break;
                        
                        //decimal conversion
                        case 0x33:
                            int first,second,third;

                            first = registers[X]/100;
                            second = (registers[X]/10)%10;
                            third = registers[X]%10;

                            memory[I] = first;
                            memory[I+1] = second;
                            memory[I+2] = third;
                            break;
                        
                        //store memory
                        case 0x55:
                            for(int i = 0; i <= X; i++){
                                memory[I+i] = registers[i];
                            }
                            if(!newMemory){
                                I = I+X+1;
                            }
                            break;

                        //load memory
                        case 0x65:
                            for(int i = 0; i <= X; i++){
                                registers[i] = memory[I+i];
                            }
                            if(!newMemory){
                                I = I+X+1;
                            }
                            break;

                        default:
                            std::cerr << std::hex << "Unrecongized instruction " << (int)first << " " << (int)X << " " << (int)Y << " " << (int)N << std::dec << endl;

                    }

                default:
                    std::cerr << std::hex << "Unrecongized instruction " << (int)first << " " << (int)X << " " << (int)Y << " " << (int)N << std::dec << endl;
            }
        }

        //store last pressed key
        void keyPressed(uint8_t key){
            keyPress = key;
            cout << "key pressed: " << key << endl;
        }
};

//class to handle display screen (will be different for microcontroller iteration)
class Display {
    private:
        SDL_Window* window;
        SDL_Renderer* renderer;

    public:
        //initialize display
        Display(){
            if (SDL_Init(SDL_INIT_VIDEO) < 0)
                std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            window = SDL_CreateWindow("chip 8 window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH*DISPLAYSCALE, HEIGHT*DISPLAYSCALE, SDL_WINDOW_SHOWN);
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        }

        //update screen using framebuffer
        void drawScreen(bool display[]){
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);         // black
            SDL_RenderClear(renderer);                              // set background black

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);         // white
            for (int y = 0; y<HEIGHT; y++){                         // iterate through pixels, if filled draw a white rectangle
                for (int x = 0; x<WIDTH; x++){
                    if(display[y*WIDTH+x]){
                        SDL_Rect rect = {x*DISPLAYSCALE, y*DISPLAYSCALE, DISPLAYSCALE, DISPLAYSCALE};
                        SDL_RenderFillRect(renderer, &rect);
                    }
                }
            }

            SDL_RenderPresent(renderer);    //render the frame
        }

        //close display
        ~Display(){
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
        }
};

void emuLoop(Emulator* emu, bool* running){
    
    while(*running){
        emu->decode(emu->fetch());
        SDL_Delay(1);
    }
}

void disLoop(Emulator* emu, Display* dis, bool* running){
    SDL_Event e;
    while(*running){
        while(SDL_PollEvent(&e)){
            if(e.type == SDL_QUIT){
                *running = false;
            }
            else if (e.type == SDL_KEYDOWN){
                switch(e.key.keysym.sym){
                    case SDLK_a:
                        emu->keyPressed(10);
                        break;
                    case SDLK_b:
                        emu->keyPressed(11);
                        break;
                    case SDLK_c:
                        emu->keyPressed(12);
                        break;
                    case SDLK_d:
                        emu->keyPressed(13);
                        break;
                    case SDLK_e:
                        emu->keyPressed(14);
                        break;
                    case SDLK_f:
                        emu->keyPressed(15);
                        break;
                    case SDLK_0:
                        emu->keyPressed(0);
                        break;
                    case SDLK_1:
                        emu->keyPressed(1);
                        break;
                    case SDLK_2:
                        emu->keyPressed(2);
                        break;
                    case SDLK_3:
                        emu->keyPressed(3);
                        break;
                    case SDLK_4:
                        emu->keyPressed(4);
                        break;
                    case SDLK_5:
                        emu->keyPressed(5);
                        break;
                    case SDLK_6:
                        emu->keyPressed(6);
                        break;
                    case SDLK_7:
                        emu->keyPressed(7);
                        break;
                    case SDLK_8:
                        emu->keyPressed(8);
                        break;
                    case SDLK_9:
                        emu->keyPressed(9);
                        break;
                }
            }
            else if (e.type == SDL_KEYUP) {
                emu->keyPressed(0xFF);   // no key pressed
            }
        }
        dis->drawScreen(emu->getDisplay());
        emu->decrementTimers();
        SDL_Delay(16);
    }
}


int main (int argc, char* argv[]){
    Emulator emu = Emulator();
    Display dis = Display();
    emu.load(FILENAME);

    /*
    uint16_t instruct;
    for(int i = 0; i<10; i++) {
        instruct = emu.fetch();
        cout << instruct << endl;
        emu.debugDecode(instruct);
    }
    */

    
    bool running = true;

    thread emuThread(emuLoop, &emu, &running);
    
    disLoop(&emu, &dis, &running);
    
    emuThread.join();
    

    return 0;
}
