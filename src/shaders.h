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

    glm::vec3 groundColor = glm::vec3(0.13f, 0.55f, 0.13f);
    glm::vec3 oceanColor = glm::vec3(0.12f, 0.38f, 0.57f);
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









