#include <optional>
#include <vector>
#include <set>
#include <string>

#include <vulkan/vulkan.h>

#ifndef KLINGON__DEVICE_UTILS_HPP
#define KLINGON__DEVICE_UTILS_HPP

namespace vu {
    // Holds the locations of each queue family on the device,
    // for now we just want the compute queue but using this structure
    // in case we want to expand this later.
    struct QueueFamilyIndices {
        std::optional<uint32_t> computeFamily;

        bool isComplete() {
            return computeFamily.has_value();
        }
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indicies;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                indicies.computeFamily = i;
                break;
            }

            i++;
        }

        return indicies;
    }
    
    // Check if our physical device
    // 1. Can support the correct type of queue structures
    // 2. Can support the selected Vulkan extensions
    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indicies = findQueueFamilies(device);

        return indicies.isComplete();
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties,
        VkPhysicalDevice physicalDevice) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && 
					(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

    std::vector<const char*> getRequiredExtensions(bool enableValidationLayers) {
        std::vector<const char*> extensions;
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }
    
    bool checkDeviceExtensionSupport(VkPhysicalDevice device, std::vector<const char*> deviceExtensions) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension: availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
    }
} // namespace vu

#endif // KLINGON__DEVICE_UTILS_HPP