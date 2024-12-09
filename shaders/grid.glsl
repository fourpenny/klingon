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


layout(local_size_x = 32, local_size_y = 32) in;

void main() {
    // Compute the flattened index based on the workgroup and local IDs
    uint idx = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;

    // Set the value to 1.0
    grid[idx] = 1.0;
}