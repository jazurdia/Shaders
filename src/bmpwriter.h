#pragma once

#include "gl.h"
#include <fstream>
#include <vector>

// Assumptions: SCREEN_WIDTH, SCREEN_HEIGHT, and zbuffer are defined appropriately.
// zbuffer is a 2D array that holds the Z-buffer values.

void writeBMP(const std::string& filename) {
    int width = SCREEN_WIDTH;
    int height = SCREEN_HEIGHT;

    float zMin = std::numeric_limits<float>::max();
    float zMax = std::numeric_limits<float>::lowest();

    for (const auto& row : zbuffer) {
        for (const auto& val : row) {
            if (val != 99999.0f) { // Ignora los valores que no han sido actualizados
                zMin = std::min(zMin, val);
                zMax = std::max(zMax, val);
            }
        }
    }

// Verifica que zMin y zMax sean diferentes
    if (zMin == zMax) {
        std::cerr << "zMin y zMax son iguales. Esto producirá una imagen en blanco o negro.\n";
        return;
    }

    std::cout << "zMin: " << zMin << ", zMax: " << zMax << "\n";


    // Abre el archivo en modo binario
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "No se pudo abrir el archivo para escribir: " << filename << "\n";
        return;
    }

    // Escribe la cabecera del archivo BMP
    uint32_t fileSize = 54 + 3 * width * height;
    uint32_t dataOffset = 54;
    uint32_t imageSize = 3 * width * height;
    uint32_t biPlanes = 1;
    uint32_t biBitCount = 24;

    uint8_t header[54] = {'B', 'M',
                          static_cast<uint8_t>(fileSize & 0xFF), static_cast<uint8_t>((fileSize >> 8) & 0xFF), static_cast<uint8_t>((fileSize >> 16) & 0xFF), static_cast<uint8_t>((fileSize >> 24) & 0xFF),
                          0, 0, 0, 0,
                          static_cast<uint8_t>(dataOffset & 0xFF), static_cast<uint8_t>((dataOffset >> 8) & 0xFF), static_cast<uint8_t>((dataOffset >> 16) & 0xFF), static_cast<uint8_t>((dataOffset >> 24) & 0xFF),
                          40, 0, 0, 0,
                          static_cast<uint8_t>(width & 0xFF), static_cast<uint8_t>((width >> 8) & 0xFF), static_cast<uint8_t>((width >> 16) & 0xFF), static_cast<uint8_t>((width >> 24) & 0xFF),
                          static_cast<uint8_t>(height & 0xFF), static_cast<uint8_t>((height >> 8) & 0xFF), static_cast<uint8_t>((height >> 16) & 0xFF), static_cast<uint8_t>((height >> 24) & 0xFF),
                          static_cast<uint8_t>(biPlanes & 0xFF), static_cast<uint8_t>((biPlanes >> 8) & 0xFF),
                          static_cast<uint8_t>(biBitCount & 0xFF), static_cast<uint8_t>((biBitCount >> 8) & 0xFF)};

    file.write(reinterpret_cast<char*>(header), sizeof(header));

    // Escribe los datos de los píxeles
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float normalized = (zbuffer[y][x] - zMin) / (zMax - zMin);
            uint8_t color = static_cast<uint8_t>(normalized * 255);
            file.write(reinterpret_cast<char*>(&color), 1);
            file.write(reinterpret_cast<char*>(&color), 1);
            file.write(reinterpret_cast<char*>(&color), 1);
        }
    }

    file.close();
    std::cout << "Archivo BMP guardado: " << filename << "\n";
}



