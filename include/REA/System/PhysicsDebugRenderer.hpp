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
			PhysicsDebugRenderer(AssetHandle<Rendering::Material> material, ECS::Registry::SystemHandle<System::Physics> physicsHandle, ECS::ContextProvider& contextProvider);

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
			const uint64_t MAX_VERTICES = 10'000;

			size_t _numVertices = 0;
			size_t _numIndices = 0;

			glm::vec2* _vertices;
			uint16_t*  _indices;

			AssetHandle<Rendering::Material>             _material;
			ECS::Registry::SystemHandle<System::Physics> _physicsHandle;

			Rendering::Vulkan::Buffer _modelBuffer;


			bool _registeredToWorld = false;
	};
}
