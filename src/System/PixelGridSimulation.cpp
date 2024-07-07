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
#include <SplitEngine/Rendering/Material.hpp>


#include "IconsFontAwesome.h"
#include "REA/PixelType.hpp"
#include "REA/Stage.hpp"
#include "REA/Context/ImGui.hpp"

#include "CDT.h"
#include "REA/MarchingSquareMesherUtils.hpp"

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
		for (int i = 0; i < entities.size(); ++i)
		{
			Rendering::Renderer* renderer = contextProvider.GetContext<RenderingContext>()->Renderer;

			Rendering::Vulkan::Device&     device       = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice();
			Rendering::Vulkan::QueueFamily computeQueue = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetQueueFamily(Rendering::Vulkan::QueueType::Compute);

			Component::PixelGrid& pixelGrid = pixelGrids[i];
			Component::Collider&  collider  = colliders[i];

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
				if (_doStep)
				{
					size_t                    size        = pixelGrid.Width * pixelGrid.Height;
					std::vector<Pixel::State> pixelStates = std::vector<Pixel::State>(1'000'000);

					memcpy(pixelStates.data(), pixelGrid.PixelState, size * sizeof(Pixel::State));

					Rendering::Vulkan::Buffer& buffer        = _shaders.MarchingSquareAlgorithm->GetProperties().GetBuffer(3);
					SSBO_MarchingCubes*        marchingCubes = _shaders.MarchingSquareAlgorithm->GetProperties().GetBufferData<SSBO_MarchingCubes>(3, _fif);

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


					std::vector<std::vector<Pixel::State>> pixelsInContour = std::vector<std::vector<Pixel::State>>(polylines.size());

					BENCHMARK_BEGIN
						for (int i = 0; i < polylines.size(); ++i)
						{
							std::vector<Pixel::State>&           pixels   = pixelsInContour[i];
							MarchingSquareMesherUtils::Polyline& polyline = polylines[i];
							std::vector<bool>                    visited(pixelGrid.Width * pixelGrid.Height, false);

							std::queue<size_t> nextIndices;

							b2Vec2     extends  = polyline.AABB.GetExtents();
							glm::uvec2 aabbSize = { glm::round(extends.x * 2), glm::round(extends.y * 2) };

							pixels.resize((aabbSize.x * aabbSize.y) + 1);

							b2Vec2     b2BottomLeftCorner = polyline.AABB.GetCenter() - extends;
							glm::uvec2 bottomLeftCorner   = { b2BottomLeftCorner.x, b2BottomLeftCorner.y };

							// Line
							glm::vec2 start = { polyline.Vertices[0].x, polyline.Vertices[0].y };
							glm::vec2 end   = { polyline.Vertices[1].x, polyline.Vertices[1].y };

							glm::vec2  direction = end - start;
							glm::ivec2 left      = { -direction.y, direction.x };

							glm::uvec2 seedPoint = glm::ivec2(glm::round(polyline.Vertices[0].x), glm::round(polyline.Vertices[0].y)) + left;

							size_t index = (seedPoint.y * pixelGrid.Width + seedPoint.x);
							nextIndices.push(index);

							while (!nextIndices.empty())
							{
								size_t currentIndex = nextIndices.front();
								nextIndices.pop();

								if (visited[currentIndex]) { continue; }

								visited[currentIndex] = true;

								uint32_t x = currentIndex % pixelGrid.Width;
								uint32_t y = currentIndex / pixelGrid.Width;

								//size_t insertionIndex  = (y - bottomLeftCorner.y) * aabbSize.x + (x - bottomLeftCorner.x);
								//pixels[insertionIndex] = pixelGrid.PixelState[currentIndex];
								//
								//pixelGrid.PixelState[currentIndex] = pixelGrid.PixelLookup[0].PixelState;


								uint32_t topY    = (y + 1) % pixelGrid.Height;
								uint32_t bottomY = (y == 0) ? pixelGrid.Height - 1 : y - 1;

								uint32_t rightX = (x + 1) % pixelGrid.Width;
								uint32_t leftX  = (x == 0) ? pixelGrid.Width - 1 : x - 1;

								// Calculate wrapped indices
								uint32_t topCenterIndex    = topY * pixelGrid.Width + x;
								uint32_t middleLeftIndex   = y * pixelGrid.Width + leftX;
								uint32_t middleRightIndex  = y * pixelGrid.Width + rightX;
								uint32_t bottomCenterIndex = bottomY * pixelGrid.Width + x;

								if (!visited[middleRightIndex] && pixelStates[middleRightIndex].PixelID > 2) { nextIndices.push(middleRightIndex); }

								if (!visited[topCenterIndex] && pixelStates[topCenterIndex].PixelID > 2) { nextIndices.push(topCenterIndex); }

								if (!visited[middleLeftIndex] && pixelStates[middleLeftIndex].PixelID > 2) { nextIndices.push(middleLeftIndex); }

								if (!visited[bottomCenterIndex] && pixelStates[bottomCenterIndex].PixelID > 2) { nextIndices.push(bottomCenterIndex); }
							}
						}

						//	memcpy(pixelGrid.PixelState, pixelStates.data(), 1'000'000 * sizeof(Pixel::State));

					BENCHMARK_END("Pixel Extraction")


					for (MarchingSquareMesherUtils::Polyline& polyline: polylines)
					{
						polyline = MarchingSquareMesherUtils::SimplifyPolylines(polyline, _lineSimplificationTolerance);
					}

					if (!polylines.empty())
					{
						_vertexBuffer.CopyData(polylines[0].Vertices.data(), polylines[0].Vertices.size() * sizeof(CDT::V2d<float>));
						MarchingSquareMesherUtils::GenerateTriangulation(polylines[0]);
					}

					/*
					if (!cdt.vertices.empty())
					{
						if (collider.Body != nullptr)
						{
							for (b2Fixture* staticFixture: _staticFixtures) { collider.Body->DestroyFixture(staticFixture); }
							_staticFixtures.clear();

							b2FixtureDef                 fixtureDef = collider.PhysicsMaterial->GetFixtureDefCopy();
							std::vector<CDT::V2d<float>> v          = std::vector<CDT::V2d<float>>(3);

							for (CDT::Triangle triangle: cdt.triangles)
							{
								v[0] = cdt.vertices[triangle.vertices[2]];
								v[1] = cdt.vertices[triangle.vertices[1]];
								v[2] = cdt.vertices[triangle.vertices[0]];

								b2PolygonShape polygonShape;
								polygonShape.Set(reinterpret_cast<const b2Vec2*>(v.data()), v.size());
								fixtureDef.shape = &polygonShape;
								_staticFixtures.push_back(collider.Body->CreateFixture(&fixtureDef));
							}
						}
					}*/
					//BENCHMARK_END("Pixel Extraction")
				}
			}

			if (stage == Stage::GridComputeBegin)
			{
				size_t numPixels     = pixelGrid.Width * pixelGrid.Height;
				size_t numWorkgroups = CeilDivide(numPixels, 64ull);

				uint32_t fif = _fif;

				UBO_SimulationData* simulationData = _shaders.FallingSimulation->GetProperties().GetBufferData<UBO_SimulationData>(0);
				simulationData->width              = pixelGrid.Width;
				simulationData->height             = pixelGrid.Height;


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

					// Fall
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

					_shaders.AccumulateSimulation->Update();

					_shaders.AccumulateSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroups, 1, 1);

					CmdWaitForPreviousComputeShader();

					SSBO_MarchingCubes* marchingCubes = _shaders.MarchingSquareAlgorithm->GetProperties().GetBufferData<SSBO_MarchingCubes>(3, fif);
					marchingCubes->numSegments        = 0;

					_shaders.MarchingSquareAlgorithm->Update();

					_shaders.MarchingSquareAlgorithm->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(((simulationData->width + simulationData->height) * 3) - 2, 64u), 1, 1);

					_commandBuffer.GetVkCommandBuffer().end();

					_doStep = false;
				}
				else
				{
					_commandBuffer.GetVkCommandBuffer().reset({});

					constexpr vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo({}, nullptr);

					_commandBuffer.GetVkCommandBuffer().begin(commandBufferBeginInfo);


					_shaders.CCLColumn->Update();

					_shaders.CCLColumn->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(pixelGrid.Width, 64), 1, 1);

					uint32_t stepIndex = 0;
					uint32_t n = pixelGrid.Width >> 1;
					while (n != 0)
					{
						CmdWaitForPreviousComputeShader();

						_shaders.CCLMerge->Update();

						_shaders.CCLMerge->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

						_shaders.CCLMerge->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &stepIndex);

						_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(n, 64u), 1, 1);

						n = n >> 1;
						stepIndex++;
					}

					CmdWaitForPreviousComputeShader();

					_shaders.CCLRelabel->Update();

					_shaders.CCLRelabel->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroups, 1, 1);

					_commandBuffer.GetVkCommandBuffer().end();
				}

				vk::SubmitInfo submitInfo;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers    = &_commandBuffer.GetVkCommandBuffer();

				computeQueue.GetVkQueue().submit(submitInfo, _computeFence);

				_fif = (fif + 1) % device.MAX_FRAMES_IN_FLIGHT;
			}


			/*if (stage == Stage::Rendering)
			{
				vk::CommandBuffer commandBuffer = contextProvider.GetContext<RenderingContext>()->Renderer->GetCommandBuffer().GetVkCommandBuffer();

				DebugMaterial->GetShader()->BindGlobal(commandBuffer);

				DebugMaterial->GetShader()->Update();

				DebugMaterial->GetShader()->Bind(commandBuffer);

				DebugMaterial->Update();

				DebugMaterial->Bind(commandBuffer);



				size_t j = 0;
				uint16_t* index = reinterpret_cast<uint16_t*>(_vertexBuffer.GetMappedData<glm::vec2>() + _vertexBuffer.GetDataElementNum(0));
				for (CDT::Triangle triangle: cdt.triangles)
				{
					index[j]     = triangle.vertices[0];
					index[j + 1] = triangle.vertices[1];
					index[j + 2] = triangle.vertices[2];
					j += 3;
				}

				size_t offset = 0;
				size_t size   = polylines.empty() ? 0 : polylines[0].Vertices.size();
				commandBuffer.bindVertexBuffers(0, _vertexBuffer.GetVkBuffer(), offset);
				//commandBuffer.bindIndexBuffer(_vertexBuffer.GetVkBuffer(), _vertexBuffer.GetSizeInBytes(0), vk::IndexType::eUint16);
				//commandBuffer.drawIndexed(j, 1, 0, 0, 0);
				commandBuffer.draw(size, 1, 0, 0);
			}*/
		}
	}
}
