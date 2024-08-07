#include "REA/System/PhysicsDebugRenderer.hpp"

#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>

namespace REA::System
{
	PhysicsDebugRenderer::PhysicsDebugRenderer(AssetHandle<Rendering::Material>             wireFrameMaterial,
	                                           AssetHandle<Rendering::Material>             lineMaterial,
	                                           ECS::Registry::SystemHandle<System::Physics> physicsHandle,
	                                           ECS::ContextProvider&                        contextProvider):
		_wireFrameMaterial(wireFrameMaterial),
		_lineMaterial(lineMaterial),
		_physicsHandle(physicsHandle)
	{
		Rendering::Vulkan::Device& device = contextProvider.GetContext<RenderingContext>()->Renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice();

		vk::DeviceSize bufferSizes[] = { sizeof(glm::vec2) * MAX_VERTICES, sizeof(uint16_t) * MAX_VERTICES, sizeof(glm::vec2) * MAX_VERTICES};
		vk::DeviceSize bufferElementSizes[] = { sizeof(glm::vec2), sizeof(uint16_t), sizeof(glm::vec2)};

		_modelBuffer = Rendering::Vulkan::Buffer(&device,
		                                         vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
		                                         vk::SharingMode::eExclusive,
		                                         {
			                                         Rendering::Vulkan::Allocator::Auto,
			                                         vk::Flags<
				                                         Rendering::Vulkan::Allocator::MemoryAllocationCreateFlagBits>(Rendering::Vulkan::Allocator::WriteSequentially |
				                                                                                                       Rendering::Vulkan::Allocator::PersistentMap)
		                                         },
		                                         3,
		                                         Rendering::Vulkan::Buffer::EMPTY_DATA,
		                                         bufferSizes,
		                                         bufferSizes,
		                                         bufferElementSizes);

		_wireFrameVertices = _modelBuffer.GetMappedData<glm::vec2>();
		_wireFrameIndices  = reinterpret_cast<uint16_t*>(_wireFrameVertices + _modelBuffer.GetDataElementNum(0));
		_lineVertices      = reinterpret_cast<glm::vec2*>(_wireFrameIndices + _modelBuffer.GetBufferElementNum(1));

		AppendFlags(e_shapeBit);
	}

	void PhysicsDebugRenderer::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {}

	void PhysicsDebugRenderer::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
	{
		for (int i = 0; i < vertexCount; ++i)
		{
			b2Vec2 vertex = vertices[i];

			_wireFrameVertices[i + _numWireFrameVertices] = { vertex.x, vertex.y };
			_wireFrameIndices[i + _numWireFrameIndices]   = i + _numWireFrameVertices;
		}

		_numWireFrameIndices += vertexCount;
		_numWireFrameVertices += vertexCount;

		_wireFrameIndices[_numWireFrameIndices++] = std::numeric_limits<uint16_t>::max();
	}

	void PhysicsDebugRenderer::DrawCircle(const b2Vec2& center, float radius, const b2Color& color) {}

	void PhysicsDebugRenderer::DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) {}

	void PhysicsDebugRenderer::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
	{
		_lineVertices[_numLineVertices]     = { p1.x, p1.y };
		_lineVertices[_numLineVertices + 1] = { p2.x, p2.y };
		_numLineVertices += 2;
	}

	void PhysicsDebugRenderer::DrawTransform(const b2Transform& xf) {}

	void PhysicsDebugRenderer::DrawPoint(const b2Vec2& p, float size, const b2Color& color) {}

	void PhysicsDebugRenderer::Destroy(ECS::ContextProvider& contextProvider) { _modelBuffer.Destroy(); }

	void PhysicsDebugRenderer::RunExecute(ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		if (stage == Stage::Physics)
		{
			_numWireFrameIndices  = 0;
			_numWireFrameVertices = 0;
			_numLineVertices      = 0;
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

				_wireFrameMaterial->GetShader()->BindGlobal(commandBuffer);

				_wireFrameMaterial->GetShader()->Update();

				_wireFrameMaterial->GetShader()->Bind(commandBuffer);

				_wireFrameMaterial->Update();

				_wireFrameMaterial->Bind(commandBuffer);

				size_t offset = 0;
				commandBuffer.bindVertexBuffers(0, _modelBuffer.GetVkBuffer(), offset);
				commandBuffer.bindIndexBuffer(_modelBuffer.GetVkBuffer(), _modelBuffer.GetSizeInBytes(0), vk::IndexType::eUint16);
				commandBuffer.drawIndexed(_numWireFrameIndices, 1, 0, 0, 0);

				_lineMaterial->GetShader()->BindGlobal(commandBuffer);

				_lineMaterial->GetShader()->Update();

				_lineMaterial->GetShader()->Bind(commandBuffer);

				_lineMaterial->Update();

				_lineMaterial->Bind(commandBuffer);

				offset = _modelBuffer.GetSizeInBytes(0) + _modelBuffer.GetSizeInBytes(1);
				commandBuffer.bindVertexBuffers(0, _modelBuffer.GetVkBuffer(), offset);
				commandBuffer.draw(_numLineVertices, 1, 0, 0);
			}
		}
	}
}
