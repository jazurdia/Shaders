#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>
#include <vector>
#include <cassert>
#include "color.h"
#include "print.h"
#include "framebuffer.h"
#include "uniforms.h"
#include "shaders.h"
#include "fragment.h"
#include "triangle.h"
#include "camera.h"
#include "ObjLoader.h"
#include "noise.h"


SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Color currentColor;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error: Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Software Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Error: Failed to create SDL window: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error: Failed to create SDL renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    setupNoise();

    return true;
}

void setColor(const Color& color) {
    currentColor = color;
}

void render(const std::vector<glm::vec3>& VBO, const Uniforms& uniforms) {
    // 1. Vertex Shader
    std::vector<Vertex> transformedVertices(VBO.size() / 3);
    for (size_t i = 0; i < VBO.size() / 3; ++i) {
        Vertex vertex = { VBO[i * 3], VBO[i * 3 + 1], VBO[i * 3 + 2] };
        transformedVertices[i] = vertexShader(vertex, uniforms);
    }

    // 2. Primitive Assembly
    std::vector<std::vector<Vertex>> assembledVertices(transformedVertices.size() / 3);

    for (size_t i = 0; i < transformedVertices.size() / 3; i++) {
        Vertex edge1 = transformedVertices[3 * i];
        Vertex edge2 = transformedVertices[3 * i + 1];
        Vertex edge3 = transformedVertices[3 * i + 2];

        assembledVertices[i] = { edge1, edge2, edge3 };
    }

    // 3. Rasterization
    std::vector<Fragment> fragments;

    for (auto & assembledVertice : assembledVertices) {
        std::vector<Fragment> rasterizedTriangle = triangle(
                assembledVertice[0],
                assembledVertice[1],
                assembledVertice[2]
        );

        for(const auto& fragment : rasterizedTriangle) {
            fragments.push_back(fragment);
        }
    }

    // 4. Fragment Shader
    // Asumiendo que 'fragments' es el vector que contiene los fragmentos
    for (auto & i : fragments) {
        const Fragment& fragment = fragmentShaderSun(i);
        // Aplicar el fragment shader para calcular el color final
        point(fragment);  // Ten en cuenta las condiciones de carrera potenciales aquí
    }

}

glm::mat4 createViewportMatrix(size_t screenWidth, size_t screenHeight) {
    glm::mat4 viewport = glm::mat4(1.0f);

    // Scale
    viewport = glm::scale(viewport, glm::vec3(screenWidth / 2.0f, screenHeight / 2.0f, 0.5f));

    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));

    return viewport;
}

int main(int argc, char* argv[]) {
    if (!init()) {
        return 1;
    }

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> texCoords;
    std::vector<Face> faces;
    std::vector<glm::vec3> vertexBufferObject; // This will contain both vertices and normals

    loadOBJ("sphere.obj", vertices, normals, texCoords, faces);
    /* loadTexture("models/diablo3.png"); */

    for (const auto& face : faces)
    {
        for (int i = 0; i < 3; ++i)
        {
            // Get the vertex position
            glm::vec3 vertexPosition = vertices[face.vertexIndices[i]];

            // Get the normal for the current vertex
            glm::vec3 vertexNormal = normals[face.normalIndices[i]];
            
            // Get the texture for the current vertex
            glm::vec3 vertexTexture = texCoords[face.texIndices[i]];

            // Add the vertex position and normal to the vertex array
            vertexBufferObject.push_back(vertexPosition);
            vertexBufferObject.push_back(vertexNormal);
            vertexBufferObject.push_back(vertexTexture);
        }
    }

    Uniforms uniforms;

    glm::mat4 model = glm::mat4(1);
    glm::mat4 view = glm::mat4(1);
    glm::mat4 projection = glm::mat4(1);

    glm::vec3 translationVector(0.0f, 0.0f, 0.0f);
    float a = 45.0f;
    glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f); // Rotate around the Y-axis
    glm::vec3 scaleFactor(1.0f, 1.0f, 1.0f);

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), translationVector);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), scaleFactor);

    // Initialize a Camera object
    Camera camera;
    camera.cameraPosition = glm::vec3(0.0f, 0.0f, -3.0f);
    camera.targetPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    // Projection matrix
    float fovInDegrees = 45.0f;
    float aspectRatio = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT); // Assuming a screen resolution of 800x600
    float nearClip = 0.1f;
    float farClip = 100.0f;
    uniforms.projection = glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);

    uniforms.viewport = createViewportMatrix(SCREEN_WIDTH, SCREEN_HEIGHT);
    Uint32 frameStart, frameTime;
    std::string title = "FPS: ";
    int speed = 100;

    bool running = true;
    while (running) {
        frame += 1;
        frameStart = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                 switch (event.key.keysym.sym) {
                    case SDLK_LEFT:
                        camera.cameraPosition.x += -speed;
                    break;
                    case SDLK_RIGHT:
                        camera.cameraPosition.x += speed;
                    break;
                    case SDLK_UP:
                        camera.cameraPosition.y += -speed;
                    break;
                        case SDLK_DOWN:
                        camera.cameraPosition.y += speed;
                    break;
                }
            }
        }

        a += 1;
        glm::mat4 rotation = glm::rotate(glm::mat4(90.0f), glm::radians(a), rotationAxis);

        // Calculate the model matrix
        uniforms.model = translation * rotation * scale;

        // Create the view matrix using the Camera object
        uniforms.view = glm::lookAt(
            camera.cameraPosition, // The position of the camera
            camera.targetPosition, // The point the camera is looking at
            camera.upVector        // The up vector defining the camera's orientation
        );

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        clearFramebuffer();

        render(vertexBufferObject, uniforms);

        renderBuffer(renderer);

        frameTime = SDL_GetTicks() - frameStart;

        // Calculate frames per second and update window title
        if (frameTime > 0) {
            std::ostringstream titleStream;
            titleStream << "FPS: " << 1000.0 / frameTime;  // Milliseconds to seconds
            SDL_SetWindowTitle(window, titleStream.str().c_str());
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
