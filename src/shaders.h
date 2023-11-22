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
    // Define los colores del sol
    glm::vec3 sunColor1 = glm::vec3(252.0f / 255.0f, 211.0f / 255.0f, 0.0f / 255.0f);
    glm::vec3 sunColor2 = glm::vec3(252.0f / 255.0f, 163.0f / 255.0f, 0.0f / 255.0f);

    // Mapeo UV
    glm::vec3 uv = glm::vec3(
            atan2(fragment.originalPos.x, fragment.originalPos.z),
            acos(fragment.originalPos.y / sqrt(fragment.originalPos.x * fragment.originalPos.x + fragment.originalPos.y * fragment.originalPos.y + fragment.originalPos.z * fragment.originalPos.z)),
            sqrt(fragment.originalPos.x * fragment.originalPos.x + fragment.originalPos.y * fragment.originalPos.y + fragment.originalPos.z * fragment.originalPos.z)
    );

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    // Ajuste de la escala del ruido para manchas más grandes y menos numerosas
    float scale = 3000.0f; // Escala más baja para manchas más grandes
    float offsetX = 10000.0f;
    float offsetY = 10000.0f;

    // Generar el valor de ruido
    float noiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale, (uv.y + offsetY) * scale);

    // Mezclar los colores basados en el valor de ruido
    float t = glm::smoothstep(-1.0f, 1.0f, noiseValue); // Mapeo [-1, 1] a [0, 1]
    glm::vec3 finalColor = glm::mix(sunColor1, sunColor2, t);

    // Aplicar el color
    fragment.color = Color(finalColor.r, finalColor.g, finalColor.b);

    return fragment;
}

Fragment fragmentShaderEarth5(Fragment& fragment) {
    Color color;

    glm::vec3 groundColor = glm::vec3(0.13f, 0.55f, 0.13f);
    glm::vec3 oceanColor = glm::vec3(0.12f, 0.38f, 0.57f);
    glm::vec3 cloudColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 iceColor = glm::vec3(0.85f, 0.85f, 0.85f);

    float x = fragment.originalPos.x;
    float y = fragment.originalPos.y;
    float z = fragment.originalPos.z;
    float radius = sqrt(x*x + y*y + z*z);

    glm::vec3 uv = glm::vec3(atan2(x, z), acos(y / radius), radius);

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // Simplificando la generación de ruido
    float noiseScale = 80.0f; // Escala aumentada
    float noise = noiseGenerator.GetNoise(uv.x * noiseScale, uv.y * noiseScale, uv.z * noiseScale);


    // Unificando la lógica de mezcla
    float landThreshold = 0.0f; // Umbral para mezclar océano y terreno
    float iceThreshold = 0.8f; // Umbral para hielo polar
    float cloudThreshold = 0.4f; // Umbral para nubes

    glm::vec3 baseColor = (noise < landThreshold) ? oceanColor : groundColor;
    baseColor = mix(baseColor, iceColor, glm::smoothstep(iceThreshold, 1.0f, abs(y / radius)));

    // Mezclar nubes independientemente del tipo de terreno
    float cloudNoise = noiseGenerator.GetNoise(uv.x * noiseScale * 5.0f, uv.y * noiseScale * 5.0f, uv.z * noiseScale * 5.0f)*1.5f;
    if (cloudNoise > cloudThreshold) {
        baseColor = mix(baseColor, cloudColor, glm::smoothstep(cloudThreshold, 0.6f, cloudNoise));
    }

    color = Color(baseColor.x, baseColor.y, baseColor.z);
    fragment.color = color * 0.8;

    return fragment;
}

Fragment fragmentShaderGasGiant(Fragment& fragment) {
    Color color;

    // Define los tonos de verde
    glm::vec3 lightGreen = glm::vec3(0.68f, 1.0f, 0.18f); // Verde claro
    glm::vec3 midGreen = glm::vec3(0.4f, 0.8f, 0.13f); // Verde medio
    glm::vec3 darkGreen = glm::vec3(0.0f, 0.39f, 0.0f); // Verde oscuro

    float x = fragment.originalPos.x;
    float y = fragment.originalPos.y;
    float z = fragment.originalPos.z;

    // Crear una inclinación en las franjas
    float diagonalFactor = 0.02f; // Controla el grado de inclinación
    float inclinedV = acos((y + diagonalFactor * x) / sqrt(x*x + y*y + z*z)) / M_PI;

    // Parámetros para controlar el ancho de la mezcla en las fronteras
    float borderSize = 0.05f; // Tamaño de la frontera de mezcla
    float lowerBorder = 0.33f - borderSize;
    float upperBorder = 0.33f + borderSize;
    float middleBorderLow = 0.66f - borderSize;
    float middleBorderHigh = 0.66f + borderSize;

    // Determinar el color basado en la coordenada V inclinada
    glm::vec3 tmpColor;
    if (inclinedV < lowerBorder) {
        tmpColor = lightGreen;
    } else if (inclinedV < upperBorder) {
        tmpColor = mix(lightGreen, midGreen, glm::smoothstep(lowerBorder, upperBorder, inclinedV));
    } else if (inclinedV < middleBorderLow) {
        tmpColor = midGreen;
    } else if (inclinedV < middleBorderHigh) {
        tmpColor = mix(midGreen, darkGreen, glm::smoothstep(middleBorderLow, middleBorderHigh, inclinedV));
    } else {
        tmpColor = darkGreen;
    }

    color = Color(tmpColor.x, tmpColor.y, tmpColor.z);

    // Aplicar la intensidad de iluminación
    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment fragmentShaderJupiter(Fragment& fragment) {
    Color color;

    // Coordenadas del fragmento
    float x = fragment.originalPos.x;
    float y = fragment.originalPos.y;
    float z = fragment.originalPos.z;
    float radius = sqrt(x*x + y*y + z*z);

    // Convertir coordenadas a UV
    float u = atan2(x, z) / (2.0f * M_PI);
    float v = acos(y / radius) / M_PI;

    // Parámetros para las franjas
    float borderSize = 0.08f; // Tamaño de la frontera de mezcla

    // Determinar el color basado en la coordenada V
    glm::vec3 tmpColor;
    float stripeWidth = 1.0f / 7.0f; // 7 franjas
    int stripeIndex = int(v / stripeWidth);
    float stripePosition = (v - stripeWidth * stripeIndex) / stripeWidth;

    // Colores de las franjas
    glm::vec3 stripeColors[7] = {
            glm::vec3(0.7f, 0.5f, 0.4f), // Tonos de marrón y beige
            glm::vec3(0.9f, 0.7f, 0.6f),
            glm::vec3(0.6f, 0.4f, 0.3f),
            glm::vec3(0.8f, 0.6f, 0.5f),
            glm::vec3(0.5f, 0.3f, 0.2f),
            glm::vec3(0.7f, 0.5f, 0.4f),
            glm::vec3(0.6f, 0.4f, 0.3f)
    };


    glm::vec3 colorBelow = stripeColors[stripeIndex % 7];
    glm::vec3 colorAbove = stripeColors[(stripeIndex + 1) % 7];
    float mixFactor = glm::smoothstep(0.5f - borderSize, 0.5f + borderSize, stripePosition);
    tmpColor = mix(colorBelow, colorAbove, mixFactor);

    // Agregar la Gran Mancha Roja
    glm::vec2 manchaPosition = glm::vec2(0.4f, 0.5f); // Posición de la mancha roja en UV
    float manchaRadius = 0.1f; // Tamaño de la mancha
    float manchaDistance = distance(glm::vec2(u, v), manchaPosition);
    float manchaFactor = 1.0f - glm::smoothstep(0.0f, manchaRadius, manchaDistance);
    glm::vec3 manchaColor = glm::vec3(0.3f, 0.1f, 0.1f); // Color oscuro para la mancha
    tmpColor = mix(tmpColor, manchaColor, manchaFactor);

    color = Color(tmpColor.x, tmpColor.y, tmpColor.z);

    // Aplicar la intensidad de iluminación
    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment fragmentShaderMars(Fragment& fragment) {
    Color color;

    glm::vec3 groundColor = glm::vec3(0.35f, 0.15f, 0.05f); // Marrón claro
    glm::vec3 oceanColor = glm::vec3(0.45f, 0.25f, 0.15f); // Marrón oscuro

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
    float baseNoiseZoom = 150.0f;
    float baseNoise = noiseGenerator.GetNoise(uv.x * baseNoiseZoom, uv.y * baseNoiseZoom, uv.z * baseNoiseZoom);

    // Capa de terreno: detalles del terreno
    float terrainNoiseZoom = 300.0f;
    float terrainNoise = noiseGenerator.GetNoise(uv.x * terrainNoiseZoom + 1000, uv.y * terrainNoiseZoom, uv.z * terrainNoiseZoom);

    // Lógica para mezclar océano y terreno
    glm::vec3 tmpColor = (baseNoise < 0.0f) ? oceanColor : groundColor;
    tmpColor = mix(tmpColor, groundColor, glm::clamp(terrainNoise, 0.0f, 1.0f));

    color = Color(tmpColor.x, tmpColor.y, tmpColor.z);

    // Aplicar la intensidad de iluminación
    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment fragmentShaderUranusRevised(Fragment& fragment) {
    Color color;

    // Colores base y de nubes
    glm::vec3 baseColor = glm::vec3(0.21f, 0.69f, 0.87f); // Azul verdoso
    glm::vec3 cloudColor = glm::vec3(0.85f, 0.85f, 0.92f); // Blanco azulado para nubes

    // Posición original del fragmento
    float x = fragment.originalPos.x;
    float y = fragment.originalPos.y;
    float z = fragment.originalPos.z;

    const float PI = 3.14159265358979323846f;

    // Calcula UV teniendo en cuenta una transición más suave
    glm::vec2 uv;
    uv.x = atan2(x, z) / (2.0f * PI);
    uv.y = acos(y / sqrt(x*x + y*y + z*z)) / PI;

    // Ruido para las nubes
    static FastNoiseLite noiseGenerator; // Estático para mejor rendimiento
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // Escala del ruido para una transición más suave
    float cloudNoiseScale = 0.6f;
    float cloudNoise = noiseGenerator.GetNoise(uv.x * cloudNoiseScale, uv.y * cloudNoiseScale);
    cloudNoise = (cloudNoise + 1.0f) / 2.0f; // Normaliza el valor del ruido

    // Transición suave entre colores
    float cloudFactor = glm::smoothstep(0.3f, 0.7f, cloudNoise);

    // Mezcla colores
    glm::vec3 tmpColor = mix(baseColor, cloudColor, cloudFactor);
    color = Color(tmpColor.x, tmpColor.y, tmpColor.z);

    // Aplica la intensidad de iluminación
    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment fragmentShaderNeptune(Fragment& fragment) {
    Color color;

    // Define el color base para Neptuno
    glm::vec3 baseColor = glm::vec3(0.05f, 0.2f, 0.5f); // Azul oscuro

    // Define el color para las nubes
    glm::vec3 cloudColor = glm::vec3(0.7f, 0.7f, 0.9f); // Azul claro para nubes

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

    // Ruido para las nubes
    float cloudNoiseScale = 0.3f;
    float cloudNoise = noiseGenerator.GetNoise(uv.x * cloudNoiseScale + 1000, uv.y * cloudNoiseScale);

    // Normalizar el valor del ruido para las nubes
    cloudNoise = (cloudNoise + 1.0f) / 2.0f;

    // Crear patrones de nubes a lo largo del ecuador
    float cloudBandWidth = 0.1f; // Ancho de la banda de nubes
    float cloudBandCenter = 0.5f; // Posición del centro de la banda de nubes
    float lowerBandEdge = cloudBandCenter - cloudBandWidth;
    float upperBandEdge = cloudBandCenter + cloudBandWidth;
    float vNormalized = uv.y / M_PI; // Normalizar v a [0, 1]
    float cloudFactor = glm::smoothstep(lowerBandEdge, upperBandEdge, vNormalized);

    // Mezclar colores basados en el ruido de las nubes
    glm::vec3 tmpColor = mix(baseColor, cloudColor, cloudFactor * cloudNoise);

    color = Color(tmpColor.x, tmpColor.y, tmpColor.z);

    // Aplicar la intensidad de iluminación
    fragment.color = color * fragment.intensity;

    return fragment;
}

