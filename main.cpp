// Useful links...
// Custom barebones game engine written using Vulkan
// https://github.com/eliasdaler/edbr/blob/517f76c1ee6f12ab3fccd3ad3ea6c8039ece233a/edbr/include/edbr/Graphics/Vulkan/Init.h
// https://github.com/eliasdaler/edbr/tree/517f76c1ee6f12ab3fccd3ad3ea6c8039ece233a/edbr/include/edbr/Graphics/Vulkan
// Vulkan-tutorial.com
// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code

#include <vulkan/vulkan.h>

#include "debug_utils.hpp"
#include "device_utils.hpp"


#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

class GridManager {
    GridManager() {};
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
        int num_bindings = 4; // for grid, lights, circles, rectangles
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorPool descriptorPool;
        VkDescriptorSet descriptorSet;

        // Buffers - one for each binding
        VkBuffer buffers[4];

        void initVulkan() {
            createInstance();
            // std::cout << "created an instance!" << std::endl;
            if (enableValidationLayers){
                vu::setupDebugMessenger(instance, debugMessenger);
            }
            pickPhysicalDevice();
            createLogicalDevice();
            // Create the buffers needed for objects we use in compute pipeline
            // (descriptor sets)
            createDescriptorSetLayout();
            createDescriptorPool();
            createDescriptorSets();
            // Create the command buffers
            createCommandPool();
            createCommandBuffer();
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
                createInfo.enabledLayerCount = static_cast<u_int32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();

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
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();
            } else {
                createInfo.enabledLayerCount = 0;
            }

            if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
                throw std::runtime_error("failed to create logical device!");
            }

            vkGetDeviceQueue(device, indices.computeFamily.value(), 0, &computeQueue);
        };

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

        void createDescriptorSets(){
            VkDescriptorBufferInfo bufferInfos[num_bindings];
            for (int i = 0; i < num_bindings; ++i){
                bufferInfos[i].buffer = buffers[i];  
                bufferInfos[i].offset = 0;
                bufferInfos[i].range = VK_WHOLE_SIZE;
            }

            VkWriteDescriptorSet descriptorWrites[num_bindings];
            for (int i = 0; i < num_bindings; ++i){
                descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[i].dstSet = descriptorSet;
                descriptorWrites[i].dstBinding = i;
                descriptorWrites[i].dstArrayElement = 0;
                descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                descriptorWrites[i].descriptorCount = 1;
                descriptorWrites[i].pBufferInfo = &bufferInfos[i];
            }
            vkUpdateDescriptorSets(device, 4, descriptorWrites, 0, nullptr);
        }

        void cleanup(){
            // Do all the stuff to clean up Vulkan here
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

            vkDestroyCommandPool(device, commandPool, nullptr);

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