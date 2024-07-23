#include "REA/System/PixelGridSimulation.hpp"

#include <CDT.h>
#include <execution>
#include <IconsFontAwesome.h>
#include <imgui.h>
#include <utility>
#include <vector>
#include <box2d/b2_edge_shape.h>
#include <box2d/b2_polygon_shape.h>
#include <glm/gtc/random.hpp>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Debug/Performance.hpp>
#include <SplitEngine/Rendering/Material.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>
#include <SplitEngine/Rendering/Vulkan/Device.hpp>

#define FASTNOISE_STATIC_LIB
#include <FastNoise/FastNoise.h>

#include "REA/Assets.hpp"
#include "REA/MarchingSquareMesherUtils.hpp"
#include "REA/PixelType.hpp"
#include "REA/Stage.hpp"
#include "REA/Component/PixelGridRigidBody.hpp"
#include "REA/Component/Transform.hpp"
#include "REA/Context/ImGui.hpp"


namespace REA::System
{
	PixelGridSimulation::PixelGridSimulation(const SimulationShaders& simulationShaders, ECS::ContextProvider& contextProvider):
		_shaders(simulationShaders)
	{
		EngineContext* engineContext = contextProvider.GetContext<EngineContext>();
		ECS::Registry& ecsRegistry   = engineContext->Application->GetECSRegistry();

		auto&                      properties = _shaders.FallingSimulation->GetProperties();
		Rendering::Vulkan::Device* device     = _shaders.FallingSimulation->GetPipeline().GetDevice();

		vk::DeviceSize bufferSizes[]        = { sizeof(glm::vec2) * 100'000, sizeof(uint16_t) * 100'000 };
		vk::DeviceSize bufferElementSizes[] = { sizeof(glm::vec2), sizeof(uint16_t) };

		_indexes = std::ranges::iota_view(static_cast<size_t>(0), 200'000ull);

		//vk::DeviceSize bufferSizes[] = {sizeof(glm::vec2) * MAX_VERTICES, sizeof(uint16_t) * MAX_VERTICES};

		_vertexBuffer = Rendering::Vulkan::Buffer(device,
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

		_commandBuffer = std::move(device->GetQueueFamily(Rendering::Vulkan::QueueType::Compute).AllocateCommandBuffer(Rendering::Vulkan::QueueType::Compute));

		_commandBuffer.GetVkCommandBufferRaw().SetFramePtr(&_fif);

		vk::FenceCreateInfo fenceCreateInfo = vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
		_computeFence                       = _shaders.FallingSimulation->GetPipeline().GetDevice()->GetVkDevice().createFence(fenceCreateInfo);

		Component::Collider collider;
		collider.InitialType     = b2_staticBody;
		collider.PhysicsMaterial = engineContext->Application->GetAssetDatabase().GetAsset<PhysicsMaterial>(Asset::PhysicsMaterial::Defaut);

		_staticEnvironmentEntityID = ecsRegistry.CreateEntity<Component::Transform, Component::Collider>({}, std::move(collider));
	}

	void PixelGridSimulation::SwapPixelBuffer() { std::swap(_readIndex, _writeIndex); }

	void PixelGridSimulation::CmdWaitForPreviousComputeShader()
	{
		vk::MemoryBarrier memoryBarrier[] = {
			vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderWrite),
			vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead),
			vk::MemoryBarrier(vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eShaderWrite),
			vk::MemoryBarrier(vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eShaderRead),
		};

		_commandBuffer.GetVkCommandBuffer().pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
		                                                    vk::PipelineStageFlagBits::eComputeShader,
		                                                    {},
		                                                    4,
		                                                    memoryBarrier,
		                                                    0,
		                                                    nullptr,
		                                                    0,
		                                                    nullptr);
	}

	void PixelGridSimulation::ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		if (stage == Stage::GridComputeEnd)
		{
			Context::ImGui* imGuiContext = contextProvider.GetContext<Context::ImGui>();
			ImGui::SetNextWindowDockID(imGuiContext->TopDockingID, ImGuiCond_Always);
			ImGui::Begin("Simulation");

			glm::vec2 avail  = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };
			glm::vec2 offset = (avail * 0.5f) - 25.0f;

			if (_paused) { offset.x -= 35; }

			glm::vec2 localPos = { ImGui::GetCursorPos().x, ImGui::GetCursorPos().y };
			glm::vec2 pos      = localPos + offset;
			ImGui::SetCursorPos({ pos.x, pos.y });

			if (ImGui::Button(_paused ? ICON_FA_PLAY : ICON_FA_PAUSE, { 50, 50 })) { _paused = !_paused; }
			else if (_paused)
			{
				ImGui::SetCursorPos({ pos.x + 60, pos.y });
				if (ImGui::Button(ICON_FA_FORWARD_STEP, { 50, 50 })) { _doStep = true; }
			}

			ImGui::End();

			ImGui::Begin("World Gen");
			if (ImGui::Button("Regenerate World")) { _generateWorld = true; }
			ImGui::SliderFloat("Cave Noise Treshold", &_worldGenerationSettings.CaveNoiseTreshold, 0.0f, 1.0f);
			ImGui::SliderFloat("Cave Noise Freqency", &_worldGenerationSettings.CaveNoiseFrequency, 0.0f, 0.1f);
			ImGui::SliderFloat("Overworld Noise Freqency", &_worldGenerationSettings.OverworldNoiseFrequency, 0.0f, 0.1f);
			ImGui::End();
		}

		System<Component::PixelGrid, Component::Collider>::ExecuteArchetypes(archetypes, contextProvider, stage);
	}

	glm::uvec2 PixelGridSimulation::GetMargolusOffset(uint32_t frame)
	{
		switch (frame % 4)
		{
			case 0:
				return { 0, 0 };
			case 1:
				return { 1, 0 };
			case 2:
				return { 0, 1 };
			case 3:
				return { 1, 1 };
			default:
				return { 0, 0 };
		}
	}

	void PixelGridSimulation::Execute(Component::PixelGrid*  pixelGrids,
	                                  Component::Collider*   colliders,
	                                  std::vector<uint64_t>& entities,
	                                  ECS::ContextProvider&  contextProvider,
	                                  uint8_t                stage)
	{
		EngineContext*       engineContext = contextProvider.GetContext<EngineContext>();
		ECS::Registry&       ecsRegistry   = engineContext->Application->GetECSRegistry();
		Rendering::Renderer* renderer      = contextProvider.GetContext<RenderingContext>()->Renderer;

		Rendering::Vulkan::Device&     device       = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice();
		Rendering::Vulkan::QueueFamily computeQueue = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetQueueFamily(Rendering::Vulkan::QueueType::Compute);

		for (int entityIndex = 0; entityIndex < entities.size(); ++entityIndex)
		{
			SSBO_SimulationData* simulationData = _shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_SimulationData>(0);
			SSBO_RigidBodyData*  rigidBodyData  = _shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_RigidBodyData>(3);

			Component::PixelGrid& pixelGrid = pixelGrids[entityIndex];

			if (stage == Stage::GridComputeEnd)
			{
				device.GetVkDevice().waitForFences(_computeFence, vk::True, UINT64_MAX);
				device.GetVkDevice().resetFences(_computeFence);

				pixelGrid.PixelState = &_shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_Pixels>(2)->Pixels[0];
			}

			if (stage == Stage::GridComputeHalted)
			{
				SSBO_MarchingCubes* marchingCubes = _shaders.MarchingSquareAlgorithm->GetProperties().GetBufferData<SSBO_MarchingCubes>(5);

				/*size_t     vi = 0;
				glm::vec2* v  = _vertexBuffer.GetMappedData<glm::vec2>();
				for (vi = 0; vi< marchingCubes->numSolidSegments; ++vi)
				{
					v[vi * 2]     = { marchingCubes->solidSegments[vi * 2].x, marchingCubes->solidSegments[vi * 2].y };
					v[vi * 2 + 1] = { marchingCubes->solidSegments[vi * 2 + 1].x, marchingCubes->solidSegments[vi * 2 + 1].y };
				}

				if (marchingCubes->numSolidSegments > 0)
				{
					_numLineSegements = vi;
				}*/

				// Pixelgrid follows camera
				if (ecsRegistry.IsEntityValid(pixelGrid.CameraEntityID))
				{
					glm::vec2 offset = { pixelGrid.SimulationWidth, pixelGrid.SimulationHeight };

					glm::vec2 targetPosition = ecsRegistry.GetComponent<Component::Transform>(pixelGrid.CameraEntityID).Position * 10.0f;
					targetPosition -= offset / 2.0f;

					targetPosition = glm::clamp(targetPosition, { 0.0f, 0.0f }, glm::vec2(pixelGrid.Width, pixelGrid.Height) - offset);

					pixelGrid.ViewTargetPosition = targetPosition;
				}

				// Delete Rigidbodies
				if (simulationData->timer % 4 == 0)
				{
					for (uint32_t deleteRigidbody: _deleteRigidbody)
					{
						RigidbodyEntry& rigidbodyEntry = _rigidBodyEntities[deleteRigidbody];

						Component::PixelGridRigidBody& pixelGridRigidBody = ecsRegistry.GetComponent<Component::PixelGridRigidBody>(rigidbodyEntry.EntityID);

						rigidBodyData->rigidBodies[pixelGridRigidBody.ShaderRigidBodyID].NeedsRecalculation = false;

						_rigidBodyDataHeap.Deallocate(pixelGridRigidBody.AllocationID);

						ecsRegistry.DestroyEntity(rigidbodyEntry.EntityID);

						rigidbodyEntry.Enabled = false;

						_availableRigidBodyIDs.Push(deleteRigidbody);
					}

					_deleteRigidbody.clear();
				}

				// Crate Rigidbodies
				if (simulationData->timer % 4 == 2)
				{
					// Create edges for "static" environment
					glm::vec2 xOffset  = { pixelGrid.SimulationWidth, 0.0f };
					glm::vec2 yOffset  = { 0.0f, pixelGrid.SimulationHeight };
					glm::vec2 xyOffset = { pixelGrid.SimulationWidth, pixelGrid.SimulationHeight };

					marchingCubes->solidSegments[marchingCubes->numSolidSegments * 2]     = pixelGrid.ViewTargetPosition;
					marchingCubes->solidSegments[marchingCubes->numSolidSegments * 2 + 1] = pixelGrid.ViewTargetPosition + yOffset;

					marchingCubes->solidSegments[marchingCubes->numSolidSegments * 2 + 2] = pixelGrid.ViewTargetPosition + yOffset;
					marchingCubes->solidSegments[marchingCubes->numSolidSegments * 2 + 3] = pixelGrid.ViewTargetPosition + xyOffset;

					marchingCubes->solidSegments[marchingCubes->numSolidSegments * 2 + 4] = pixelGrid.ViewTargetPosition + xyOffset;
					marchingCubes->solidSegments[marchingCubes->numSolidSegments * 2 + 5] = pixelGrid.ViewTargetPosition + xOffset;

					marchingCubes->solidSegments[marchingCubes->numSolidSegments * 2 + 6] = pixelGrid.ViewTargetPosition + xOffset;
					marchingCubes->solidSegments[marchingCubes->numSolidSegments * 2 + 7] = pixelGrid.ViewTargetPosition;
					marchingCubes->numSolidSegments += 4;

					std::vector<CDT::Edge> solidEdges = std::vector<CDT::Edge>(marchingCubes->numSolidSegments, CDT::Edge(0, 0));
					for (int i = 0; i < marchingCubes->numSolidSegments; ++i) { solidEdges[i] = CDT::Edge(i * 2, (i * 2) + 1); }

					std::span<CDT::V2d<float>> solidSegmentSpan = std::span(reinterpret_cast<CDT::V2d<float>*>(&marchingCubes->solidSegments[0]),
					                                                        marchingCubes->numSolidSegments * 2);

					MarchingSquareMesherUtils::RemoveDuplicatesAndRemapEdges(solidSegmentSpan, solidEdges);

					std::vector<MarchingSquareMesherUtils::Polyline> solidPolylines = MarchingSquareMesherUtils::SeperateAndSortPolylines(solidSegmentSpan, solidEdges);

					for (MarchingSquareMesherUtils::Polyline& solidPolyline: solidPolylines)
					{
						solidPolyline = MarchingSquareMesherUtils::SimplifyPolylines(solidPolyline, _lineSimplificationTolerance);
					}

					if (solidPolylines.size() > 0) { _numLineSegements = 0; }

					Component::Collider& collider = ecsRegistry.GetComponent<Component::Collider>(_staticEnvironmentEntityID);
					if (collider.Body != nullptr)
					{
						for (b2Fixture* fixture: collider.Fixtures) { collider.Body->DestroyFixture(fixture); }

						collider.Fixtures.clear();

						b2FixtureDef fixtureDef = collider.PhysicsMaterial->GetFixtureDefCopy();
						for (MarchingSquareMesherUtils::Polyline solidPolyline: solidPolylines)
						{
							for (int i = 0; i < solidPolyline.Vertices.size(); ++i)
							{
								// Draw
								glm::vec2* v = _vertexBuffer.GetMappedData<glm::vec2>();

								v[_numLineSegements * 2 + i * 2]     = { solidPolyline.Vertices[i].x, solidPolyline.Vertices[i].y };
								v[_numLineSegements * 2 + i * 2 + 1] = {
									solidPolyline.Vertices[(i + 1) % solidPolyline.Vertices.size()].x,
									solidPolyline.Vertices[(i + 1) % solidPolyline.Vertices.size()].y
								};


								CDT::V2d<float>& v1 = solidPolyline.Vertices[i];
								CDT::V2d<float>& v2 = solidPolyline.Vertices[(i + 1) % solidPolyline.Vertices.size()];

								b2EdgeShape edgeShape{};
								edgeShape.SetTwoSided({ v1.x, v1.y }, { v2.x, v2.y });

								fixtureDef.shape = &edgeShape;

								collider.Fixtures.push_back(collider.Body->CreateFixture(&fixtureDef));
							}

							_numLineSegements += (solidPolyline.Vertices.size());
						}
					}

					// Create rigidbodies and their colission meshes
					std::vector<CDT::Edge> connectedEdges = std::vector<CDT::Edge>(marchingCubes->numConnectedSegments, CDT::Edge(0, 0));
					for (int i = 0; i < marchingCubes->numConnectedSegments; ++i) { connectedEdges[i] = CDT::Edge(i * 2, (i * 2) + 1); }

					std::span<CDT::V2d<float>> connectedSegmentSpan = std::span(reinterpret_cast<CDT::V2d<float>*>(&marchingCubes->connectedSegments[0]),
					                                                            marchingCubes->numConnectedSegments * 2);

					MarchingSquareMesherUtils::RemoveDuplicatesAndRemapEdges(connectedSegmentSpan, connectedEdges);

					std::vector<MarchingSquareMesherUtils::Polyline> connectedPolylines = MarchingSquareMesherUtils::SeperateAndSortPolylines(connectedSegmentSpan, connectedEdges);

					std::vector<size_t>                              polylinesToMove{};
					std::vector<MarchingSquareMesherUtils::Polyline> polylines{};

					// TODO: find a way to not do this
					// Filter out holes
					for (int currentPolyLineIndex = 0; currentPolyLineIndex < connectedPolylines.size(); ++currentPolyLineIndex)
					{
						MarchingSquareMesherUtils::Polyline& currentPolyline = connectedPolylines[currentPolyLineIndex];

						bool isHole = false;
						for (int polyLineIndex = 0; polyLineIndex < connectedPolylines.size(); ++polyLineIndex)
						{
							if (currentPolyLineIndex == polyLineIndex) { continue; }

							MarchingSquareMesherUtils::Polyline& polyline = connectedPolylines[polyLineIndex];

							if (polyline.AABB.Contains(currentPolyline.AABB))
							{
								isHole = true;
								break;
							}
						}

						if (!isHole) { polylinesToMove.push_back(currentPolyLineIndex); }
					}

					for (size_t toMove: polylinesToMove) { polylines.push_back(std::move(connectedPolylines[toMove])); }

					for (MarchingSquareMesherUtils::Polyline& polyline: polylines)
					{
						// TODO: Find reliable way to get seed point
						// Convert to integer grid coordinates
						glm::ivec2 seedPoint = glm::ivec2(glm::floor(polyline.Vertices[0].x), glm::floor(polyline.Vertices[0].y));

						// Simplify line
						polyline = MarchingSquareMesherUtils::SimplifyPolylines(polyline, _lineSimplificationTolerance);

						// If a polyline is somehow and edge skip it
						if (polyline.Vertices.size() < 3) { continue; }

						b2AABB b2Aabb = polyline.AABB;

						// Calculate size needed
						b2Vec2     extends  = b2Aabb.GetExtents();
						glm::uvec2 aabbSize = { glm::round(extends.x * 2 * 10.0f), glm::round(extends.y * 2 * 10.0f)  };
						size_t     dataSize = aabbSize.x * aabbSize.y;

						_cclRange = b2AABB({ glm::min(_cclRange.lowerBound.x, b2Aabb.lowerBound.x), glm::min(_cclRange.lowerBound.y, b2Aabb.lowerBound.y) },
						                   { glm::max(_cclRange.upperBound.x, b2Aabb.upperBound.x), glm::max(_cclRange.upperBound.y, b2Aabb.upperBound.y) });

						Component::Transform          transform{};
						Component::PixelGridRigidBody pixelGridRigidBody{};
						Component::Collider           collider{};

						// Setup Transform
						b2Vec2 center      = b2Aabb.GetCenter();
						transform.Position = { center.x, center.y, 0.0f };

						// Setup PixelGridRigidBody
						uint32_t shaderID = -1u;
						if (_availableRigidBodyIDs.IsEmpty())
						{
							shaderID = _rigidBodyIDCounter++;
							_rigidBodyEntities.resize(_rigidBodyIDCounter);
						}
						else { shaderID = _availableRigidBodyIDs.Pop(); }

						pixelGridRigidBody.ShaderRigidBodyID = shaderID;
						pixelGridRigidBody.AllocationID      = _rigidBodyDataHeap.Allocate(dataSize);
						pixelGridRigidBody.DataIndex         = _rigidBodyDataHeap.GetAllocationInfo(pixelGridRigidBody.AllocationID).Offset;

						pixelGridRigidBody.Size = aabbSize;

						// Write RigidbodyInfo to shader
						rigidBodyData->rigidBodies[shaderID].ID        = shaderID;
						rigidBodyData->rigidBodies[shaderID].DataIndex = pixelGridRigidBody.DataIndex;
						rigidBodyData->rigidBodies[shaderID].Size      = pixelGridRigidBody.Size;

						// Setup Collider
						collider.PhysicsMaterial = engineContext->Application->GetAssetDatabase().GetAsset<PhysicsMaterial>(Asset::PhysicsMaterial::Defaut);
						collider.InitialType     = b2_dynamicBody;

						// Triangulate line
						CDT::Triangulation<float> cdt = MarchingSquareMesherUtils::GenerateTriangulation(polyline);

						b2Vec2     b2Center           = polyline.AABB.GetCenter();
						b2Vec2     b2BottomLeftCorner = b2Center - extends;
						glm::uvec2 bottomLeftCorner   = { b2BottomLeftCorner.x * 10.0f, b2BottomLeftCorner.y * 10.0f };

						// Create collider
						if (!cdt.vertices.empty())
						{
							CDT::V2d<float> vertices[3]{};

							for (CDT::Triangle triangle: cdt.triangles)
							{
								vertices[0] = cdt.vertices[triangle.vertices[2]];
								vertices[1] = cdt.vertices[triangle.vertices[1]];
								vertices[2] = cdt.vertices[triangle.vertices[0]];

								vertices[0] = { vertices[0].x - b2Center.x, vertices[0].y - b2Center.y };
								vertices[1] = { vertices[1].x - b2Center.x, vertices[1].y - b2Center.y };
								vertices[2] = { vertices[2].x - b2Center.x, vertices[2].y - b2Center.y };

								// Skip weird ass small polygons cdt sometime generates
								float area = 0.5f * std::abs(vertices[0].x * (vertices[1].y - vertices[2].y) + vertices[1].x * (vertices[2].y - vertices[0].y) + vertices[2].x * (
									                             vertices[0].y - vertices[1].y));

								if (area < 0.01f) { continue; }

								b2PolygonShape polygonShape;
								polygonShape.Set(reinterpret_cast<const b2Vec2*>(&vertices[0]), 3);

								collider.InitialShapes.push_back(polygonShape);
							}
						}

						_newRigidBodies.push_back({ bottomLeftCorner, aabbSize, seedPoint, shaderID });

						// Create entity
						uint64_t rigidBodyEntity = ecsRegistry.CreateEntity<Component::Transform, Component::PixelGridRigidBody, Component::Collider>(std::move(transform),
							std::move(pixelGridRigidBody),
							std::move(collider));

						_rigidBodyEntities[shaderID] = { rigidBodyEntity, true, false };
					}

					marchingCubes->numConnectedSegments = 0;
					marchingCubes->numSolidSegments     = 0;
				}
			}

			if (stage == Stage::GridComputeBegin)
			{
				size_t numPixels     = pixelGrid.SimulationWidth * pixelGrid.SimulationHeight;
				size_t numWorkgroups = CeilDivide(numPixels, 64ull);

				simulationData->width            = pixelGrid.Width;
				simulationData->height           = pixelGrid.Height;
				simulationData->simulationWidth  = pixelGrid.SimulationWidth;
				simulationData->simulationHeight = pixelGrid.SimulationHeight;
				simulationData->targetPosition   = pixelGrid.ViewTargetPosition;

				if (_firstUpdate)
				{
					memcpy(simulationData->pixelLookup, pixelGrid.PixelDataLookup.data(), pixelGrid.PixelDataLookup.size() * sizeof(Pixel::Data));

					_firstUpdate = false;
				}

				if (_generateWorld)
				{
					BENCHMARK_BEGIN
						std::vector<Pixel::State> world = std::vector<Pixel::State>(pixelGrid.Width * pixelGrid.Height, pixelGrid.PixelLookup[PixelType::Air].PixelState);

						auto caveNoiseGenerator =
								FastNoise::NewFromEncodedNodeTree("IgAAAAA/j8J1Pg0AAwAAAB+FE0EQAM3MDEATAJqZGT8LAAAAAAAAAAAAAQAAAAQAAAAA9iicPwAfhes+AArXoz0ArkeNQQ==");
						std::vector<float> caveNoise = std::vector<float>(world.size());
						caveNoiseGenerator->GenUniformGrid2D(caveNoise.data(), 0, 0, pixelGrid.Width, pixelGrid.Height, _worldGenerationSettings.CaveNoiseFrequency, 1234);

						auto overworldNoiseGenerator =
								FastNoise::NewFromEncodedNodeTree("IQATAClcDz4UACkAAAAAgD8AAACAPwAAAAAAAAAAAAAQAOxR+D8ZAA0AAwAAAD0KF0ApAAAAAAA/AHsUrj4AzczMPQCPwnU+AAAAAD8=");
						std::vector<float> overworldNoise = std::vector<float>(world.size());
						overworldNoiseGenerator->GenUniformGrid2D(overworldNoise.data(),
						                                          0,
						                                          0,
						                                          pixelGrid.Width,
						                                          pixelGrid.Height,
						                                          _worldGenerationSettings.OverworldNoiseFrequency,
						                                          1234);

						const float blendRegionHeight = 30.0f; // Height of the blending region
						const float caveLayerHeight   = pixelGrid.Height / 2;

						const float blendStart = caveLayerHeight;
						const float blendEnd   = caveLayerHeight + blendRegionHeight;

						const float waterLevelStart = caveLayerHeight + 100.0f;
						const float waterLevelEnd = caveLayerHeight + 150.0f;


						for (int i = 0; i < world.size(); ++i)
						{
							size_t x = i % pixelGrid.Width;
							size_t y = i / pixelGrid.Width;

							if (y < caveLayerHeight)
							{
								if (caveNoise[i] < _worldGenerationSettings.CaveNoiseTreshold) { world[i] = pixelGrid.PixelLookup[PixelType::Stone].PixelState; }
							}
							else
							{
								float noise         = glm::clamp(glm::abs(overworldNoise[x]) + 0.1f, 0.0f, 1.0f);
								float terrainHeight = (noise * (caveLayerHeight / 2.0f)) + caveLayerHeight;

								// Calculate blend factor
								float blendFactor = (y - blendStart) / (blendEnd - blendStart);
								blendFactor       = std::clamp(blendFactor, 0.0f, 1.0f);

								if (terrainHeight > y && caveNoise[i] < glm::mix(_worldGenerationSettings.CaveNoiseTreshold, 0.80f, blendFactor))
								{
									if (y >= blendStart && y <= blendEnd)
									{
										float blendNoise = glm::abs(overworldNoise[x + (pixelGrid.Width * 100)]);
										world[i]         = blendStart + (blendNoise * blendRegionHeight) > y
											                   ? pixelGrid.PixelLookup[PixelType::Stone].PixelState
											                   : pixelGrid.PixelLookup[PixelType::Dirt].PixelState;
									}
									else { world[i] = pixelGrid.PixelLookup[PixelType::Dirt].PixelState; }
								}
							}
						}

						memcpy(pixelGrid.PixelState, world.data(), world.size() * sizeof(Pixel::State));

						_generateWorld = false;
					BENCHMARK_END("World Generation")
				}

				if (!_paused || _doStep)
				{
					simulationData->deltaTime = contextProvider.GetContext<TimeContext>()->DeltaTime;
					simulationData->timer     = simulationData->timer + 1;
					simulationData->rng       = glm::linearRand(0.0f, 1.0f);

					_commandBuffer.GetVkCommandBuffer().reset({});

					vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo({}, nullptr);

					_commandBuffer.GetVkCommandBuffer().begin(commandBufferBeginInfo);

					// Fall and Flow
					uint32_t   flowIteration         = 0;
					uint32_t   timer                 = simulationData->timer;
					glm::uvec2 margolusOffset        = GetMargolusOffset(timer);
					size_t     numWorkgroupsMorgulus = CeilDivide(CeilDivide(numPixels, 4ull), 64ull);

					_shaders.FallingSimulation->Update();

					for (int i = 0; i < 7; ++i)
					{
						_shaders.FallingSimulation->Bind(_commandBuffer.GetVkCommandBuffer());

						_shaders.FallingSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &_readIndex);
						_shaders.FallingSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &_writeIndex);
						_shaders.FallingSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 2, &flowIteration);
						_shaders.FallingSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 3, &margolusOffset);

						_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroupsMorgulus, 1, 1);

						SwapPixelBuffer();

						margolusOffset = GetMargolusOffset(timer + i);
						flowIteration++;

						CmdWaitForPreviousComputeShader();
					}

					// Temperature and Charge
					_shaders.AccumulateSimulation->Update();

					_shaders.AccumulateSimulation->Bind(_commandBuffer.GetVkCommandBuffer());

					_shaders.AccumulateSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &_readIndex);
					_shaders.AccumulateSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &_writeIndex);

					_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroups, 1, 1);

					SwapPixelBuffer();

					// Do CCL if there are rigidbodies to discover
					if (!_newRigidBodies.empty())
					{
						b2Vec2 extends            = _cclRange.GetExtents();
						b2Vec2 b2BottomLeftCorner = _cclRange.GetCenter() - extends;

						glm::uvec2 offset = { 0, 0 };
						glm::uvec2 size   = { 1024, 1024 };

						uint32_t numWorkGroupsForSinglePixelOps = CeilDivide(size.x * size.y, 64u);

						CmdWaitForPreviousComputeShader();

						_shaders.CCLInitialize->Update();

						_shaders.CCLInitialize->Bind(_commandBuffer.GetVkCommandBuffer());

						_shaders.CCLInitialize->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &_readIndex);
						_shaders.CCLInitialize->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &offset);
						_shaders.CCLInitialize->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 2, &size);

						_commandBuffer.GetVkCommandBuffer().dispatch(numWorkGroupsForSinglePixelOps, 1, 1);

						CmdWaitForPreviousComputeShader();

						_shaders.CCLColumn->Update();

						_shaders.CCLColumn->Bind(_commandBuffer.GetVkCommandBuffer());

						_shaders.CCLColumn->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &offset);
						_shaders.CCLColumn->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &size);

						_commandBuffer.GetVkCommandBuffer().dispatch(size.x, 1, 1);

						// Merge
						uint32_t stepIndex = 0;
						uint32_t n         = size.x >> 1;
						while (n != 0)
						{
							CmdWaitForPreviousComputeShader();

							_shaders.CCLMerge->Update();

							_shaders.CCLMerge->Bind(_commandBuffer.GetVkCommandBuffer());

							_shaders.CCLMerge->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &offset);
							_shaders.CCLMerge->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &size);
							_shaders.CCLMerge->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 2, &stepIndex);

							_commandBuffer.GetVkCommandBuffer().dispatch(n, 1, 1);

							n = n >> 1;
							stepIndex++;
						}

						CmdWaitForPreviousComputeShader();

						// Relabel
						_shaders.CCLRelabel->Update();

						_shaders.CCLRelabel->Bind(_commandBuffer.GetVkCommandBuffer());

						_shaders.CCLRelabel->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &offset);
						_shaders.CCLRelabel->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &size);

						_commandBuffer.GetVkCommandBuffer().dispatch(size.x * size.y, 1, 1);

						CmdWaitForPreviousComputeShader();

						// Extract
						_shaders.CCLExtract->Update();

						_shaders.CCLExtract->Bind(_commandBuffer.GetVkCommandBuffer());

						for (NewRigidBody newRigidBody: _newRigidBodies)
						{
							_shaders.CCLExtract->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &_readIndex);
							_shaders.CCLExtract->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &newRigidBody.Offset);
							_shaders.CCLExtract->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 2, &newRigidBody.Size);
							_shaders.CCLExtract->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 3, &newRigidBody.SeedPoint);
							_shaders.CCLExtract->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 4, &newRigidBody.RigidBodyID);
							_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(newRigidBody.Size.x * newRigidBody.Size.y, 64u), 1, 1);
						}

						_newRigidBodies.clear();
						_cclRange = { { 10'000'000, 10'000'000 }, { 0, 0 } };
					}

					CmdWaitForPreviousComputeShader();

					// Rigidbody Remove
					_shaders.RigidBodyRemove->Update();

					_shaders.RigidBodyRemove->Bind(_commandBuffer.GetVkCommandBuffer());

					_shaders.RigidBodyRemove->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &_readIndex);

					_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroups, 1, 1);

					CmdWaitForPreviousComputeShader();

					// Contour
					if (simulationData->timer % 4 == 2)
					{
						_shaders.MarchingSquareAlgorithm->Update();

						_shaders.MarchingSquareAlgorithm->Bind(_commandBuffer.GetVkCommandBuffer());

						_shaders.MarchingSquareAlgorithm->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &_readIndex);

						_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(((simulationData->simulationWidth + simulationData->simulationHeight) * 3) - 2, 64u), 1, 1);

						CmdWaitForPreviousComputeShader();
					}

					// Rigidbody
					_shaders.RigidBodySimulation->Update();

					_shaders.RigidBodySimulation->Bind(_commandBuffer.GetVkCommandBuffer());

					_shaders.RigidBodySimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &_readIndex);

					for (int rigidBodyShaderID = 1; rigidBodyShaderID < _rigidBodyEntities.size(); ++rigidBodyShaderID)
					{
						RigidbodyEntry& rigidBodyEntry = _rigidBodyEntities[rigidBodyShaderID];

						if (!rigidBodyEntry.Enabled) { continue; }

						if (!rigidBodyEntry.Active)
						{
							rigidBodyEntry.Active = true;
							continue;
						}

						if (simulationData->timer % 4 == 0 && rigidBodyData->rigidBodies[rigidBodyShaderID].NeedsRecalculation)
						{
							_deleteRigidbody.push_back(rigidBodyShaderID);
							rigidBodyData->rigidBodies[rigidBodyShaderID].ID = 0;
						}

						Component::Transform&          transform          = ecsRegistry.GetComponent<Component::Transform>(rigidBodyEntry.EntityID);
						Component::PixelGridRigidBody& pixelGridRigidBody = ecsRegistry.GetComponent<Component::PixelGridRigidBody>(rigidBodyEntry.EntityID);

						rigidBodyData->rigidBodies[rigidBodyShaderID].Position = transform.Position * 10.0f;
						rigidBodyData->rigidBodies[rigidBodyShaderID].Rotation = transform.Rotation;

						_shaders.RigidBodySimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &pixelGridRigidBody.ShaderRigidBodyID);

						_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(pixelGridRigidBody.Size.x * pixelGridRigidBody.Size.y, 64u), 1, 1);
					}

					_commandBuffer.GetVkCommandBuffer().end();

					_doStep = false;
				}
				else
				{
					_commandBuffer.GetVkCommandBuffer().reset({});

					constexpr vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo({}, nullptr);

					_commandBuffer.GetVkCommandBuffer().begin(commandBufferBeginInfo);

					_shaders.IdleSimulation->Update();

					_shaders.IdleSimulation->Bind(_commandBuffer.GetVkCommandBuffer());

					_shaders.IdleSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &_readIndex);

					_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroups, 1, 1);

					_commandBuffer.GetVkCommandBuffer().end();
				}

				vk::SubmitInfo submitInfo;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers    = &_commandBuffer.GetVkCommandBuffer();

				computeQueue.GetVkQueue().submit(submitInfo, _computeFence);

				_fif = (_fif + 1) % device.MAX_FRAMES_IN_FLIGHT;
			}

			if (stage == Stage::Rendering)
			{
				vk::CommandBuffer commandBuffer = contextProvider.GetContext<RenderingContext>()->Renderer->GetCommandBuffer().GetVkCommandBuffer();

				DebugMaterial->GetShader()->BindGlobal(commandBuffer);

				DebugMaterial->GetShader()->Update();

				DebugMaterial->GetShader()->Bind(commandBuffer);

				DebugMaterial->Update();

				DebugMaterial->Bind(commandBuffer);

				size_t offset = 0;
				commandBuffer.bindVertexBuffers(0, _vertexBuffer.GetVkBuffer(), offset);
				commandBuffer.draw(_numLineSegements * 2, 1, 0, 0);
			}
		}
	}
}
