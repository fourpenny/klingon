#version 450

layout(binding = 0) buffer GridBuffer {
    float grid[]; // Flattened 2D grid
};

// layout(binding = 1) buffer LightSources {
//     vec4 lights[]; // Each vec4: (x, y, intensity, attenuation)
// };

// layout(binding = 2) buffer Circles {
//     vec3 circles[]; // (cx, cy, r)
// };

// layout(binding = 3) buffer Rectangles {
//     vec4 rectangles[]; // (cx, cy, w, h)
// };

// TODO: Take every grid cell in the buffer (which starts at 0) and set it to 1