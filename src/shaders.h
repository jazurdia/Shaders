#pragma once
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include "FastNoise.h"
#include "uniforms.h"
#include "fragment.h"
#include "noise.h"
#include "print.h"

static int frame = 0;

Vertex vertexShader(const Vertex& vertex, const Uniforms& uniforms) {
    // Apply transformations to the input vertex using the matrices from the uniforms
    glm::vec4 clipSpaceVertex = uniforms.projection * uniforms.view * uniforms.model * glm::vec4(vertex.position, 1.0f);

    // Perspective divide
    glm::vec3 ndcVertex = glm::vec3(clipSpaceVertex) / clipSpaceVertex.w;

    // Apply the viewport transform
    glm::vec4 screenVertex = uniforms.viewport * glm::vec4(ndcVertex, 1.0f);
    // w lo manda al centro.
    
    // Transform the normal
    glm::vec3 transformedNormal = glm::mat3(uniforms.model) * vertex.normal;
    transformedNormal = glm::normalize(transformedNormal);

    glm::vec3 transformedWorldPosition = glm::vec3(uniforms.model * glm::vec4(vertex.position, 1.0f));

    // Return the transformed vertex as a vec3
    return Vertex{
        glm::vec3(screenVertex),
        transformedNormal,
        vertex.tex,
        transformedWorldPosition,
        vertex.position
    };
}

Fragment fragmentShaderSun(Fragment& fragment) {
    Color color;

    // Define los colores del sol
    glm::vec3 brightColor = glm::vec3(1.0f, 0.8f, 0.0f); // Amarillo brillante para el centro
    glm::vec3 darkColor = glm::vec3(1.0f, 0.4f, 0.0f); // Naranja oscuro para los bordes y las manchas

    float x = fragment.originalPos.x;
    float y = fragment.originalPos.y;
    float z = fragment.originalPos.z;

    glm::vec3 uv = glm::vec3(
            atan2(x, z) / (2.0f * M_PI),
            acos(y / sqrt(x*x + y*y + z*z)) / M_PI,
            sqrt(x*x + y*y + z*z)
    );

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    // Escala grande para manchas grandes
    float noiseScale = 0.04f;
    float baseNoise = noiseGenerator.GetNoise(uv.x * noiseScale, uv.y * noiseScale);

    // Agregamos más capas de ruido para crear variaciones en las manchas
    float noiseScaleLayer = 0.08f; // Una segunda capa de ruido para más detalle en las manchas
    float layerNoise = noiseGenerator.GetNoise(uv.x * noiseScaleLayer, uv.y * noiseScaleLayer);

    // Mezclamos las capas de ruido, asegurándonos de que la baseNoise sea la dominante
    float mixedNoise = baseNoise * 0.6f + layerNoise * 0.4f;

    // Normalizamos el ruido mezclado para que esté dentro del rango de 0 a 1
    mixedNoise = mixedNoise * 0.5f + 0.5f;
    mixedNoise = glm::clamp(mixedNoise, 0.0f, 1.0f);

    // Mezclamos los colores del sol basándonos en el ruido
    glm::vec3 tmpColor = mix(brightColor, darkColor, mixedNoise);

    color = Color(tmpColor.x, tmpColor.y, tmpColor.z);

    // La intensidad de la iluminación del sol se mantiene constante
    fragment.color = color * 1.0f; // Multiplicador de intensidad fijo para evitar sombras

    return fragment;
}

Fragment fragmentShaderEarth5(Fragment& fragment) {
    Color color;

    glm::vec3 groundColor = glm::vec3(0.44f, 0.51f, 0.33f);
    glm::vec3 oceanColor = glm::vec3(0.12f, 0.38f, 0.57f);
    glm::vec3 vegetationColor = glm::vec3(0.13f, 0.55f, 0.13f);
    glm::vec3 cloudColor = glm::vec3(0.9f, 0.9f, 0.9f); // Un blanco ligeramente gris para las nubes
    glm::vec3 iceColor = glm::vec3(1.0f, 1.0f, 1.0f); // Blanco puro para el hielo polar

    float x = fragment.originalPos.x;
    float y = fragment.originalPos.y;
    float z = fragment.originalPos.z;
    float radius = sqrt(x*x + y*y + z*z);

    glm::vec3 uv = glm::vec3(
            atan2(x, z),
            acos(y / radius),
            radius
    );

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // Capa base: Océano y terreno
    float baseNoiseZoom = 100.0f;
    float baseNoise = noiseGenerator.GetNoise(uv.x * baseNoiseZoom, uv.y * baseNoiseZoom, uv.z * baseNoiseZoom);

    // Capa de terreno: detalles del terreno
    float terrainNoiseZoom = 300.0f;
    float terrainNoise = noiseGenerator.GetNoise(uv.x * terrainNoiseZoom + 1000, uv.y * terrainNoiseZoom, uv.z * terrainNoiseZoom);

    // Capa de vegetación: solo sobre el terreno
    float vegetationNoiseZoom = 600.0f;
    float vegetationNoise = noiseGenerator.GetNoise(uv.x * vegetationNoiseZoom + 2000, uv.y * vegetationNoiseZoom, uv.z * vegetationNoiseZoom);

    // Capa de nubes
    float cloudNoiseZoom = 1200.0f; // Aumentar para nubes más grandes y menos numerosas
    float cloudNoise = noiseGenerator.GetNoise(uv.x * cloudNoiseZoom + 3000, uv.y * cloudNoiseZoom, uv.z * cloudNoiseZoom);

    // Lógica para mezclar océano y terreno
    glm::vec3 tmpColor = (baseNoise < 0.0f) ? oceanColor : groundColor;
    tmpColor = mix(tmpColor, groundColor, glm::clamp(terrainNoise, 0.0f, 1.0f));

    // Asegurarse de que la vegetación solo se aplique en áreas de terreno
    if (baseNoise > 0.2f) { // Ajustar este umbral según sea necesario
        tmpColor = mix(tmpColor, vegetationColor, glm::clamp(vegetationNoise, 0.0f, 1.0f));
    }

    // Capa de hielo polar: aplicada en función de la latitud (coordenada Y)
    float poleThreshold = 0.8f; // Umbral para la aplicación de hielo en los polos
    float poleFactor = glm::smoothstep(poleThreshold, 1.0f, abs(y / radius));
    tmpColor = mix(tmpColor, iceColor, poleFactor);

    // Mezcla suave para las nubes
    float cloudFactor = glm::smoothstep(0.4f, 0.6f, cloudNoise);
    tmpColor = mix(tmpColor, cloudColor, cloudFactor);

    color = Color(tmpColor.x, tmpColor.y, tmpColor.z);

    // Aplicar la intensidad de iluminación
    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment fragmentShaderGasGiant(Fragment& fragment) {
    Color color;

    // Define los colores
    glm::vec3 blueColor = glm::vec3(0.0f, 0.0f, 1.0f); // Azul
    glm::vec3 purpleColor = glm::vec3(0.5f, 0.0f, 0.5f); // Morado
    glm::vec3 redColor = glm::vec3(1.0f, 0.0f, 0.0f); // Rojo

    float x = fragment.originalPos.x;
    float y = fragment.originalPos.y;
    float z = fragment.originalPos.z;

    glm::vec3 uv = glm::vec3(
            atan2(x, z) / (2.0f * M_PI),
            acos(y / sqrt(x*x + y*y + z*z)) / M_PI,
            sqrt(x*x + y*y + z*z)
    );

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // Generar ruido para las capas de color
    float noiseZoomBlue = 0.1f;
    float noiseZoomPurple = 0.15f;
    float noiseZoomRed = 0.2f;

    float noiseValueBlue = noiseGenerator.GetNoise(uv.x * noiseZoomBlue, uv.y * noiseZoomBlue, uv.z * noiseZoomBlue);
    float noiseValuePurple = noiseGenerator.GetNoise(uv.x * noiseZoomPurple + 1000, uv.y * noiseZoomPurple, uv.z * noiseZoomPurple);
    float noiseValueRed = noiseGenerator.GetNoise(uv.x * noiseZoomRed + 2000, uv.y * noiseZoomRed, uv.z * noiseZoomRed);

    // Normalizar los valores de ruido
    noiseValueBlue = (noiseValueBlue + 1.0f) / 2.0f;
    noiseValuePurple = (noiseValuePurple + 1.0f) / 2.0f;
    noiseValueRed = (noiseValueRed + 1.0f) / 2.0f;

    // Aplicar los colores en función del valor de ruido, superponiéndolos
    glm::vec3 tmpColor = glm::vec3(0.0f, 0.0f, 0.0f); // Color inicial neutro
    tmpColor += blueColor * noiseValueBlue;
    tmpColor += purpleColor * noiseValuePurple;
    tmpColor += redColor * noiseValueRed;

    // Limitar el color para que no supere el valor máximo
    tmpColor = glm::clamp(tmpColor, 0.0f, 1.0f);

    color = Color(tmpColor.x, tmpColor.y, tmpColor.z);

    // Aplicar la intensidad de iluminación
    fragment.color = color * fragment.intensity;

    return fragment;
}





