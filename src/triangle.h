#pragma once
#include "gl.h"
#include "Vertex.h"

glm::vec3 lightPos = glm::vec3(150.0f, 150.0f, 150.0f);

glm::vec3 barycentricCoordinates(const glm::vec3& P, const glm::vec3& A, const glm::vec3& B, const glm::vec3& C) {
    glm::vec3 s[2];
    for (int i = 0; i < 2; i++) {
        s[i][0] = C[i] - A[i];
        s[i][1] = B[i] - A[i];
        s[i][2] = A[i] - P[i];
    }
    glm::vec3 u = glm::cross(s[0], s[1]);
    if (std::abs(u[2]) > 1e-2) {
        return {1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z};
    }
    return {-1, 1, 1};
}

std::vector<Fragment> triangle(const Vertex& a, const Vertex& b, const Vertex& c) {
    glm::vec3 A = a.position;
    glm::vec3 B = b.position;
    glm::vec3 C = c.position;

    // Calculate the bounding box of the triangle
    float minX = std::min(std::min(A.x, B.x), C.x);
    float minY = std::min(std::min(A.y, B.y), C.y);
    float maxX = std::max(std::max(A.x, B.x), C.x);
    float maxY = std::max(std::max(A.y, B.y), C.y);

    // Cast to int
    minX = std::floor(minX);
    minY = std::floor(minY);
    maxX = std::ceil(maxX);
    maxY = std::ceil(maxY);

    // Calculate the normal
    glm::vec3 normal = glm::normalize(glm::cross(B - A, C - A));

    std::vector<Fragment> fragments;
    // Iterate over each point in the bounding box
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            glm::vec3 P = glm::vec3(x, y, 0);

            // Calculate the barycentric coordinates
            glm::vec3 barycentric = barycentricCoordinates(P, A, B, C);

            // If the point is inside the triangle
            if (barycentric.x >= 0 && barycentric.y >= 0 && barycentric.z >= 0) {
                // Calculate the depth
                float z = A.z * barycentric.x + B.z * barycentric.y + C.z * barycentric.z;
//                std::cout << z << std::endl;

                // Calculate the intensity
                // glm::vec3 lightDir = glm::normalize(lightPos - P);  // Otro tipo de luz
                glm::vec3 lightDir = glm::normalize(lightPos - a.position);  // Flat shading
                float intensity = glm::dot(normal, lightDir);

                // Create the fragment
                Fragment fragment = Fragment{
                        glm::vec3(x, y, z),
                        a.color * barycentric.x + b.color * barycentric.y + c.color * barycentric.z,
                        intensity
                };

                // Add the fragment to the list
                fragments.push_back(fragment);
            }
        }
    }
    return fragments;
}