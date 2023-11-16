#include <vector>
#include "SDL.h"
#include "glm/glm.hpp"
#include "Uniforms.h"
#include "Vertex.h"
#include "Fragment.h"
#include "triangle.h"
/**
std::vector<glm::vec3> vertexShader(const std::vector<glm::vec3>& vertices, Uniforms u){
    std::vector<glm::vec3> transformedVertices;
    for (int i = 0; i < vertices.size(); i+= 2){
        glm::vec3 v = vertices[i];
        glm::vec3 c = vertices[i+1];

        Vertex vertex = {v, Color(c.x, c.y, c.z)};

        Vertex transformedVertex = vertexShader(vertex, u);
    }
    return transformedVertices;
}
 **/

// Vertices intro triangles.
std::vector<std::vector<Vertex>> primitiveAssembly (
        const std::vector<Vertex>& transformedVertices
        ) {;
    std::vector<std::vector<Vertex>> groupedVertices;

    for(int i = 0; i < transformedVertices.size(); i += 3){
        std::vector<Vertex> vertexGroup;
        vertexGroup.push_back(transformedVertices[i]);
        vertexGroup.push_back(transformedVertices[i+1]);
        vertexGroup.push_back(transformedVertices[i+2]);

        groupedVertices.push_back(vertexGroup);
    }
    return groupedVertices;
}



// Rasterizes a collection of triangles into fragments
std::vector<Fragment> rasterizeTriangles(const std::vector<std::vector<Vertex>>& triangles) {
    std::vector<Fragment> fragments;

    // Iterate over each triangle and rasterize it
    for (const std::vector<Vertex>& triangleVertices : triangles) {
        // Rasterize the triangle into fragments
        std::vector<Fragment> rasterizedTriangle = triangle(
                triangleVertices[0],
                triangleVertices[1],
                triangleVertices[2]
        );

        // Add the rasterized fragments to the main collection
        fragments.insert(
                fragments.end(),
                rasterizedTriangle.begin(),
                rasterizedTriangle.end()
        );
    }

    return fragments;
}

Fragment fragmentShader(Fragment& fragment) {
    fragment.color = fragment.color * fragment.intensity;
    return fragment;
}