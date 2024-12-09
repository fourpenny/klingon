// Useful links...
// Custom barebones game engine written using Vulkan
// https://github.com/eliasdaler/edbr/blob/517f76c1ee6f12ab3fccd3ad3ea6c8039ece233a/edbr/include/edbr/Graphics/Vulkan/Init.h
// https://github.com/eliasdaler/edbr/tree/517f76c1ee6f12ab3fccd3ad3ea6c8039ece233a/edbr/include/edbr/Graphics/Vulkan
// Vulkan-tutorial.com
// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code

#include "glm/glm.hpp"

#include "debug_utils.hpp"
#include "device_utils.hpp"
#include "buffer_utils.hpp"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <fstream>

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

struct Rectangle {
    glm::vec4 rect;
};

struct Circle {
    glm::vec3 circ;
};

struct LightSource {
    glm::vec4 light;
};

struct GridCell {
    int x;
    int y;
    float val;
};

class GridManager {
    public:
        // Grid Info
        std::vector<GridCell> grid;
        int gridWidth = 20;
        int gridHeight = 20;

        GridManager(){
            for (int i = 0; i < gridHeight; i++){
                for (int j = 0; j < gridWidth; j++){
                    GridCell cell{i, j, 0.0};
                    grid.push_back(cell);
                }
            }
        }
};

class VulkanComputeApp {
    public:
        VulkanComputeApp(){
            initVulkan();
        }

        ~VulkanComputeApp() {
            cleanup();
        }
    private:
        // Grid stuff
        GridManager gridManager;

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;

        // Devices
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device;

        // Queues
        VkQueue computeQueue;
        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer;

        // Descriptor sets - define resources provided to shaders
        // See https://docs.vulkan.org/spec/latest/chapters/descriptorsets.html
        // for more details
        int num_bindings = 1; // for grid, lights, circles, rectangles
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorPool descriptorPool;
        VkDescriptorSet descriptorSet;

        // Buffers for shapes
        VkBuffer gridBuffer;
        VkDeviceMemory gridBufferMemory;
        VkDeviceSize gridBufferSize;
        // VkBuffer circleBuffer;
        // VkDeviceSize circleBufferSize;
        // VkBuffer rectBuffer;
        // VkDeviceSize rectBufferSize;
        // VkBuffer lightBuffer; 
        // VkDeviceSize lightBufferSize;

        // VMA
        VmaAllocator allocator;
        VmaAllocation gridAllocation;

        // Compute pipeline
        VkPipelineLayout computePipelineLayout;
        VkPipeline computePipeline;

        void initVulkan() {
            createInstance();
            if (enableValidationLayers){
                vu::setupDebugMessenger(instance, debugMessenger);
            }
            pickPhysicalDevice();
            createLogicalDevice();

            createComputePipeline();
            
            // Create the buffers needed for objects we use in compute pipeline
            createVmaAllocator();
            initializeAppBuffers();
            // createDescriptorPool();
            // createDescriptorSets();
            // Create the command buffers
            // createCommandBuffer();
        }

        static std::vector<char> readFile(const std::string& filename) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error("failed to open file!");
            }

            // Preallocate a buffer for the size of the opened file
            size_t fileSize = (size_t) file.tellg();
            std::vector<char> buffer(fileSize);

            // Now we can read all the bytes at once
            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();

            return buffer;
        }

        void createInstance() {
            if (enableValidationLayers && !vu::checkValidationLayerSupport()){
                throw std::runtime_error("validation layers requested but not available!");
            }
        
            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Grid Intersection App";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "No Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_3;

            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            std::vector<const char*> reqExtensions;
            auto extensions = vu::getRequiredExtensions(enableValidationLayers);
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();

            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
            if (enableValidationLayers) {
                createInfo.enabledLayerCount = static_cast<u_int32_t>(vu::validationLayers.size());
                createInfo.ppEnabledLayerNames = vu::validationLayers.data();

                vu::populateDebugMessengerCreateInfo(debugCreateInfo);
                createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
            } else {
                createInfo.enabledLayerCount = 0;

                createInfo.pNext = nullptr;
            }

            if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
                throw std::runtime_error("failed to create instance!");
            }

        }

        void pickPhysicalDevice() {
            uint32_t deviceCount = 0;
            // If output is a nullptr, get the # of physical devices
            vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

            if (deviceCount == 0) {
                throw std::runtime_error("failed to find any GPUs with Vulkan support!");
            }

            std::vector<VkPhysicalDevice> devices(deviceCount);
            // // This has different behavior when we call it a second time
            // // It now allocates pointers to the physical devices into the vector
            vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

            for (const auto& device : devices) {
                // For now, we pick the first device
                if (vu::isDeviceSuitable(device)) {
                    physicalDevice = device;
                    break;
                }
            }

            if (physicalDevice == VK_NULL_HANDLE) {
                throw std::runtime_error("failed to find a suitable GPU!");
            }
        }

        void createLogicalDevice(){
            vu::QueueFamilyIndices indices = vu::findQueueFamilies(physicalDevice);

            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = {indices.computeFamily.value()};
            
            // Double check whether this is needed or not since we're
            // only using the compute queue
            float queuePriority = 1.0f;
            for (uint32_t queueFamily : uniqueQueueFamilies) {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = indices.computeFamily.value();
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            VkPhysicalDeviceFeatures deviceFeatures{};

            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            createInfo.pQueueCreateInfos = queueCreateInfos.data();

            createInfo.pEnabledFeatures = &deviceFeatures;

            createInfo.enabledExtensionCount = 0; 
            createInfo.ppEnabledExtensionNames = nullptr; 

            if (enableValidationLayers) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(vu::validationLayers.size());
                createInfo.ppEnabledLayerNames = vu::validationLayers.data();
            } else {
                createInfo.enabledLayerCount = 0;
            }

            if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
                throw std::runtime_error("failed to create logical device!");
            }

            vkGetDeviceQueue(device, indices.computeFamily.value(), 0, &computeQueue);
        };

        void createVmaAllocator(){
            VmaAllocatorCreateInfo allocatorCreateInfo{};
            allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
            allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
            allocatorCreateInfo.physicalDevice = physicalDevice;
            allocatorCreateInfo.device = device;
            allocatorCreateInfo.instance = instance;
            allocatorCreateInfo.pVulkanFunctions = nullptr;

            vmaCreateAllocator(&allocatorCreateInfo, &allocator);
        }

        VkShaderModule createShaderModule(const std::vector<char>& code) {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shader module!");
            }

            return shaderModule;
        }

        void createCommandPool(){
            vu::QueueFamilyIndices queueFamilyIndices = vu::findQueueFamilies(physicalDevice);

            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = queueFamilyIndices.computeFamily.value();

            if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create command pool!");
            }
        }

        void createCommandBuffer(){
            VkCommandBufferAllocateInfo allocateInfo = {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocateInfo.commandPool = commandPool;
            allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocateInfo.commandBufferCount = 1;

            if (vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer) != VK_SUCCESS){
                throw std::runtime_error("failed to allocate command buffers!");
            }
        }

        void createComputePipeline(){
            // 1. Create descriptor set layout
            VkDescriptorSetLayoutBinding bufferBinding{};
            bufferBinding.binding = 0; // Matches `layout(binding = 0)` in the shader
            bufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bufferBinding.descriptorCount = 1;
            bufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            bufferBinding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = 1;
            layoutInfo.pBindings = &bufferBinding;

            if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout!");
            }
            
            // 2. Define pipeline layout
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

            if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create compute pipeline layout!");
            }

            // 3. Create compute shader modules and associate it with the right
            // stage in the pipeline (the only one)
            auto computeShaderCode = readFile("shaders/grid.spv");

            VkShaderModule computeShaderModule = createShaderModule(computeShaderCode);

            VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
            computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            computeShaderStageInfo.module = computeShaderModule;
            computeShaderStageInfo.pName = "main";

            // 4. Create the actual pipeline :)
            VkComputePipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipelineInfo.layout = computePipelineLayout;
            pipelineInfo.stage = computeShaderStageInfo;

            if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create compute pipeline!");
            }

            // 5. Now that we've added the shader to the pipeline we can release it
            // from memory
            vkDestroyShaderModule(device, computeShaderModule, nullptr);
        }

        void createDescriptorSetLayout(){
            VkDescriptorSetLayoutBinding bindings[num_bindings];

            for (int i = 0; i < num_bindings; ++i){
                bindings[i].binding = i;
                bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                bindings[i].descriptorCount = 1;
                bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                bindings[i].pImmutableSamplers = nullptr;
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = num_bindings;
            layoutInfo.pBindings = bindings;

            if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create descriptor set layout!");
            }
        }

        void createDescriptorPool(){
            VkDescriptorPoolSize poolSizes[num_bindings];

            for (int i = 0; i < num_bindings; ++i){
                poolSizes[i].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[i].descriptorCount = 1; 
            }

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = num_bindings;
            poolInfo.pPoolSizes = poolSizes;
            poolInfo.maxSets = 1; // One descriptor set

            if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create descriptor pool!");
            }
        }

        // void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        //     VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
            
        //     if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        //         throw std::runtime_error("failed to create buffer!");
        //     }

        //     VkMemoryRequirements memRequirements;
        //     vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
            
        //     VkMemoryAllocateInfo allocInfo{};
        //     allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        //     allocInfo.allocationSize = memRequirements.size;
        //     allocInfo.memoryTypeIndex = vu::findMemoryType(
        //         memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        //         physicalDevice);

        //     if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        //         throw std::runtime_error("failed to allocate buffer memory!");
        //     }
        
        //     vkBindBufferMemory(device, buffer, bufferMemory, 0);
        // }

        void createDescriptorSets(){
            std::cout << "we're updating the descriptor sets" << std::endl;

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &descriptorSetLayout; 

            if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate descriptor sets!");
            }
            
            VkDescriptorBufferInfo gridBufferInfo{};
            gridBufferInfo.buffer = gridBuffer;
            gridBufferInfo.offset = 0;
            gridBufferInfo.range = sizeof(float);

            std::cout << "howdy?" << std::endl;
            VkWriteDescriptorSet gridDescriptorWrite;
            gridDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            gridDescriptorWrite.dstSet = descriptorSet;
            gridDescriptorWrite.dstBinding = 0;
            gridDescriptorWrite.dstArrayElement = 0;
            gridDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            gridDescriptorWrite.descriptorCount = 1;
            gridDescriptorWrite.pBufferInfo = &gridBufferInfo;
            gridDescriptorWrite.pImageInfo = nullptr;
            gridDescriptorWrite.pTexelBufferView = nullptr;
            
            vkUpdateDescriptorSets(device, 1, &gridDescriptorWrite, 0, nullptr);
        }

        void initializeAppBuffers(){
            VkDeviceSize gridBufferSize = gridManager.gridHeight * gridManager.gridWidth * sizeof(float);
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = gridBufferSize; 
            bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; 
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

            vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &gridBuffer, &gridAllocation, nullptr);
            
            // VkBuffer gridStagingBuffer;
            // VkDeviceMemory gsBufferMemory;
            // createBuffer(gridBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            //     gridStagingBuffer, gsBufferMemory);
            
            // void* gridData;
            // vkMapMemory(device, gsBufferMemory, 0, gridBufferSize, 0, &gridData);
            // std::vector<float> cellValues;
            // for (auto cell : gridManager.grid) {
            //     cellValues.push_back(cell.val);
            // }
            // memcpy(gridData, cellValues.data(), (size_t) gridBufferSize);
            // vkUnmapMemory(device, gsBufferMemory);

            // createBuffer(gridBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            //     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gridBuffer, gridBufferMemory);

            // vu::copyBuffer(device, commandPool, gridStagingBuffer, gridBuffer, gridBufferSize, computeQueue);

            // vkDestroyBuffer(device, gridStagingBuffer, nullptr);
            // vkFreeMemory(device, gsBufferMemory, nullptr);
        }

        void cleanup(){
            // Do all the stuff to clean up Vulkan here
            // vkDestroyBuffer(device, gridBuffer, nullptr);
            // vkFreeMemory(device, gridBufferMemory, nullptr);
            
            // vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            // vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

            // vkDestroyCommandPool(device, commandPool, nullptr);
            vmaDestroyBuffer(allocator, gridBuffer, gridAllocation);
            
            vmaDestroyAllocator(allocator);

            vkDestroyDevice(device, nullptr);

            if (enableValidationLayers) {
                vu::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            }

            vkDestroyInstance(instance, nullptr);
        };

};

int main() {
    try {
        VulkanComputeApp app;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "hello" << std::endl;

    return EXIT_SUCCESS;
}