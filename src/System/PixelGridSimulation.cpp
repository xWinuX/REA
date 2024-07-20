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

			ImGui::Checkbox("remove on next frame", &_removeOnNextFrame);

			ImGui::SliderFloat("Line Simplification Tolerance", &_lineSimplificationTolerance, 0.0f, 100.0f);
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

	struct LineSegment
	{
		CDT::V2d<float> startPoint;
		CDT::V2d<float> endPoint;
	};

	struct Labels
	{
		int32_t L[1048576];
	};

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
				//BENCHMARK_BEGIN
				device.GetVkDevice().waitForFences(_computeFence, vk::True, UINT64_MAX);
				device.GetVkDevice().resetFences(_computeFence);
				//				BENCHMARK_END("compute fence")

				pixelGrid.PixelState = &_shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_Pixels>(2)->Pixels[0];
				pixelGrid.Labels     = &_shaders.CCLInitialize->GetProperties().GetBufferData<Labels>(5)->L[0];
			}

			if (stage == Stage::GridComputeHalted)
			{
				SSBO_MarchingCubes* marchingCubes = _shaders.MarchingSquareAlgorithm->GetProperties().GetBufferData<SSBO_MarchingCubes>(5);


				/*size_t     vi = 0;
				glm::vec2* v  = _vertexBuffer.GetMappedData<glm::vec2>();
				for (vi = 0; vi< marchingCubes->numSegments; ++vi)
				{
					v[vi * 2]     = { marchingCubes->segments[vi * 2].x, marchingCubes->segments[vi * 2].y };
					v[vi * 2 + 1] = { marchingCubes->segments[vi * 2 + 1].x, marchingCubes->segments[vi * 2 + 1].y };
				}

				_numLineSegements = vi;
*/


				if (Input::GetDown(KeyCode::RIGHT)) { pixelGrid.ViewTargetPosition .x += 1.0f; }
				if (Input::GetDown(KeyCode::UP)) { pixelGrid.ViewTargetPosition .y += 1.0f; }
				if (Input::GetDown(KeyCode::LEFT)) { pixelGrid.ViewTargetPosition .x -= 1.0f; }
				if (Input::GetDown(KeyCode::DOWN)) { pixelGrid.ViewTargetPosition .y -= 1.0f; }

				pixelGrid.ViewTargetPosition = glm::clamp(pixelGrid.ViewTargetPosition, {0.0f, 0.0f}, {pixelGrid.Width/2, pixelGrid.Height/2});

				// Delete Rigidbodies
				if (simulationData->timer % 4 == 0)
				{
					for (uint32_t deleteRigidbody: _deleteRigidbody)
					{
						LOG("delete Rigidbody {0}", deleteRigidbody);
						RigidbodyEntry& rigidbodyEntry = _rigidBodyEntities[deleteRigidbody];

						Component::PixelGridRigidBody& pixelGridRigidBody = ecsRegistry.GetComponent<Component::PixelGridRigidBody>(rigidbodyEntry.EntityID);

						rigidBodyData->rigidBodies[pixelGridRigidBody.ShaderRigidBodyID].NeedsRecalculation = false;

						LOG("Delete allocation id {0}", pixelGridRigidBody.AllocationID);
						_rigidBodyDataHeap.Deallocate(pixelGridRigidBody.AllocationID);

						ecsRegistry.DestroyEntity(rigidbodyEntry.EntityID);

						rigidbodyEntry.Enabled = false;

						_availableRigidBodyIDs.Push(deleteRigidbody);
					}

					_deleteRigidbody.clear();
				}

				if (simulationData->timer % 4 == 2)
				{
					//SSBO_MarchingCubes* marchingCubes = _shaders.MarchingSquareAlgorithm->GetProperties().GetBufferData<SSBO_MarchingCubes>(5);

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
								CDT::V2d<float>& v1 = solidPolyline.Vertices[i];
								CDT::V2d<float>& v2 = solidPolyline.Vertices[(i + 1) % solidPolyline.Vertices.size()];

								b2EdgeShape edgeShape{};
								edgeShape.SetTwoSided({ v1.x, v1.y }, { v2.x, v2.y });

								fixtureDef.shape = &edgeShape;

								collider.Fixtures.push_back(collider.Body->CreateFixture(&fixtureDef));
							}
						}
					}

					std::vector<CDT::Edge> connectedEdges = std::vector<CDT::Edge>(marchingCubes->numConnectedSegments, CDT::Edge(0, 0));
					for (int i = 0; i < marchingCubes->numConnectedSegments; ++i) { connectedEdges[i] = CDT::Edge(i * 2, (i * 2) + 1); }

					std::span<CDT::V2d<float>> connectedSegmentSpan = std::span(reinterpret_cast<CDT::V2d<float>*>(&marchingCubes->connectedSegments[0]),
					                                                            marchingCubes->numConnectedSegments * 2);

					MarchingSquareMesherUtils::RemoveDuplicatesAndRemapEdges(connectedSegmentSpan, connectedEdges);

					std::vector<MarchingSquareMesherUtils::Polyline> connectedPolylines = MarchingSquareMesherUtils::SeperateAndSortPolylines(connectedSegmentSpan, connectedEdges);

					std::vector<size_t>                              polylinesToMove{};
					std::vector<MarchingSquareMesherUtils::Polyline> polylines{};

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

					if (polylines.size() > 0) { _numLineSegements = 0; }

					for (MarchingSquareMesherUtils::Polyline& polyline: polylines)
					{
						// Convert to integer grid coordinates
						glm::ivec2 seedPoint = glm::ivec2(glm::floor(polyline.Vertices[0].x), glm::floor(polyline.Vertices[0].y));

						// Draw
						size_t     vi = 0;
						glm::vec2* v  = _vertexBuffer.GetMappedData<glm::vec2>();

						for (vi = 0; vi < polyline.Vertices.size(); ++vi)
						{
							v[_numLineSegements * 2 + vi * 2]     = { polyline.Vertices[vi].x, polyline.Vertices[vi].y };
							v[_numLineSegements * 2 + vi * 2 + 1] = {
								polyline.Vertices[(vi + 1) % polyline.Vertices.size()].x,
								polyline.Vertices[(vi + 1) % polyline.Vertices.size()].y
							};
						}

						_numLineSegements += (polyline.Vertices.size());

						// Simplify line
						polyline = MarchingSquareMesherUtils::SimplifyPolylines(polyline, _lineSimplificationTolerance);

						if (polyline.Vertices.size() < 3) { continue; }

						b2AABB b2Aabb = polyline.AABB;

						// Calculate size needed
						b2Vec2     extends  = b2Aabb.GetExtents();
						glm::uvec2 aabbSize = { glm::round(extends.x * 2), glm::round(extends.y * 2) };
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
						glm::uvec2 bottomLeftCorner   = { b2BottomLeftCorner.x, b2BottomLeftCorner.y };

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

				LOG("num pixels {0}", numPixels);

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
/*
					// Do CCL if there are rigidbodies to discover
					if (!_newRigidBodies.empty())
					{
						LOG("Do ccL");
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

						_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(((simulationData->width + simulationData->height) * 3) - 2, 64u), 1, 1);

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

						rigidBodyData->rigidBodies[rigidBodyShaderID].Position = transform.Position;
						rigidBodyData->rigidBodies[rigidBodyShaderID].Rotation = transform.Rotation;

						_shaders.RigidBodySimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &pixelGridRigidBody.ShaderRigidBodyID);

						_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(pixelGridRigidBody.Size.x * pixelGridRigidBody.Size.y, 64u), 1, 1);
					}*/

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
