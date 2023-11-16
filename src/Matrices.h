#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // Ensure this header is included for transformation functions

float a = 3.14f / 3.0f;

glm::mat4 createModelMatrix() {
    glm::mat4 translate = glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians(a++), glm::vec3(0.0f, 1.0f, 0.0f));

    return translate * scale * rotation;
}


// creates a perspective projection matrix
glm::mat4 createProjectionMatrix() {
    float fovInDegrees = 45.0f;
    float aspectRatio = SCREEN_WIDTH / SCREEN_HEIGHT;
    float nearClip = 0.1f;
    float farClip = 100.0f;
    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);
    // Scale
    viewport = glm::scale(viewport, glm::vec3(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 0.5f));
    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));
    return viewport;
}



