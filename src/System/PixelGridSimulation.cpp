#include "REA/System/PixelGridSimulation.hpp"

#include <CDT.h>
#include <execution>
#include <IconsFontAwesome.h>
#include <imgui.h>
#include <utility>
#include <vector>
#include <box2d/b2_edge_shape.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_chain_shape.h>
#include <glm/gtc/random.hpp>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Debug/Performance.hpp>
#include <SplitEngine/Rendering/Material.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>
#include <SplitEngine/Rendering/Vulkan/Device.hpp>

#include <glm/ext/matrix_transform.hpp>

#include "REA/Assets.hpp"
#include "REA/MarchingSquareMesherUtils.hpp"
#include "REA/Math.hpp"
#include "REA/PixelType.hpp"
#include "REA/Stage.hpp"
#include "REA/WorldGenerator.hpp"
#include "REA/Component/PixelGridRigidBody.hpp"
#include "REA/Component/Transform.hpp"
#include "REA/Context/ImGui.hpp"
#include "REA/Constants.hpp"


namespace REA::System
{
	PixelGridSimulation::PixelGridSimulation(const SimulationShaders& simulationShaders, ECS::ContextProvider& contextProvider):
		_shaders(simulationShaders)
	{
		EngineContext* engineContext = contextProvider.GetContext<EngineContext>();
		ECS::Registry& ecsRegistry   = engineContext->Application->GetECSRegistry();

		auto&                      properties = _shaders.IdleSimulation->GetProperties();
		Rendering::Vulkan::Device* device     = _shaders.IdleSimulation->GetPipeline().GetDevice();


		vk::DeviceSize bufferSize   = sizeof(Pixel::State) * (Constants::NUM_CHUNKS * Constants::NUM_ELEMENTS_IN_CHUNK);
		vk::DeviceSize minAlignment = device->GetPhysicalDevice().GetProperties().limits.minStorageBufferOffsetAlignment;
		vk::DeviceSize padding      = minAlignment - (bufferSize % minAlignment);
		padding                     = padding == minAlignment ? 0 : padding;
		bufferSize += padding;

		_copyBuffer = Rendering::Vulkan::Buffer(device,
		                                        vk::Flags<vk::BufferUsageFlagBits>(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer),
		                                        vk::SharingMode::eExclusive,
		                                        {
			                                        Rendering::Vulkan::Allocator::Auto,
			                                        vk::Flags<
				                                        Rendering::Vulkan::Allocator::MemoryAllocationCreateFlagBits>(Rendering::Vulkan::Allocator::RandomAccess |
				                                                                                                      Rendering::Vulkan::Allocator::PersistentMap),
			                                        vk::Flags<Rendering::Vulkan::Allocator::MemoryPropertyFlagBits>(Rendering::Vulkan::Allocator::HostCached |
			                                                                                                        Rendering::Vulkan::Allocator::HostCoherant |
			                                                                                                        Rendering::Vulkan::Allocator::HostVisible)
		                                        },
		                                        1u,
		                                        bufferSize);


		_commandBuffer = std::move(device->GetQueueFamily(Rendering::Vulkan::QueueType::Compute).AllocateCommandBuffer(Rendering::Vulkan::QueueType::Compute));

		_commandBuffer.GetVkCommandBufferRaw().SetFramePtr(&_fif);

		// Remap buffers for swapping
		Rendering::Vulkan::Buffer& writeBuffer = properties.GetBuffer(3);
		for (int i = 0; i < device->MAX_FRAMES_IN_FLIGHT; ++i)
		{
			uint32_t fifIndex    = (i + 1) % device->MAX_FRAMES_IN_FLIGHT;
			auto&    bufferInfos = properties.GetBufferInfo(3, fifIndex);
			properties.SetBuffer(4, writeBuffer, bufferInfos.offset, bufferInfos.range, i);
		}

		_shaders.IdleSimulation->Update();

		// Create Fences
		vk::FenceCreateInfo fenceCreateInfo = vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
		_computeFence                       = _shaders.IdleSimulation->GetPipeline().GetDevice()->GetVkDevice().createFence(fenceCreateInfo);

		// Create static environment Collider
		for (uint64_t& staticEnvironmentEntityID: _staticEnvironmentEntityIDs)
		{
			Component::Collider collider;
			collider.InitialType      = b2_staticBody;
			collider.PhysicsMaterial  = engineContext->Application->GetAssetDatabase().GetAsset<PhysicsMaterial>(Asset::PhysicsMaterial::Defaut);
			staticEnvironmentEntityID = ecsRegistry.CreateEntity<Component::Transform, Component::Collider>({}, std::move(collider));
		}
	}

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
			ImGui::SliderFloat("Cave Noise Freqency", &_worldGenerationSettings.CaveNoiseFrequency, 0.0f, 1.0f);
			ImGui::SliderFloat("Overworld Noise Freqency", &_worldGenerationSettings.OverworldNoiseFrequency, 0.0f, 1.0f);
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

	int mod(int a, int b) { return (a % b + b) % b; }

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
			SSBO_SimulationData* simulationData = _shaders.IdleSimulation->GetProperties().GetBufferData<SSBO_SimulationData>(0);
			SSBO_RigidBodyData*  rigidBodyData  = _shaders.IdleSimulation->GetProperties().GetBufferData<SSBO_RigidBodyData>(2);
			SSBO_Updates*        updates        = _shaders.IdleSimulation->GetProperties().GetBufferData<SSBO_Updates>(1);

			Component::PixelGrid& pixelGrid = pixelGrids[entityIndex];

			if (stage == Stage::GridComputeEnd)
			{
				device.GetVkDevice().waitForFences(_computeFence, vk::True, UINT64_MAX);
				device.GetVkDevice().resetFences(_computeFence);

				pixelGrid.Chunks = &_shaders.IdleSimulation->GetProperties().GetBufferData<SSBO_Pixels>(3, _fif)->Chunks;
				memcpy(pixelGrid.ChunkRegenerate.data(), &updates->regenerateChunks[0], sizeof(uint32_t) * Constants::NUM_CHUNKS);
			}

			if (stage == Stage::GridComputeHalted)
			{
				SSBO_MarchingCubes* marchingCubes = _shaders.MarchingSquareAlgorithm->GetProperties().GetBufferData<SSBO_MarchingCubes>(5);

				if (simulationData->timer % 4 == 2)
				{
					std::vector<std::vector<MarchingSquareMesherUtils::Polyline>> solidPolylineCollection = std::vector<std::vector<
						MarchingSquareMesherUtils::Polyline>>(Constants::NUM_CHUNKS, {});


					//BENCHMARK_BEGIN
					_indexes = std::ranges::iota_view(0ull, static_cast<size_t>(Constants::NUM_CHUNKS));
					std::for_each(std::execution::par_unseq,
					              _indexes.begin(),
					              _indexes.end(),
					              [&](size_t chunkIndex)
					              {
						              uint32_t             chunkMapping = pixelGrid.ChunkMapping[chunkIndex];
						              MarchingSquareChunk& chunk        = marchingCubes->worldChunks[chunkMapping];

						              if (!pixelGrid.ChunkRegenerate[chunkMapping]) { return; }

						              std::vector<CDT::Edge> solidEdges = std::vector<CDT::Edge>(chunk.numSegments, CDT::Edge(0, 0));
						              for (int i = 0; i < chunk.numSegments; ++i) { solidEdges[i] = CDT::Edge(i * 2, (i * 2) + 1); }

						              std::span<CDT::V2d<float>> solidSegmentSpan = std::span(reinterpret_cast<CDT::V2d<float>*>(&chunk.segments[0]), chunk.numSegments * 2);

						              MarchingSquareMesherUtils::RemoveDuplicatesAndRemapEdges(solidSegmentSpan, solidEdges);

						              std::vector<MarchingSquareMesherUtils::Polyline> solidPolylines =
								              MarchingSquareMesherUtils::SeperateAndSortPolylines(solidSegmentSpan, solidEdges);

						              for (MarchingSquareMesherUtils::Polyline& solidPolyline: solidPolylines)
						              {
							              solidPolylineCollection[chunkMapping].push_back(std::move(MarchingSquareMesherUtils::SimplifyPolylines(solidPolyline,
								                                                                        _lineSimplificationTolerance)));
						              }
					              });

					//					BENCHMARK_END("Environment Collisions Calculate")


					//	BENCHMARK_BEGIN

					for (int chunkIndex = 0; chunkIndex < Constants::NUM_CHUNKS; ++chunkIndex)
					{
						uint32_t             chunkMapping = pixelGrid.ChunkMapping[chunkIndex];
						Component::Collider& collider     = ecsRegistry.GetComponent<Component::Collider>(_staticEnvironmentEntityIDs[chunkMapping]);

						if (!pixelGrid.ChunkRegenerate[chunkMapping]) { continue; }

						std::vector<MarchingSquareMesherUtils::Polyline>& solidPolylines = solidPolylineCollection[chunkMapping];

						if (collider.Body != nullptr)
						{
							for (b2Fixture* fixture: pixelGrid.ChunkFixtures[chunkMapping]) { collider.Body->DestroyFixture(fixture); }

							pixelGrid.ChunkFixtures[chunkMapping].clear();

							b2FixtureDef fixtureDef = collider.PhysicsMaterial->GetFixtureDefCopy();
							for (MarchingSquareMesherUtils::Polyline solidPolyline: solidPolylines)
							{
								size_t size = solidPolyline.Closed ? solidPolyline.Vertices.size() : solidPolyline.Vertices.size() - 1;
								for (int i = 0; i < size; ++i)
								{
									CDT::V2d<float>& v1 = solidPolyline.Vertices[i];
									CDT::V2d<float>& v2 = solidPolyline.Vertices[(i + 1) % solidPolyline.Vertices.size()];

									b2EdgeShape edgeShape{};
									edgeShape.SetTwoSided({ v1.x, v1.y }, { v2.x, v2.y });

									fixtureDef.shape = &edgeShape;

									pixelGrid.ChunkFixtures[chunkMapping].push_back(collider.Body->CreateFixture(&fixtureDef));
								}
							}

							pixelGrid.ChunkRegenerate[chunkMapping] = false;
						}
					}

					//BENCHMARK_END("Environment Collision Apply")

					// Create rigidbodies and their colission meshes
					std::vector<CDT::Edge> connectedEdges = std::vector<CDT::Edge>(marchingCubes->connectedChunk.numSegments, CDT::Edge(0, 0));
					for (int i = 0; i < marchingCubes->connectedChunk.numSegments; ++i) { connectedEdges[i] = CDT::Edge(i * 2, (i * 2) + 1); }

					std::span<CDT::V2d<float>> connectedSegmentSpan = std::span(reinterpret_cast<CDT::V2d<float>*>(&marchingCubes->connectedChunk.segments[0]),
					                                                            marchingCubes->connectedChunk.numSegments * 2);

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
						glm::uvec2 aabbSize = { glm::round(extends.x * 2 * 10.0f), glm::round(extends.y * 2 * 10.0f) };
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
						if (pixelGrid.AvailableRigidBodyIDs.IsEmpty())
						{
							shaderID = pixelGrid.RigidBodyIDCounter++;
							pixelGrid.RigidBodyEntities.resize(pixelGrid.RigidBodyIDCounter);
						}
						else { shaderID = pixelGrid.AvailableRigidBodyIDs.Pop(); }

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

						pixelGrid.NewRigidBodies.push_back({ bottomLeftCorner, aabbSize, seedPoint, shaderID });

						// Create entity
						uint64_t rigidBodyEntity = ecsRegistry.CreateEntity<Component::Transform, Component::PixelGridRigidBody, Component::Collider>(std::move(transform),
							std::move(pixelGridRigidBody),
							std::move(collider));

						pixelGrid.RigidBodyEntities[shaderID] = { rigidBodyEntity, true, false };
					}
				}

				// Pixelgrid follows camera
				if (ecsRegistry.IsEntityValid(pixelGrid.CameraEntityID))
				{
					// This is inside of the pixelGrid variable
					glm::vec2 offset = { -pixelGrid.SimulationWidth, -pixelGrid.SimulationHeight };

					glm::vec2 targetPosition = ecsRegistry.GetComponent<Component::Transform>(pixelGrid.CameraEntityID).Position * 10.0f;

					targetPosition += offset / 2.0f;

					targetPosition = glm::clamp(targetPosition, { 0.0f, 0.0f }, glm::vec2(pixelGrid.WorldWidth, pixelGrid.WorldHeight) + offset);

					pixelGrid.PreviousChunkOffset = pixelGrid.ChunkOffset;
					pixelGrid.ChunkOffset         = glm::ivec2(static_cast<uint32_t>(targetPosition.x) / Constants::CHUNK_SIZE,
					                                           static_cast<uint32_t>(targetPosition.y) / Constants::CHUNK_SIZE);

					if (pixelGrid.PreviousChunkOffset != pixelGrid.ChunkOffset)
					{
						BENCHMARK_BEGIN
							PixelChunks* copyPixels = &_copyBuffer.GetMappedData<SSBO_CopyPixels>()->Chunks;

							// Copy data from the world to the chunk
							Rendering::Vulkan::Buffer& pixelsBuffer = _shaders.FallingSimulation->GetProperties().GetBuffer(3);

							vk::DeviceSize offset = _fif == 0 ? 0 : pixelsBuffer.GetSizeInBytes();
							pixelsBuffer.Copy(_copyBuffer, offset, 0, pixelsBuffer.GetSizeInBytes());

							// Save old chunks
							glm::ivec2 loadDirection = pixelGrid.PreviousChunkOffset - pixelGrid.ChunkOffset;
							for (int y = 0; y < pixelGrid.SimulationChunksY; ++y)
							{
								for (int x = 0; x < pixelGrid.SimulationChunksX; ++x)
								{
									int oldWorldX = pixelGrid.PreviousChunkOffset.x + x;
									int oldWorldY = pixelGrid.PreviousChunkOffset.y + y;

									bool wasInOldBounds = x + loadDirection.x < 0 || x + loadDirection.x >= pixelGrid.SimulationChunksX || y + loadDirection.y < 0 || y +
									                      loadDirection.y >= pixelGrid.SimulationChunksY;

									int chunkIndex = y * pixelGrid.SimulationChunksX + x;

									if (wasInOldBounds)
									{
										int worldIndex = oldWorldY * pixelGrid.WorldChunksX + oldWorldX;
										memcpy(pixelGrid.World[worldIndex].data(),
										       (*copyPixels)[pixelGrid.ChunkMapping[chunkIndex]].data(),
										       Constants::NUM_ELEMENTS_IN_CHUNK * sizeof(Pixel::State));
									}
								}
							}

							// Remap mappings
							for (int y = 0; y < pixelGrid.SimulationChunksY; ++y)
							{
								for (int x = 0; x < pixelGrid.SimulationChunksX; ++x)
								{
									int chunkIndex = y * pixelGrid.SimulationChunksX + x;

									int wrappedX = mod(x - pixelGrid.ChunkOffset.x, pixelGrid.SimulationChunksX);
									int wrappedY = mod(y - pixelGrid.ChunkOffset.y, pixelGrid.SimulationChunksY);

									if (wrappedX < 0) { wrappedX += pixelGrid.SimulationChunksX; }
									if (wrappedY < 0) { wrappedY += pixelGrid.SimulationChunksY; }

									int newMappingIndex                     = wrappedY * pixelGrid.SimulationChunksX + wrappedX;
									pixelGrid.ChunkMapping[newMappingIndex] = chunkIndex;
								}
							}

							// Load new chunks
							loadDirection = pixelGrid.ChunkOffset - pixelGrid.PreviousChunkOffset;
							for (int y = 0; y < pixelGrid.SimulationChunksY; ++y)
							{
								for (int x = 0; x < pixelGrid.SimulationChunksX; ++x)
								{
									int chunkIndex = y * pixelGrid.SimulationChunksX + x;

									int newWorldX = pixelGrid.ChunkOffset.x + x;
									int newWorldY = pixelGrid.ChunkOffset.y + y;

									bool isInNewBounds = x + loadDirection.x < 0 || x + loadDirection.x >= pixelGrid.SimulationChunksX || y + loadDirection.y < 0 || y +
									                     loadDirection.y >= pixelGrid.SimulationChunksY;

									if (isInNewBounds)
									{
										int      worldIndex   = newWorldY * pixelGrid.WorldChunksX + newWorldX;
										unsigned chunkMapping = pixelGrid.ChunkMapping[chunkIndex];

										pixelGrid.ChunkRegenerate[chunkMapping] = true;

										memcpy((*pixelGrid.Chunks)[chunkMapping].data(),
										       pixelGrid.World[worldIndex].data(),
										       Constants::NUM_ELEMENTS_IN_CHUNK * sizeof(Pixel::State));
									}
								}
							}

						BENCHMARK_END("Chunking")
					}
				}

				// Delete Rigidbodies
				if (simulationData->timer % 4 == 0)
				{
					for (uint32_t deleteRigidbody: pixelGrid.DeleteRigidbody)
					{
						Component::PixelGrid::RigidbodyEntry& rigidbodyEntry = pixelGrid.RigidBodyEntities[deleteRigidbody];

						Component::PixelGridRigidBody& pixelGridRigidBody = ecsRegistry.GetComponent<Component::PixelGridRigidBody>(rigidbodyEntry.EntityID);

						rigidBodyData->rigidBodies[pixelGridRigidBody.ShaderRigidBodyID].NeedsRecalculation = false;

						_rigidBodyDataHeap.Deallocate(pixelGridRigidBody.AllocationID);

						ecsRegistry.DestroyEntity(rigidbodyEntry.EntityID);

						rigidbodyEntry.Enabled = false;

						pixelGrid.AvailableRigidBodyIDs.Push(deleteRigidbody);
					}

					pixelGrid.DeleteRigidbody.clear();
				}

				marchingCubes->connectedChunk.numSegments = 0;
				for (MarchingSquareChunk& worldChunk: marchingCubes->worldChunks) { worldChunk.numSegments = 0; }
			}

			if (stage == Stage::GridComputeBegin)
			{
				size_t   numPixels     = pixelGrid.SimulationWidth * pixelGrid.SimulationHeight;
				size_t   numWorkgroups = CeilDivide(numPixels, 64ull);
				uint32_t fif           = _fif;

				simulationData->chunkOffset = pixelGrid.ChunkOffset;

				for (int i = 0; i < simulationData->chunkMapping.size(); ++i) { simulationData->chunkMapping[i] = pixelGrid.ChunkMapping[i]; }

				memcpy(&updates->regenerateChunks[0], pixelGrid.ChunkRegenerate.data(), sizeof(uint32_t) * Constants::NUM_CHUNKS);

				if (_firstUpdate)
				{
					memcpy(simulationData->pixelLookup, pixelGrid.PixelDataLookup.data(), pixelGrid.PixelDataLookup.size() * sizeof(Pixel::Data));

					_firstUpdate = false;
				}

				if (_generateWorld)
				{
					std::vector<Pixel::State> world = std::vector<Pixel::State>(pixelGrid.WorldWidth * pixelGrid.WorldHeight, pixelGrid.PixelLookup[PixelType::Air].PixelState);

					BENCHMARK_BEGIN
						WorldGenerator::GenerateWorld(world, pixelGrid, _worldGenerationSettings);

						// Split up world into chunks
						for (int chunkY = 0; chunkY < pixelGrid.WorldChunksY; ++chunkY)
						{
							for (int chunkX = 0; chunkX < pixelGrid.WorldChunksX; ++chunkX)
							{
								std::vector<Pixel::State>& chunk = pixelGrid.World[chunkY * pixelGrid.WorldChunksX + chunkX];

								for (int y = 0; y < Constants::CHUNK_SIZE; ++y)
								{
									for (int x = 0; x < Constants::CHUNK_SIZE; ++x)
									{
										int worldX = chunkX * Constants::CHUNK_SIZE + x;
										int worldY = chunkY * Constants::CHUNK_SIZE + y;

										if (worldX < pixelGrid.WorldWidth && worldY < pixelGrid.WorldHeight)
										{
											int worldIndex    = worldY * pixelGrid.WorldWidth + worldX;
											int chunkIndex    = y * Constants::CHUNK_SIZE + x;
											chunk[chunkIndex] = world[worldIndex];
										}
									}
								}
							}
						}
					BENCHMARK_END("World Generation")


					BENCHMARK_BEGIN
						for (int y = 0; y < pixelGrid.SimulationChunksY; ++y)
						{
							for (int x = 0; x < pixelGrid.SimulationChunksX; ++x)
							{
								// Calculate the corresponding world coordinates
								int worldX = x + pixelGrid.ChunkOffset.x;
								int worldY = y + pixelGrid.ChunkOffset.y;

								memcpy((*pixelGrid.Chunks)[pixelGrid.ChunkMapping[y * pixelGrid.SimulationChunksX + x]].data(),
								       pixelGrid.World[worldY * pixelGrid.WorldChunksX + worldX].data(),
								       Constants::NUM_ELEMENTS_IN_CHUNK * sizeof(Pixel::State));
							}
						}
					BENCHMARK_END("Copy CPU to GPU")

					_generateWorld = false;
				}

				if (!_paused || _doStep)
				{
					simulationData->deltaTime = contextProvider.GetContext<TimeContext>()->DeltaTime;
					simulationData->timer     = simulationData->timer + 1;
					simulationData->rng       = glm::linearRand(0.0f, 1.0f);


					_commandBuffer.GetVkCommandBuffer().reset({});

					vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo({}, nullptr);

					_commandBuffer.GetVkCommandBuffer().begin(commandBufferBeginInfo);

					// Rigidbody
					_shaders.RigidBodySimulation->Update();

					_shaders.RigidBodySimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					for (int rigidBodyShaderID = 1; rigidBodyShaderID < pixelGrid.RigidBodyEntities.size(); ++rigidBodyShaderID)
					{
						Component::PixelGrid::RigidbodyEntry& rigidBodyEntry = pixelGrid.RigidBodyEntities[rigidBodyShaderID];

						if (!rigidBodyEntry.Enabled) { continue; }

						if (!rigidBodyEntry.Active)
						{
							rigidBodyEntry.Active = true;
							continue;
						}

						Component::Transform&          transform          = ecsRegistry.GetComponent<Component::Transform>(rigidBodyEntry.EntityID);
						Component::Collider&           collider           = ecsRegistry.GetComponent<Component::Collider>(rigidBodyEntry.EntityID);
						Component::PixelGridRigidBody& pixelGridRigidBody = ecsRegistry.GetComponent<Component::PixelGridRigidBody>(rigidBodyEntry.EntityID);

						bool isOutsideOfRange = false;
						if (ecsRegistry.IsEntityValid(pixelGrid.CameraEntityID))
						{
							glm::vec2 targetPosition = ecsRegistry.GetComponent<Component::Transform>(pixelGrid.CameraEntityID).Position;

							bool range = glm::distance(targetPosition, glm::vec2(transform.Position.x, transform.Position.y)) * 10.0f > 512.0f;

							if (range) { collider.Body->SetEnabled(false); }
							else { collider.Body->SetEnabled(true); }
						}

						if (simulationData->timer % 4 == 0 && (rigidBodyData->rigidBodies[rigidBodyShaderID].NeedsRecalculation || pixelGridRigidBody.Size.x < 3 ||
						                                       pixelGridRigidBody.Size.y < 3))
						{
							pixelGrid.DeleteRigidbody.push_back(rigidBodyShaderID);
							rigidBodyData->rigidBodies[rigidBodyShaderID].ID = 0;
						}

						float counterForce = rigidBodyData->rigidBodies[rigidBodyShaderID].CounterVelocity.x * 5.0f;
						collider.Body->SetLinearDamping(rigidBodyData->rigidBodies[rigidBodyShaderID].CounterVelocity.x*5.0f);
						collider.Body->SetAngularDamping(rigidBodyData->rigidBodies[rigidBodyShaderID].CounterVelocity.x*5.0f);

						rigidBodyData->rigidBodies[rigidBodyShaderID].Position        = transform.Position * 10.0f;
						rigidBodyData->rigidBodies[rigidBodyShaderID].Velocity        = collider.Body->GetLinearVelocity();
						rigidBodyData->rigidBodies[rigidBodyShaderID].CounterVelocity = { 0, 0 };
						rigidBodyData->rigidBodies[rigidBodyShaderID].Rotation        = transform.Rotation;

						_shaders.RigidBodySimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &pixelGridRigidBody.ShaderRigidBodyID);

						_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(pixelGridRigidBody.Size.x * pixelGridRigidBody.Size.y, 64u), 1, 1);
					}
					CmdWaitForPreviousComputeShader();

					// Fall and Flow
					uint32_t   flowIteration         = 0;
					uint32_t   timer                 = simulationData->timer;
					glm::uvec2 margolusOffset        = GetMargolusOffset(timer);
					size_t     numWorkgroupsMorgulus = CeilDivide(CeilDivide(numPixels, 4ull), 64ull);

					_shaders.FallingSimulation->Update();

					for (int i = 0; i < 8; ++i)
					{
						_shaders.FallingSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

						_shaders.FallingSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &flowIteration);
						_shaders.FallingSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &margolusOffset);

						_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroupsMorgulus, 1, 1);

						margolusOffset = GetMargolusOffset(timer + i);
						flowIteration++;

						CmdWaitForPreviousComputeShader();
					}

					// Temperature and Charge
					_shaders.AccumulateSimulation->Update();

					_shaders.AccumulateSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroups, 1, 1);

					CmdWaitForPreviousComputeShader();

					// Do CCL if there are rigidbodies to discover
					if (!pixelGrid.NewRigidBodies.empty())
					{
						b2Vec2 extends            = _cclRange.GetExtents();
						b2Vec2 b2BottomLeftCorner = _cclRange.GetCenter() - extends;

						glm::uvec2 offset = { 0, 0 };
						glm::uvec2 size   = { Constants::NUM_ELEMENTS_X, Constants::NUM_ELEMENTS_Y };

						uint32_t numWorkGroupsForSinglePixelOps = CeilDivide(size.x * size.y, 64u);

						_shaders.CCLInitialize->Update();

						_shaders.CCLInitialize->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

						_shaders.CCLInitialize->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &offset);
						_shaders.CCLInitialize->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &size);

						_commandBuffer.GetVkCommandBuffer().dispatch(numWorkGroupsForSinglePixelOps, 1, 1);

						CmdWaitForPreviousComputeShader();

						_shaders.CCLColumn->Update();

						_shaders.CCLColumn->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

						_shaders.CCLColumn->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &offset);
						_shaders.CCLColumn->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &size);

						_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(size.x, 64u), 1, 1);

						// Merge
						uint32_t stepIndex = 0;
						uint32_t n         = size.x >> 1;
						while (n != 0)
						{
							CmdWaitForPreviousComputeShader();

							_shaders.CCLMerge->Update();

							_shaders.CCLMerge->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

							_shaders.CCLMerge->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &offset);
							_shaders.CCLMerge->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &size);
							_shaders.CCLMerge->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 2, &stepIndex);

							_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(n, 64u), 1, 1);

							n = n >> 1;
							stepIndex++;
						}

						CmdWaitForPreviousComputeShader();

						// Relabel
						_shaders.CCLRelabel->Update();

						_shaders.CCLRelabel->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

						_shaders.CCLRelabel->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &offset);
						_shaders.CCLRelabel->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &size);

						_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(size.x * size.y, 64u), 1, 1);

						CmdWaitForPreviousComputeShader();

						// Extract
						_shaders.CCLExtract->Update();

						_shaders.CCLExtract->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

						for (Component::PixelGrid::NewRigidBody newRigidBody: pixelGrid.NewRigidBodies)
						{
							_shaders.CCLExtract->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &newRigidBody.Offset);
							_shaders.CCLExtract->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &newRigidBody.Size);
							_shaders.CCLExtract->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 2, &newRigidBody.SeedPoint);
							_shaders.CCLExtract->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 3, &newRigidBody.RigidBodyID);
							_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(newRigidBody.Size.x * newRigidBody.Size.y, 64u), 1, 1);
						}

						pixelGrid.NewRigidBodies.clear();
						_cclRange = { { 10'000'000, 10'000'000 }, { 0, 0 } };
					}

					// Particles
					_shaders.PixelParticle->Update();

					_shaders.PixelParticle->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(Constants::MAX_PIXEL_PARTICLES, 64u), 1, 1);

					CmdWaitForPreviousComputeShader();

					// Rigidbody Remove
					_shaders.RigidBodyRemove->Update();

					_shaders.RigidBodyRemove->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroups, 1, 1);

					CmdWaitForPreviousComputeShader();

					// Contour
					if (simulationData->timer % 4 == 2)
					{
						CmdWaitForPreviousComputeShader();

						_shaders.MarchingSquareAlgorithm->Update();

						_shaders.MarchingSquareAlgorithm->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

						_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(((pixelGrid.SimulationWidth + pixelGrid.SimulationHeight) * 3) - 2, 64), 1, 1);
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

					_shaders.IdleSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroups, 1, 1);

					_commandBuffer.GetVkCommandBuffer().end();
				}

				vk::SubmitInfo submitInfo;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers    = &_commandBuffer.GetVkCommandBuffer();

				computeQueue.GetVkQueue().submit(submitInfo, _computeFence);

				_fif = (fif + 1) % Rendering::Vulkan::Device::MAX_FRAMES_IN_FLIGHT;
			}
		}
	}
}
