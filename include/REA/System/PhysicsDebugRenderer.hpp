#pragma once

#include <vector>
#include <box2d/b2_draw.h>
#include <glm/vec2.hpp>
#include <SplitEngine/AssetDatabase.hpp>
#include <SplitEngine/ECS/ContextProvider.hpp>
#include <SplitEngine/ECS/SystemBase.hpp>
#include <SplitEngine/Rendering/Material.hpp>

#include "Physics.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class PhysicsDebugRenderer : public ECS::SystemBase, public b2Draw
	{
		public:
			PhysicsDebugRenderer(AssetHandle<Rendering::Material>             wireFrameMaterial,
			                     AssetHandle<Rendering::Material>             lineMaterial,
			                     ECS::Registry::SystemHandle<System::Physics> physicsHandle,
			                     ECS::ContextProvider&                        contextProvider);

			void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;

			void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;

			void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) override;

			void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) override;
			void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override;
			void DrawTransform(const b2Transform& xf) override;
			void DrawPoint(const b2Vec2& p, float size, const b2Color& color) override;

		protected:
			void Destroy(ECS::ContextProvider& contextProvider) override;

			void RunExecute(ECS::ContextProvider& context, uint8_t stage) override;

		private:
			const uint64_t MAX_VERTICES = 100'000;

			size_t _numWireFrameVertices = 0;
			size_t _numWireFrameIndices  = 0;

			glm::vec2* _wireFrameVertices;
			uint16_t*  _wireFrameIndices;

			size_t     _numLineVertices = 0;
			glm::vec2* _lineVertices;

			AssetHandle<Rendering::Material>             _wireFrameMaterial;
			AssetHandle<Rendering::Material>             _lineMaterial;
			ECS::Registry::SystemHandle<System::Physics> _physicsHandle;

			Rendering::Vulkan::Buffer _modelBuffer;

			bool _registeredToWorld = false;
	};
}
