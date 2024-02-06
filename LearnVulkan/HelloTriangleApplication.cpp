#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <set>
#include <vector>
#include <fstream>

#include "startup.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

/**
 * \brief 创建DebugUtilsMessengerEXT对象
 * \param instance
 * \param pCreateInfo
 * \param pAllocator
 * \param pCallback
 * \return
 */
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

/**
 * \brief 销毁DebugUtilsMessengerEXT对象
 * \param instance
 * \param callback
 * \param pAllocator
 */
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, callback, pAllocator);
	}
}

/**
 * \brief 队列族的索引
 */
struct QueueFamilyIndices
{
	// 支持绘制指令的队列族索引
	int m_graphicsFamily = -1;

	// 支持表现的队列族索引
	int m_presentFamily = -1;

	bool isComplete()
	{
		return m_graphicsFamily >= 0 && m_presentFamily >= 0;
	}
};

/**
 * \brief 交换链的支持细节
 */
struct SwapChainSupportDetails
{
	// 基础表面特性
	VkSurfaceCapabilitiesKHR m_capabilities;

	// 表面格式
	std::vector<VkSurfaceFormatKHR> m_formats;

	// 可用的呈现模式
	std::vector<VkPresentModeKHR> m_presentModes;
};

#ifdef HelloTriangle
class HelloTriangleApplication
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	// 窗口句柄
	GLFWwindow* m_window;

	// Vulkan实例
	VkInstance m_instance;

	// Debug回调
	VkDebugUtilsMessengerEXT m_callback;

	// 物理设备
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

	// 逻辑设备
	VkDevice m_device;

	// 图形绘制队列句柄
	VkQueue m_graphicsQueue;

	// 窗口表面
	VkSurfaceKHR m_surface;

	// 表现队列句柄
	VkQueue m_presentQueue;

	// 交换链句柄
	VkSwapchainKHR m_swapChain;

	// 交换链图像
	std::vector<VkImage> m_swapChainImages;

	// 交换链图像格式
	VkFormat m_swapChainImageFormat;

	// 交换链范围
	VkExtent2D m_swapChainExtent;

	// 图像视图
	std::vector<VkImageView> m_swapChainImageViews;

	/**
	 * \brief 初始化窗口
	 */
	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	}

	/**
	 * \brief 初始化Vulkan
	 */
	void initVulkan()
	{
		createInstance();
		setupDebugCallback();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createGraphicsPipeline();
	}

	/**
	 * \brief 主循环
	 */
	void mainLoop()
	{
		while (!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
		}
	}

	/**
	 * \brief 清理工作
	 */
	void cleanup()
	{
		for(auto imageView : m_swapChainImageViews)
		{
			vkDestroyImageView(m_device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);

		vkDestroyDevice(m_device, nullptr);

		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(m_instance, m_callback, nullptr);
		}

		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyInstance(m_instance, nullptr);

		glfwDestroyWindow(m_window);

		glfwTerminate();
	}

	/**
	 * \brief 创建Vulkan实例
	 */
	void createInstance()
	{
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// 检测扩展支持
		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();

		// 在校验层启用时使用校验层
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance!");
		}
	}

	/**
	 * \brief 检查校验层
	 * \return
	 */
	bool checkValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		// 获取所有可用的校验层列表
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		// 检查是否存在列表中指定的校验层
		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

	/**
	 * \brief 根据是否启用校验层，返回所需的扩展列表
	 * \return
	 */
	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;

		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	/**
	 * \brief 设置Debug回调
	 */
	void setupDebugCallback()
	{
		if (!enableValidationLayers)
		{
			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;	// Optional

		if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_callback) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set up debug callback!");
		}
	}

	/**
	 * \brief 创建窗口表面
	 */
	void createSurface()
	{
		if(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");
		}
	}

	/**
	 * \brief 选择一个物理设备（显卡）
	 */
	void pickPhysicalDevice()
	{
		// 请求显卡的数量
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

		if(deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		// 分配数组来存储VkPhysicalDevice对象
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

		// 选择第一个满足需求的设备
		for(const auto& device : devices)
		{
			if(isDeviceSuitable(device))
			{
				m_physicalDevice = device;
				break;
			}
		}

		if(m_physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	/**
	 * \brief 创建逻辑设备
	 */
	void createLogicalDevice()
	{
		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> uniqueQueueFamilies = { indices.m_graphicsFamily, indices.m_presentFamily };

		float queuePriority = 1.0f;
		for(int queueFamily : uniqueQueueFamilies)
		{
			// 指定要创建的队列
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);

		}

		// 指定使用的设备特性
		VkPhysicalDeviceFeatures deviceFeatures = {};

		// 创建逻辑设备
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if(enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device!");
		}

		// 获取队列句柄
		vkGetDeviceQueue(m_device, indices.m_graphicsFamily, 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, indices.m_presentFamily, 0, &m_presentQueue);
	}

	/**
	 * \brief 创建交换链
	 */
	void createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.m_formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.m_presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.m_capabilities);

		// 设置交换链队列可以容纳的图像个数
		uint32_t imageCount = swapChainSupport.m_capabilities.minImageCount + 1;
		// maxImageCount的值为0表明，只要内存满足，可以使用任意数量的图像
		if(swapChainSupport.m_capabilities.maxImageCount > 0 &&  imageCount > swapChainSupport.m_capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.m_capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
		uint32_t queueFamilyIndices[] = { (uint32_t)indices.m_graphicsFamily, (uint32_t)indices.m_presentFamily };

		if(indices.m_graphicsFamily != indices.m_presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;	// Optional
			createInfo.pQueueFamilyIndices = nullptr;	// Optional
		}

		createInfo.preTransform = swapChainSupport.m_capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
		m_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

		m_swapChainImageFormat = surfaceFormat.format;
		m_swapChainExtent = extent;

	}

	/**
	 * \brief 创建图像视图
	 */
	void createImageViews()
	{
		// 分配空间类存储图像视图
		m_swapChainImageViews.resize(m_swapChainImages.size());

		// 遍历所有交换链图像，创建图像视图
		for(size_t i = 0; i < m_swapChainImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_swapChainImages[i];

			// 指定图像数据的解释方式
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_swapChainImageFormat;

			// 指定图像颜色通道的映射
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			// 指定图像的用途和图像的哪一部分可以被访问
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			// 创建图像视图
			if(vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	/**
	 * \brief 创建图形管线
	 */
	void createGraphicsPipeline()
	{
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");
		
		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;	// Optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;	// Optional

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_swapChainExtent.width;
		viewport.height = (float)m_swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;

		vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
		vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
	}

	/**
	 * \brief 设备是否满足需求
	 * \param device 
	 * \return 
	 */
	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		// 这里简单处理，只需交换链支持一种图像格式和一种支持我们的窗口表面的呈现模式即可
		bool swapChainAdequate = false;
		if(extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	/**
	 * \brief 检查设备扩展是否被支持
	 * \param device 
	 * \return 
	 */
	bool checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		// 构建当前可用的设备扩展列表
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		// 将集合中的扩展剔除，如果集合中的元素为0，说明所需的扩展全部都被满足
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for(const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	/**
	 * \brief 寻找满足条件的队列族
	 * \param device 
	 * \return 
	 */
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		// 获取设备的队列族个数
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		// 分配数组存储队列族的VkQueueFamilyProperties对象
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for(const auto& queueFamily : queueFamilies)
		{
			// 确定支持绘制指令的队列族索引
			if(queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.m_graphicsFamily = i;
			}


			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

			// 确定支持表现的队列族索引
			if(queueFamily.queueCount > 0 && presentSupport)
			{
				indices.m_presentFamily = i;
			}

			if(indices.isComplete())
			{
				break;
			}

			i++;
		}

		return indices;
	}

	/**
	 * \brief 查询交换链是否支持
	 * \param device 
	 * \return 
	 */
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;

		// 查询基础表面特性
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.m_capabilities);

		// 查询表面支持的格式
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

		if(formatCount != 0)
		{
			details.m_formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.m_formats.data());
		}

		// 查询支持的呈现格式
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

		if(presentModeCount != 0)
		{
			details.m_presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.m_presentModes.data());
		}

		return details;
	}

	/**
	 * \brief 选择合适的表面格式
	 * \param availableFormats 
	 * \return 
	 */
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
	{
		// 如果表面没有自己的首选格式，则返回设定的格式
		if(availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		// 如果是一个格式列表，则检查我们想要的设定的格式是否存在于列表中
		for(const auto& availableFormat : availableFormats)
		{
			if(availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}
	}

	/**
	 * \brief 选择最佳的可用呈现模式
	 * \param availablePresentModes 
	 * \return 
	 */
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
	{
		// 设置一种通用模式
		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

		// 优先使用三倍缓冲模式
		for(const auto& availablePresentMode : availablePresentModes)
		{
			if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
			else if(availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				bestMode = availablePresentMode;
			}
		}

		return bestMode;
	}

	/**
	 * \brief 选择交换范围
	 * \param capabilities 
	 * \return 
	 */
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			VkExtent2D actualExtent = { WIDTH, HEIGHT };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	/**
	 * \brief 创建VkShaderModule对象
	 * \param code 
	 * \return 
	 */
	VkShaderModule createShaderModule(const std::vector<char>& code)
	{
		// 指定VkShaderModuleCreateInfo存储字节码的数组和数组长度
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		// 创建VkShaderModule对象
		VkShaderModule shaderModule;
		if(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}

	/**
	 * \brief 用于载入二进制文件的辅助函数
	 * \param filename 
	 * \return 
	 */
	static std::vector<char> readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if(!file.is_open())
		{
			throw std::runtime_error("failed to open file!");
		}

		// 根据读取位置确定文件大小，分配足够的数组空间来容纳数据
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		// 读取整个文件
		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	/**
	 * \brief 接受调试信息的回调函数
	 * \param messageSeverity 消息的级别
	 * \param messageType 消息的类型
	 * \param pCallbackData
	 * \param pUserData
	 * \return
	 */
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
};

int main()
{
	HelloTriangleApplication app;

	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
#else
#endif