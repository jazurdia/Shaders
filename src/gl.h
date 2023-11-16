#pragma once
#include "SDL.h"
#include "Color.h"
#include "Fragment.h"

// globals

SDL_Window* window;
SDL_Renderer* renderer;

const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 600;

std::array<std::array<float, SCREEN_WIDTH>, SCREEN_HEIGHT> zbuffer;
std::array<std::array<float, SCREEN_WIDTH>, SCREEN_HEIGHT> zbufferToPrint;

Color currentColor = {255, 255, 255, 255}; // Initially set to white
Color clearColor = {0, 0, 0, 255}; // Initially set to black

// SDL will initiate the window and renderer
void initSDL() {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Software Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}

void setColor(const Color& color) {
    currentColor = color;
}


// Function to clear the framebuffer with the clearColor
void clear() {
    SDL_SetRenderDrawColor(renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    SDL_RenderClear(renderer);

    // reset the zbuffer
    for (auto &row : zbuffer) {
        std::fill(row.begin(), row.end(), 99999.0f);
    }
}


// Function to set a specific pixel in the framebuffer to the currentColor
void point(Fragment f) {
    if (f.position.z < zbuffer[f.position.y][f.position.x]) {
        SDL_SetRenderDrawColor(renderer, f.color.r, f.color.g, f.color.b, f.color.a);
        SDL_RenderDrawPoint(renderer, f.position.x, f.position.y);
        zbuffer[f.position.y][f.position.x] = f.position.z;
        zbufferToPrint[f.position.y][f.position.x] = f.position.z;
        // std::cout << zbuffer[f.position.y][f.position.x] << std::endl;
    }
}

// setupVertexFromObject
