#include <glm/gtx/hash.hpp>

#include "REA/System/PixelGridSimulation.hpp"

#include <vector>
#include <iostream>
#include <execution>
#include <imgui.h>
#include <glm/gtc/random.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Debug/Performance.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>
#include <SplitEngine/Rendering/Vulkan/Device.hpp>
#include <utility>
#include <box2d/b2_polygon_shape.h>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Rendering/Material.hpp>


#include "IconsFontAwesome.h"
#include "REA/PixelType.hpp"
#include "REA/Stage.hpp"
#include "REA/Context/ImGui.hpp"

#include "CDT.h"
#include "REA/Assets.hpp"
#include "REA/MarchingSquareMesherUtils.hpp"
#include "REA/Component/PixelGridRigidBody.hpp"
#include "REA/Component/Transform.hpp"

namespace REA::System
{
	/*
	 * Meshing / Rigidbody flow
	 * If there's stuff to mesh that isn't "static" terrain save max/min position of it
	 * Use this max/min position to run CCL (need to pass in width, height and offset so indices can be calculated correctly)
	 * Allocate rigidbody gpu object with seed point to get id from
	 * Run Compute shader that removes pixels from main grid and puts them into the rigidbody Data (use Translation table so )
	 */


	PixelGridSimulation::PixelGridSimulation(const SimulationShaders& simulationShaders):
		_shaders(simulationShaders)
	{
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

		Rendering::Vulkan::Buffer& writeBuffer = properties.GetBuffer(2);

		for (int i = 0; i < device->MAX_FRAMES_IN_FLIGHT; ++i)
		{
			uint32_t fifIndex    = (i + 1) % device->MAX_FRAMES_IN_FLIGHT;
			auto&    bufferInfos = properties.GetBufferInfo(2, fifIndex);
			properties.SetBuffer(1, writeBuffer, bufferInfos.offset, bufferInfos.range, i);
		}

		//	properties.OverrideBufferPtrs(1, writeBuffer);
		_shaders.IdleSimulation->Update();
		_shaders.AccumulateSimulation->Update();
		_shaders.AccumulateSimulation->Update();

		vk::FenceCreateInfo fenceCreateInfo = vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
		_computeFence                       = _shaders.FallingSimulation->GetPipeline().GetDevice()->GetVkDevice().createFence(fenceCreateInfo);
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
		                                                    1,
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

		for (int i = 0; i < entities.size(); ++i)
		{
			SSBO_SimulationData* simulationData = _shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_SimulationData>(0);

			Component::PixelGrid& pixelGrid = pixelGrids[i];

			if (stage == Stage::GridComputeEnd)
			{
				//BENCHMARK_BEGIN
				device.GetVkDevice().waitForFences(_computeFence, vk::True, UINT64_MAX);
				device.GetVkDevice().resetFences(_computeFence);
				//				BENCHMARK_END("compute fence")

				pixelGrid.PixelState = _shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_Pixels>(1, _fif)->Pixels;
			}

			if (stage == Stage::GridComputeHalted)
			{
				Rendering::Vulkan::Buffer& buffer        = _shaders.MarchingSquareAlgorithm->GetProperties().GetBuffer(4);
				SSBO_MarchingCubes*        marchingCubes = _shaders.MarchingSquareAlgorithm->GetProperties().GetBufferData<SSBO_MarchingCubes>(4, _fif);

				/*
				LineSegment* lineSegment = reinterpret_cast<LineSegment*>(&marchingCubes->segments[0]);

				BENCHMARK_BEGIN
				std::sort(lineSegment,
						  lineSegment + marchingCubes->numSegments,
						  [](const LineSegment& lineSegment1, const LineSegment& lineSegment2)
						  {
							  return (lineSegment1.startPoint.x + lineSegment1.startPoint.y + lineSegment1.endPoint.x + lineSegment1.endPoint.y) > (
										 lineSegment2.startPoint.x + lineSegment2.startPoint.y + lineSegment2.endPoint.x + lineSegment2.endPoint.y);
						  });
				BENCHMARK_END("sort lines")*/

				std::vector<CDT::Edge> edges = std::vector<CDT::Edge>(marchingCubes->numSegments, CDT::Edge(0, 0));
				for (int i = 0; i < marchingCubes->numSegments; ++i) { edges[i] = CDT::Edge(i * 2, (i * 2) + 1); }

				std::span<CDT::V2d<float>> segmentsSpan = std::span(reinterpret_cast<CDT::V2d<float>*>(&marchingCubes->segments[0]), marchingCubes->numSegments * 2);

				MarchingSquareMesherUtils::RemoveDuplicatesAndRemapEdges(segmentsSpan, edges);

				std::vector<MarchingSquareMesherUtils::Polyline> polylines = MarchingSquareMesherUtils::SeperateAndSortPolylines(segmentsSpan, edges);

				LOG("num polylines {0}", polylines.size());

				_cclRange = { { 0, 0 }, { 0, 0 } };
				for (MarchingSquareMesherUtils::Polyline& polyline: polylines)
				{
					// Line
					glm::vec2 start = { polyline.Vertices[0].x, polyline.Vertices[0].y };
					glm::vec2 end   = { polyline.Vertices[1].x, polyline.Vertices[1].y };

					glm::vec2  direction = end - start;
					glm::ivec2 left      = { -direction.y, direction.x };

					glm::uvec2 seedPoint = glm::ivec2(glm::round(polyline.Vertices[0].x), glm::round(polyline.Vertices[0].y)) + left;

					size_t index = (seedPoint.y * pixelGrid.Width + seedPoint.x);

					if (!pixelGrid.PixelState[index].Flags.Has(Pixel::Flags::Static) && pixelGrid.PixelState[index].RigidBodyID == 4095)
					{
						// Calculate size needed
						b2AABB     b2Aabb   = polyline.AABB;
						b2Vec2     extends  = b2Aabb.GetExtents();
						glm::uvec2 aabbSize = { glm::round(extends.x * 2), glm::round(extends.y * 2) };
						size_t     dataSize = aabbSize.x * aabbSize.y;

						_cclRange = b2AABB({ glm::min(_cclRange.lowerBound.x, b2Aabb.lowerBound.x), glm::min(_cclRange.lowerBound.y, b2Aabb.lowerBound.y) },
						                   { glm::max(_cclRange.upperBound.x, b2Aabb.upperBound.x), glm::max(_cclRange.upperBound.y, b2Aabb.upperBound.y) });

						Component::Transform          transform;
						Component::PixelGridRigidBody pixelGridRigidBody;
						Component::Collider           collider;


						// Setup Transform
						b2Vec2 center      = b2Aabb.GetCenter();
						transform.Position = { center.x, center.y, 0.0f };

						// Setup PixelGridRigidBody
						uint32_t shaderID = -1u;
						if (_availableRigidBodyIDs.IsEmpty())
						{
							shaderID = _rigidBodyIDCounter++;
							LOG("counter: {0}", _rigidBodyIDCounter);
							_rigidBodyEntities.resize(_rigidBodyIDCounter);
						}
						else { shaderID = _availableRigidBodyIDs.Pop(); }

						pixelGridRigidBody.ShaderID     = shaderID;
						pixelGridRigidBody.AllocationID = _rigidBodyDataHeap.Allocate(dataSize);
						pixelGridRigidBody.DataIndex    = _rigidBodyDataHeap.GetAllocationInfo(pixelGridRigidBody.AllocationID).Offset;

						LOG("allocation size {0}", _rigidBodyDataHeap.GetAllocationInfo(pixelGridRigidBody.AllocationID).Size);
						LOG("allocation offset {0}", _rigidBodyDataHeap.GetAllocationInfo(pixelGridRigidBody.AllocationID).Offset);
						pixelGridRigidBody.Size = aabbSize;

						// Write RigidbodyInfo to shader
						simulationData->rigidBodies[shaderID].ID        = shaderID;
						simulationData->rigidBodies[shaderID].DataIndex = pixelGridRigidBody.DataIndex;
						simulationData->rigidBodies[shaderID].Size      = pixelGridRigidBody.Size;

						// Setup Collider
						collider.PhysicsMaterial = engineContext->Application->GetAssetDatabase().GetAsset<PhysicsMaterial>(Asset::PhysicsMaterial::Defaut);
						collider.InitialType     = b2_dynamicBody;

						// Simplify line
						polyline = MarchingSquareMesherUtils::SimplifyPolylines(polyline, _lineSimplificationTolerance);

						// Triangulate line
						CDT::Triangulation<float> cdt = MarchingSquareMesherUtils::GenerateTriangulation(polyline);


						b2Vec2     b2Center           = polyline.AABB.GetCenter();
						b2Vec2     b2BottomLeftCorner = b2Center - extends;
						glm::uvec2 bottomLeftCorner   = { b2BottomLeftCorner.x, b2BottomLeftCorner.y };


						// Create collider
						if (!cdt.vertices.empty())
						{
							std::vector<CDT::V2d<float>> v = std::vector<CDT::V2d<float>>(3);

							for (CDT::Triangle triangle: cdt.triangles)
							{
								v[0] = cdt.vertices[triangle.vertices[2]];
								v[1] = cdt.vertices[triangle.vertices[1]];
								v[2] = cdt.vertices[triangle.vertices[0]];

								v[0] = { v[0].x - b2Center.x, v[0].y - b2Center.y };
								v[1] = { v[1].x - b2Center.x, v[1].y - b2Center.y };
								v[2] = { v[2].x - b2Center.x, v[2].y - b2Center.y };

								LOG("collsion triangle");

								b2PolygonShape polygonShape;
								polygonShape.Set(reinterpret_cast<const b2Vec2*>(v.data()), v.size());

								collider.InitialShapes.push_back(polygonShape);
							}
						}

						_newRigidBodies.push_back({ bottomLeftCorner, aabbSize, seedPoint, shaderID });

						// Create entity
						uint64_t rigidBodyEntity = ecsRegistry.CreateEntity<Component::Transform, Component::PixelGridRigidBody, Component::Collider>(std::move(transform),
							std::move(pixelGridRigidBody),
							std::move(collider));

						_rigidBodyEntities[shaderID] = { rigidBodyEntity, false };

						LOG("Create entity");
					}
				}
			}

			if (stage == Stage::GridComputeBegin)
			{
				size_t numPixels     = pixelGrid.Width * pixelGrid.Height;
				size_t numWorkgroups = CeilDivide(numPixels, 64ull);

				uint32_t fif = _fif;

				simulationData->width  = pixelGrid.Width;
				simulationData->height = pixelGrid.Height;


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

					// Do CCL if there are rigidbodies to discover
					if (_cclRange.GetPerimeter() > 0.0f)
					{
						b2Vec2 extends            = _cclRange.GetExtents();
						b2Vec2 b2BottomLeftCorner = _cclRange.GetCenter() - extends;

						glm::uvec2 offset = { b2BottomLeftCorner.x, b2BottomLeftCorner.y };
						glm::uvec2 size   = { glm::round(extends.x * 2), glm::round(extends.y * 2) };

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

						_shaders.CCLRelabel->Update();

						_shaders.CCLRelabel->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

						_shaders.CCLRelabel->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &offset);
						_shaders.CCLRelabel->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &size);

						_commandBuffer.GetVkCommandBuffer().dispatch(numWorkGroupsForSinglePixelOps, 1, 1);

						CmdWaitForPreviousComputeShader();

						_shaders.CCLExtract->Update();

						_shaders.CCLExtract->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

						for (NewRigidBody newRigidBody: _newRigidBodies)
						{
							_shaders.CCLExtract->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &newRigidBody.Offset);
							_shaders.CCLExtract->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &newRigidBody.Size);
							_shaders.CCLExtract->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 2, &newRigidBody.SeedPoint);
							_shaders.CCLExtract->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 3, &newRigidBody.RigidBodyID);
							_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(newRigidBody.Size.x * newRigidBody.Size.y, 64u), 1, 1);
						}

						_newRigidBodies.clear();
						_cclRange = { { 0, 0 }, { 0, 0 } };

						CmdWaitForPreviousComputeShader();
					}

					// Rigidbody
					_shaders.RigidBodyRemove->Update();

					_shaders.RigidBodyRemove->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroups, 1, 1);

					CmdWaitForPreviousComputeShader();

					// Contour
					SSBO_MarchingCubes* marchingCubes = _shaders.MarchingSquareAlgorithm->GetProperties().GetBufferData<SSBO_MarchingCubes>(4, fif);
					marchingCubes->numSegments        = 0;

					_shaders.MarchingSquareAlgorithm->Update();

					_shaders.MarchingSquareAlgorithm->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(((simulationData->width + simulationData->height) * 3) - 2, 64u), 1, 1);

					CmdWaitForPreviousComputeShader();

					_shaders.RigidBodySimulation->Update();

					_shaders.RigidBodySimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					for (int shaderID = 0; shaderID < _rigidBodyEntities.size(); ++shaderID)
					{
						RigidbodyEntry& rigidBodyEntry = _rigidBodyEntities[shaderID];

						if (rigidBodyEntry.EntityID == -1u || !rigidBodyEntry.Active)
						{
							rigidBodyEntry.Active = true;
							continue;
						}

						Component::Transform&          transform          = ecsRegistry.GetComponent<Component::Transform>(rigidBodyEntry.EntityID);
						Component::PixelGridRigidBody& pixelGridRigidBody = ecsRegistry.GetComponent<Component::PixelGridRigidBody>(rigidBodyEntry.EntityID);

						simulationData->rigidBodies[shaderID].Position = transform.Position;
						simulationData->rigidBodies[shaderID].Rotation = transform.Rotation;

						_shaders.RigidBodySimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &pixelGridRigidBody.ShaderID);

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
						CmdWaitForPreviousComputeShader();

						_shaders.FallingSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &flowIteration);
						_shaders.FallingSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &margolusOffset);

						_shaders.FallingSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

						_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroupsMorgulus, 1, 1);

						fif = (fif + 1) % device.MAX_FRAMES_IN_FLIGHT;

						margolusOffset = GetMargolusOffset(timer + i);
						flowIteration++;
					}

					CmdWaitForPreviousComputeShader();

					// Temperature and Charge
					_shaders.AccumulateSimulation->Update();

					_shaders.AccumulateSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroups, 1, 1);

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

				_fif = (fif + 1) % device.MAX_FRAMES_IN_FLIGHT;
			}
		}
	}
}
