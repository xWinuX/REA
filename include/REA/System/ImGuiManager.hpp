#pragma once

#include <SplitEngine/Application.hpp>
#include <SplitEngine/ECS/ContextProvider.hpp>
#include <SplitEngine/ECS/SystemBase.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>

#include <vulkan/vulkan.hpp>


using namespace SplitEngine;

namespace REA::System
{
	class ImGuiManager : public ECS::SystemBase
	{
		public:
			ImGuiManager(ECS::ContextProvider& contextProvider);

		protected:
			void Destroy(ECS::ContextProvider& contextProvider) override;

			void RunExecute(ECS::ContextProvider& contextProvider, uint8_t stage) override;

			void StartNewImGuiFrame();

		private:
			bool               _firstRender = true;
			vk::DescriptorPool _imGuiDescriptorPool{};
	};
}
