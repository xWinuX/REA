#include "REA/System/PhysicsDebugRenderer.hpp"

#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>

namespace REA::System
{
	PhysicsDebugRenderer::PhysicsDebugRenderer(AssetHandle<Rendering::Material>             material,
	                                           ECS::Registry::SystemHandle<System::Physics> physicsHandle,
	                                           ECS::ContextProvider&                        contextProvider):
		_material(material),
		_physicsHandle(physicsHandle)
	{
		Rendering::Vulkan::Device& device = contextProvider.GetContext<RenderingContext>()->Renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice();

		vk::DeviceSize bufferSizes[]        = { sizeof(glm::vec2) * MAX_VERTICES, sizeof(uint16_t) * MAX_VERTICES };
		vk::DeviceSize bufferElementSizes[] = { sizeof(glm::vec2), sizeof(uint16_t) };
		//vk::DeviceSize bufferSizes[] = {sizeof(glm::vec2) * MAX_VERTICES, sizeof(uint16_t) * MAX_VERTICES};

		_modelBuffer = Rendering::Vulkan::Buffer(&device,
		                                         vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
		                                         vk::SharingMode::eExclusive,
		                                         {
			                                         Rendering::Vulkan::Allocator::Auto,
			                                         vk::Flags<
				                                         Rendering::Vulkan::Allocator::MemoryAllocationCreateFlagBits>(Rendering::Vulkan::Allocator::WriteSequentially |
				                                                                                                       Rendering::Vulkan::Allocator::PersistentMap)
		                                         },
		                                         2,
		                                         Rendering::Vulkan::Buffer::EMPTY_DATA,
		                                         bufferSizes,
		                                         bufferSizes,
		                                         bufferElementSizes);

		_vertices = _modelBuffer.GetMappedData<glm::vec2>();
		_indices  = reinterpret_cast<uint16_t*>(_vertices + _modelBuffer.GetDataElementNum(0));

		AppendFlags(e_shapeBit);
	}

	void PhysicsDebugRenderer::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {}

	void PhysicsDebugRenderer::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
	{
		for (int i = 0; i < vertexCount; ++i)
		{
			b2Vec2 vertex = vertices[i];

			_vertices[i + _numVertices] = { vertex.x, vertex.y };
			_indices[i + _numIndices]   = i + _numVertices;
		}

		_numIndices += vertexCount;
		_numVertices += vertexCount;

		_indices[_numIndices++] = std::numeric_limits<uint16_t>::max();
	}

	void PhysicsDebugRenderer::DrawCircle(const b2Vec2& center, float radius, const b2Color& color) {}

	void PhysicsDebugRenderer::DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) {}

	void PhysicsDebugRenderer::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) { }

	void PhysicsDebugRenderer::DrawTransform(const b2Transform& xf) {}

	void PhysicsDebugRenderer::DrawPoint(const b2Vec2& p, float size, const b2Color& color) {}

	void PhysicsDebugRenderer::Destroy(ECS::ContextProvider& contextProvider) { _modelBuffer.Destroy(); }

	void PhysicsDebugRenderer::RunExecute(ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		if (stage == Stage::Physics)
		{
			_numIndices  = 0;
			_numVertices = 0;
		}

		if (stage == Stage::Rendering)
		{
			if (!_registeredToWorld)
			{
				_physicsHandle.System->RegisterDebugDraw(this);
				_registeredToWorld = true;
			}
			else
			{
				vk::CommandBuffer commandBuffer = contextProvider.GetContext<RenderingContext>()->Renderer->GetCommandBuffer().GetVkCommandBuffer();

				_material->GetShader()->BindGlobal(commandBuffer);

				_material->GetShader()->Update();

				_material->GetShader()->Bind(commandBuffer);

				_material->Update();

				_material->Bind(commandBuffer);

				size_t offset = 0;
				commandBuffer.bindVertexBuffers(0, _modelBuffer.GetVkBuffer(), offset);
				commandBuffer.bindIndexBuffer(_modelBuffer.GetVkBuffer(), _modelBuffer.GetSizeInBytes(0), vk::IndexType::eUint16);
				commandBuffer.drawIndexed(_numIndices, 1, 0, 0, 0);
			}
		}
	}
}
