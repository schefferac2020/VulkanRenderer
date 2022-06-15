#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#if defined(DEBUG)
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif

struct QueueFamilyIndicies {
    std::optional<uint32_t> graphicsFamily;


    bool isComplete() {
        return graphicsFamily.has_value();
    }
};

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void createInstance() {
        //Check validation layers
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested but not available!!!");
        }

        VkApplicationInfo appInfo{}; // Optional but can be useful
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{}; //Tells driver which global extensions and val layers to use
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        //need extentions for glfw windows
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        createInfo.enabledLayerCount = 0;

        //add validation layers if enabled
        if (enableValidationLayers) {
            createInfo.enabledExtensionCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledExtensionCount = 0;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create the instance!");
        }

    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        //Can determine if the device is suitable based on the properties and features HERE (eg. dedicated graphics card)


        //Make sure the physical device supports a queeue family that can handle graphics
        QueueFamilyIndicies indicies = findQueueFamilies(device);
        return indicies.isComplete();
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        //Check if all the names in validationLayers exist in the availableLayers list
        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }


    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("No GPUs with VULKAN support was found!");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndicies indicies = findQueueFamilies(physicalDevice);

        //Specifies the queues that need to be created
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indicies.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;

        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        //Specifies the deviceFeatures like (eg. geometry shaders, etc)
        VkPhysicalDeviceFeatures deviceFeatures{};

        //All information about logical device
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;

        //Device specific extensions and validation layers (ONLY USED IN OLDER VERSIONS OF VULKAN)
        createInfo.enabledExtensionCount = 0;

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        //Instantiate the logical device with these structs
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create a logical device!");
        }

        vkGetDeviceQueue(device, indicies.graphicsFamily.value(), 0, &graphicsQueue);
    }

    //Look for groups of queues that support graphics commands
    QueueFamilyIndicies findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndicies indicies;

        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        //Need at least one queue family that supports VK_QUEUE_GRAPHICS
        uint32_t i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indicies.graphicsFamily = i;
            }

            //Early exit if all queue families are found
            if (indicies.isComplete()) {
                break;
            }
            ++i;
        }

        return indicies;
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroyInstance(instance, nullptr);
        vkDestroyDevice(device, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    VkQueue graphicsQueue;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    GLFWwindow* window;
    VkInstance instance;
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}