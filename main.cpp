// TODO:
// Create a 64 x 64 grid
// Be able to add arbitrary shapes to it
// (rectangle and circle)
// Render that as an image using stb_image_lib

#include <vector>
#include <cmath>
#include <stdio.h>
#include <cstdint>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Define the grid size
const int GRID_WIDTH = 512;
const int GRID_HEIGHT = 512;

// Grayscale image buffer (1 byte per pixel)
std::vector<uint8_t> image(GRID_WIDTH * GRID_HEIGHT, 255); // Initialize with white

// Draw a rectangle on the grid
void drawRectangle(int x, int y, int width, int height, uint8_t color) {
    for (int i = y; i < y + height; ++i) {
        for (int j = x; j < x + width; ++j) {
            if (i >= 0 && i < GRID_HEIGHT && j >= 0 && j < GRID_WIDTH) {
                image[i * GRID_WIDTH + j] = color;
            }
        }
    }
}

// Draw a circle on the grid
void drawCircle(int cx, int cy, int radius, uint8_t color) {
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                int px = cx + x;
                int py = cy + y;
                if (px >= 0 && px < GRID_WIDTH && py >= 0 && py < GRID_HEIGHT) {
                    image[py * GRID_WIDTH + px] = color;
                }
            }
        }
    }
}

int main() {
    // Draw some shapes
    drawRectangle(50, 50, 100, 150, 100);  // Rectangle (darker gray)
    drawCircle(300, 300, 50, 50);          // Circle (even darker gray)

    // Save the image as a PNG
    if (stbi_write_png("output.png", GRID_WIDTH, GRID_HEIGHT, 1, image.data(), GRID_WIDTH)) {
        printf("Image saved to output.png\n");
    } else {
        printf("Failed to save image.\n");
    }

    return 0;
}
