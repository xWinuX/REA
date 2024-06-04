#include "REA/System/ImGuiManager.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Systems.hpp>

#include "IconsFontAwesome.h"
#include "REA/Color.hpp"
#include "REA/Context/ImGui.hpp"

namespace REA::System
{
	ImGuiManager::ImGuiManager(ECS::ContextProvider& contextProvider)
	{
		vk::DescriptorPoolSize poolSizes[] = {
			{ vk::DescriptorType::eSampler, 1000 },
			{ vk::DescriptorType::eCombinedImageSampler, 1000 },
			{ vk::DescriptorType::eSampledImage, 1000 },
			{ vk::DescriptorType::eStorageImage, 1000 },
			{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
			{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
			{ vk::DescriptorType::eUniformBuffer, 1000 },
			{ vk::DescriptorType::eStorageBuffer, 1000 },
			{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
			{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
			{ vk::DescriptorType::eInputAttachment, 1000 }
		};

		const vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo = vk::DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, poolSizes);

		Rendering::Renderer*               renderer       = contextProvider.GetContext<RenderingContext>()->Renderer;
		Rendering::Vulkan::PhysicalDevice& physicalDevice = renderer->GetVulkanInstance().GetPhysicalDevice();
		Rendering::Vulkan::Device&         device         = physicalDevice.GetDevice();
		SDL_Window*                        window         = contextProvider.GetContext<EngineContext>()->Application->GetWindow().GetSDLWindow();

		_imGuiDescriptorPool = device.GetVkDevice().createDescriptorPool(descriptorPoolCreateInfo);

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontDefault();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.IniFilename = nullptr;

		ImFontConfig config;
		config.MergeMode = true;
		config.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
		static const ImWchar iconRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		io.Fonts->AddFontFromFileTTF("res/fonts/fa-solid-900.ttf", 13.0f, &config, iconRanges);

		ImGuiStyle& imGuiStyle = ImGui::GetStyle();

		imGuiStyle.FrameRounding = 5.0f;
		imGuiStyle.ChildRounding = 5.0f;
		imGuiStyle.GrabRounding = 5.0f;



		Context::ImGui* imGuiContext = contextProvider.GetContext<Context::ImGui>();

		Color dangerColor = Color(0xFF4A48FF);
		imGuiContext->ColorDanger = static_cast<ImVec4>(dangerColor);
		imGuiContext->ColorDangerHover = static_cast<ImVec4>(dangerColor * 1.5f);
		imGuiContext->ColorDangerClicked = static_cast<ImVec4>(dangerColor * 2.0f);

		ImGui_ImplSDL2_InitForVulkan(window);

		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance                  = renderer->GetVulkanInstance().GetVkInstance();
		initInfo.PhysicalDevice            = physicalDevice.GetVkPhysicalDevice();
		initInfo.Device                    = device.GetVkDevice();
		initInfo.Queue                     = device.GetQueueFamily(Rendering::Vulkan::QueueType::Graphics).GetVkQueue();
		initInfo.DescriptorPool            = _imGuiDescriptorPool;
		initInfo.MinImageCount             = 3;
		initInfo.ImageCount                = 3;
		initInfo.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&initInfo, device.GetRenderPass().GetVkRenderPass());

		ImGui_ImplVulkan_CreateFontsTexture();

		ImGui_ImplVulkan_DestroyFontsTexture();

		StartNewImGuiFrame();

		// Add event call back
		contextProvider.GetContext<EngineContext>()->EventSystem->OnPollEvent.Add([](SDL_Event& event) { ImGui_ImplSDL2_ProcessEvent(&event); });
	}

	void ImGuiManager::Destroy(ECS::ContextProvider& contextProvider)
	{
		ImGui_ImplVulkan_Shutdown();

		contextProvider.GetContext<RenderingContext>()->Renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetVkDevice().destroy(_imGuiDescriptorPool);
	}

	void ImGuiManager::RunExecute(ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		vk::CommandBuffer& commandBuffer = contextProvider.GetContext<RenderingContext>()->Renderer->GetCommandBuffer().GetVkCommandBuffer();
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

		StartNewImGuiFrame();
		ImGuiID dockSpaceID = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
		if (_firstRender)
		{
			Context::ImGui* imGuiContext = contextProvider.GetContext<Context::ImGui>();

			ImGui::DockBuilderSplitNode(dockSpaceID, ImGuiDir_Left, 0.2f, &imGuiContext->LeftDockingID, &imGuiContext->CenterDockingID);
			ImGui::DockBuilderSplitNode(imGuiContext->CenterDockingID, ImGuiDir_Right, 0.4f, &imGuiContext->RightDockingID, &imGuiContext->CenterDockingID);
			ImGui::DockBuilderSplitNode(imGuiContext->CenterDockingID, ImGuiDir_Up, 0.15f, &imGuiContext->TopDockingID, &imGuiContext->CenterDockingID);
			ImGui::DockBuilderSplitNode(imGuiContext->TopDockingID, ImGuiDir_Left, 0.3f, &imGuiContext->TopLeftDockingID, &imGuiContext->TopDockingID);
			
			ImGui::DockBuilderFinish(dockSpaceID);

			_firstRender = false;
		}
	}

	void ImGuiManager::StartNewImGuiFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();

		ImGui::NewFrame();
	}
}
