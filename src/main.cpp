#include <SDL2/SDL.h>
#include <vector>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "Uniforms.h"
#include "Fragment.h"
#include "Shaders.h"
#include "Camera.h"
#include "Matrices.h"
#include "bmpwriter.h"
#include "loadObject.h"

// constants
Uniforms uniforms;
Camera camera;


std::vector<glm::vec3> setupVertexFromObject(const std::vector<glm::vec3>& vertices, const std::vector<Face>& faces){
    std::vector<glm::vec3> vertexBufferObject;

    for (const Face& face : faces) {
        for (const std::array<int, 3>& vertexIndices : face.vertexIndices) {
            glm::vec3 vertex = vertices[vertexIndices[0] - 1];
            glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);

            vertexBufferObject.push_back(vertex);
            vertexBufferObject.push_back(color);
        }
    }

    return vertexBufferObject;
}

// this will render the object.
void render(const std::vector<glm::vec3> &vertices) {
    // 1. Vertex Shader
    std::vector<Vertex> transformedVertices;
    for (int i = 0; i < vertices.size(); i+=2) {
        glm::vec3 v = vertices[i];
        glm::vec3 c = vertices[i + 1];

        Vertex vertex = {v, Color(c.x, c.y, c.z)};

        Vertex transformedVertex = vertexShader(vertex, uniforms);
        transformedVertices.push_back(transformedVertex);
    }

    // 2. Primitive Assembly
    // uses primitiveAssembly function from Shaders.h
    std::vector<std::vector<Vertex>> groupedVertices = primitiveAssembly(transformedVertices);

    // 3. Rasterization using rasterizeTriangle from Shaders.h
    std::vector<Fragment> fragments = rasterizeTriangles(groupedVertices);

    // 4. Fragment Shader. Does not uses function.

    for (Fragment fragment : fragments) {
        point(fragmentShader(fragment));
    }
}

int main (int argc, char** argv){
    initSDL();

    std:: vector<glm::vec3> vertices;
    std::vector<Face> faces;

    // load the OBJ file
    bool sucess = loadOBJ("sphere.obj", vertices, faces);
    if (!sucess) {
        std::cerr << "Error loading OBJ file" << std::endl;
        return 1;
    }

    std::vector<glm::vec3> vertexBufferObject = setupVertexFromObject(vertices, faces);

    bool running = true;

    camera.cameraPosition = glm::vec3(-1.0f, 0.5f, -1.0f);
    camera.targetPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.upVector = glm::vec3(0.0f, -1.0f, 0.0f);

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // camera

        // uniforms
        uniforms.model = createModelMatrix();
        uniforms.view = createViewMatrix(camera);
        uniforms.projection = createProjectionMatrix();
        uniforms.viewport = createViewportMatrix();

        // clear
        clear();

        // render
        render(vertexBufferObject);

        // present the framebuffer
        SDL_RenderPresent(renderer);

        // delay
        SDL_Delay(2);
    }
    // save the framebuffer to a BMP file
    writeBMP("out.bmp");

    // cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;

}



