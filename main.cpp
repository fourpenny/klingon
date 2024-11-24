#include <vulkan/vulkan.h>

#include "vulkan_utils.h"

// Use the standard validation layer SDK
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
		instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
									VkDebugUtilsMessengerEXT debugMessenger,
									const VkAllocationCallbacks* pAllocator) {
	auto func  = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

class VulkanComputeApp {
    public:
        void init(){
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

        void initVulkan() {
            createInstance();
            if (enableValidationLayers){
                vu::setupDebugMessenger(instance, debugMessenger);
            }
            pickPhysicalDevice();
            createLogicalDevice();
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

            auto extensions = getRequiredExtensions();
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

            // std::vector<VkPhysicalDevice> devices(deviceCount);
            // // This has different behavior when we call it a second time
            // // It now allocates pointers to the physical devices into the vector
            // vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

            // for (const auto& device : devices) {
            //     // For now, we pick the first device
            //     if (isDeviceSuitable(device)) {
            //         physicalDevice = device;
            //         break;
            //     }
            // }

            // if (physicalDevice == VK_NULL_HANDLE) {
            //     throw std::runtime_error("failed to find a suitable GPU!");
            // }
        }

        // bool isDeviceSuitable(VkPhysicalDevice device) {
        //     QueueFamilyIndices indices = findQueueFamilies(device);

        //     bool extensionsSupported = checkDeviceExtensionSupport(device);

        //     bool swapChainAdequate = false;
        //     if (extensionsSupported) {
        //         SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        //         swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        //     }

        //     return indices.isComplete() && extensionsSupported && swapChainAdequate;
        // }

        void createLogicalDevice(){};

        void cleanup(){
            // Do all the stuff to clean up Vulkan here
            vkDestroyDevice(device, nullptr);

            if (enableValidationLayers) {
                DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            }
        };

}
