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

namespace REA::System
{
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

			ImGui::SliderFloat("Line Simplification Tolerance", &_lineSimplificationTolerance, 0.0f, 10.0f);
			ImGui::End();
		}

		LOG("num pixelgrid archetypes {0}", archetypes.size());

		System<Component::PixelGrid>::ExecuteArchetypes(archetypes, contextProvider, stage);
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

	void dfs(const std::vector<std::vector<uint32_t>>& adjacencyList, uint32_t nodeIndex, std::unordered_set<uint32_t>& visited, std::vector<uint32_t>& component)
	{
		std::stack<uint32_t> stack;
		stack.push(nodeIndex);

		while (!stack.empty())
		{
			uint32_t currentNode = stack.top();
			stack.pop();

			if (visited.contains(currentNode)) continue;

			visited.insert(currentNode);
			component.push_back(currentNode);

			for (const uint32_t neighborIndex: adjacencyList[currentNode]) { if (!visited.contains(neighborIndex)) { stack.push(neighborIndex); } }
		}
	}

	void buildAdjacencyList(std::vector<std::vector<uint32_t>>& adjacencyList, const std::vector<CDT::Edge>& edges)
	{
		for (const auto& edge: edges)
		{
			adjacencyList[edge.v1()].push_back(edge.v2());
			adjacencyList[edge.v2()].push_back(edge.v1());
		}
	}

	std::vector<std::vector<uint32_t>> findConnectedComponents(const std::vector<std::vector<uint32_t>>& adjacencyList)
	{
		std::unordered_set<uint32_t>       visited;
		std::vector<std::vector<uint32_t>> components;

		for (uint32_t vertexID = 0; vertexID < adjacencyList.size(); ++vertexID)
		{
			if (!visited.contains(vertexID))
			{
				std::vector<uint32_t> component;
				dfs(adjacencyList, vertexID, visited, component);
				components.push_back(std::move(component));
			}
		}

		return components;
	}

	std::vector<CDT::V2d<float>> sortPolyline(std::vector<std::vector<uint32_t>>& adjacencyList, const std::vector<uint32_t>& polylineIndices, std::span<CDT::V2d<float>>& vertices)
	{
		std::vector<CDT::V2d<float>> sortedPolyline;

		if (polylineIndices.empty()) { return sortedPolyline; }

		std::unordered_set<uint32_t> visited;
		uint32_t                     currentIndex = polylineIndices[0];
		CDT::V2d<float>              current      = vertices[currentIndex];
		visited.insert(currentIndex);

		sortedPolyline.push_back(current);
		while (sortedPolyline.size() < polylineIndices.size() - 1)
		{
			bool found = false;
			for (uint32_t neighbor: adjacencyList[currentIndex])
			{
				if (visited.contains(neighbor)) continue;
				sortedPolyline.push_back(vertices[neighbor]);
				visited.insert(neighbor);
				currentIndex = neighbor;
				current      = vertices[currentIndex];
				found        = true;
				break;
			}
			if (!found) break;
		}

		return sortedPolyline;
	}

	std::vector<std::vector<CDT::V2d<float>>> separateAndSortPolylines(std::vector<std::vector<uint32_t>>& adjacencyList,
	                                                                   std::span<CDT::V2d<float>>&         vertices,
	                                                                   std::vector<CDT::Edge>&             edges)
	{
		std::vector<std::vector<CDT::V2d<float>>> polylines;
		if (vertices.size() > 0 && edges.size() > 0)
		{
			//BENCHMARK_BEGIN
			buildAdjacencyList(adjacencyList, edges);
			//BENCHMARK_END("build adjacent")

			std::vector<std::vector<uint32_t>> components;
			//BENCHMARK_BEGIN
			components = findConnectedComponents(adjacencyList);
			//BENCHMARK_END("findConnected")

			//BENCHMARK_BEGIN
			for (const auto& component: components)
			{
				std::vector<CDT::V2d<float>> sortedPolyline = sortPolyline(adjacencyList, component, vertices);
				polylines.push_back(std::move(sortedPolyline));
			}
			//BENCHMARK_END("sort polyline")
		}
		return polylines;
	}


	// Function to calculate the perpendicular distance from a point to a line
	float perpendicularDistance(const CDT::V2d<float>& p, const CDT::V2d<float>& lineStart, const CDT::V2d<float>& lineEnd)
	{
		float dx     = lineEnd.x - lineStart.x;
		float dy     = lineEnd.y - lineStart.y;
		float area   = glm::abs(dx * (lineStart.y - p.y) - dy * (lineStart.x - p.x));
		float bottom = glm::sqrt(dx * dx + dy * dy);
		return area / bottom;
	}

	// Recursive function to simplify the line
	void douglasPeucker(const std::vector<CDT::V2d<float>>& points, float epsilon, std::vector<CDT::V2d<float>>& result)
	{
		if (points.size() < 2) { throw std::invalid_argument("Not enough points to simplify"); }

		std::vector<bool> include(points.size(), false);
		include[0] = include[points.size() - 1] = true;

		std::stack<std::pair<size_t, size_t>> toProcess;
		toProcess.emplace(0, points.size() - 1);

		while (!toProcess.empty())
		{
			auto [start, end] = toProcess.top();
			toProcess.pop();

			double maxDist = 0.0;
			size_t index   = start;

			for (size_t i = start + 1; i < end; ++i)
			{
				double dist = perpendicularDistance(points[i], points[start], points[end]);
				if (dist > maxDist)
				{
					index   = i;
					maxDist = dist;
				}
			}

			if (maxDist > epsilon)
			{
				include[index] = true;
				toProcess.emplace(start, index);
				toProcess.emplace(index, end);
			}
		}

		for (size_t i = 0; i < points.size(); ++i) { if (include[i]) { result.push_back(points[i]); } }
	}

	std::vector<CDT::V2d<float>> simplifyPolyline(const std::vector<CDT::V2d<float>>& points, float epsilon)
	{
		std::vector<CDT::V2d<float>> result;
		douglasPeucker(points, epsilon, result);
		return result;
	}

	std::vector<std::vector<uint32_t>> _adjacencyList;


	void PixelGridSimulation::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		LOG("entiteis {0}", entities.size());
		for (int i = 0; i < entities.size(); ++i)
		{
			Rendering::Renderer* renderer = contextProvider.GetContext<RenderingContext>()->Renderer;

			Rendering::Vulkan::Device&     device       = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice();
			Rendering::Vulkan::QueueFamily computeQueue = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetQueueFamily(Rendering::Vulkan::QueueType::Compute);

			Component::PixelGrid& pixelGrid = pixelGrids[i];

			if (stage == Stage::GridComputeEnd)
			{
				//BENCHMARK_BEGIN
				device.GetVkDevice().waitForFences(_computeFence, vk::True, UINT64_MAX);
				device.GetVkDevice().resetFences(_computeFence);
				//				BENCHMARK_END("compute fence")

				pixelGrid.PixelState = _shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_Pixels>(1, _fif)->Pixels;
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

					_shaders.IdleSimulation->Update();

					_shaders.IdleSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(numPixels, 64ull), 1, 1);

					CmdWaitForPreviousComputeShader();

					SSBO_MarchingCubes* marchingCubes = _shaders.MarchingSquareAlgorithm->GetProperties().GetBufferData<SSBO_MarchingCubes>(3, fif);
					marchingCubes->numSegments        = 0;

					_shaders.MarchingSquareAlgorithm->Update();

					_shaders.MarchingSquareAlgorithm->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

					_commandBuffer.GetVkCommandBuffer().dispatch(CeilDivide(((simulationData->width + simulationData->height) * 3) - 2, 64u), 1, 1);

					_commandBuffer.GetVkCommandBuffer().end();
				}

				vk::SubmitInfo submitInfo;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers    = &_commandBuffer.GetVkCommandBuffer();

				computeQueue.GetVkQueue().submit(submitInfo, _computeFence);

				_fif = (fif + 1) % device.MAX_FRAMES_IN_FLIGHT;
			}


			if (stage == Stage::Rendering)
			{
				vk::CommandBuffer commandBuffer = contextProvider.GetContext<RenderingContext>()->Renderer->GetCommandBuffer().GetVkCommandBuffer();

				DebugMaterial->GetShader()->BindGlobal(commandBuffer);

				DebugMaterial->GetShader()->Update();

				DebugMaterial->GetShader()->Bind(commandBuffer);

				DebugMaterial->Update();

				DebugMaterial->Bind(commandBuffer);

				Rendering::Vulkan::Buffer& buffer = _shaders.MarchingSquareAlgorithm->GetProperties().GetBuffer(3);
				LOG("fif {0}", _fif);
				SSBO_MarchingCubes* marchingCubes = _shaders.MarchingSquareAlgorithm->GetProperties().GetBufferData<SSBO_MarchingCubes>(3, _fif);


				//BENCHMARK_BEGIN
				std::vector<CDT::V2d<float>> vertices = std::vector<CDT::V2d<float>>(marchingCubes->numSegments * 2);
				std::vector<CDT::Edge>       edges    = std::vector<CDT::Edge>(marchingCubes->numSegments, CDT::Edge(0, 0));

				BENCHMARK_BEGIN
					memcpy(vertices.data(), marchingCubes->segments, marchingCubes->numSegments * 2 * sizeof(CDT::V2d<float>));
					for (int i = 0; i < marchingCubes->numSegments; ++i) { edges[i] = CDT::Edge(i * 2, (i * 2) + 1); }
				BENCHMARK_END("create edge buffer")

				//LineSegment* lineSegments = reinterpret_cast<LineSegment*>(&marchingCubes->segments);
				//std::vector<std::vector<LineSegment>> polygons;

				std::span<CDT::V2d<float>> segmentsSpan;

				if (marchingCubes->numSegments > 0)
				{
					BENCHMARK_BEGIN

						glm::vec2*                segmentBegin = &marchingCubes->segments[0];
						glm::vec2*                segmentEnd   = &marchingCubes->segments[marchingCubes->numSegments * 2];
						const CDT::DuplicatesInfo di           = CDT::FindDuplicates<float>(segmentBegin,
						                                                                    segmentEnd,
						                                                                    [](const glm::vec2& vert) { return vert.x; },
						                                                                    [](const glm::vec2& vert) { return vert.y; });


						glm::vec2* newEnd = remove_at(segmentBegin, segmentEnd, di.duplicates.begin(), di.duplicates.end());

						size_t numVertices = std::distance(segmentBegin, newEnd);

						//segmentsSpan = std::span<CDT::V2d<float>>(reinterpret_cast<CDT::V2d<float>*>(segmentBegin), numVertices);

						//CDT::RemapEdges(edges, di.mapping);

						CDT::RemoveDuplicatesAndRemapEdges(vertices, edges);

					BENCHMARK_END("remove duplicated and remap edges")
				}

				segmentsSpan = std::span(vertices);

				LOG("Previous size {0} New Size {1}", marchingCubes->numSegments*2, segmentsSpan.size());

				std::vector<std::vector<CDT::V2d<float>>> polyLines;
				_adjacencyList.resize(segmentsSpan.size());
				//BENCHMARK_BEGIN
				polyLines = separateAndSortPolylines(_adjacencyList, segmentsSpan, edges);
				//BENCHMARK_END("Polyline generation")
				for (std::vector<uint32_t>& adjacencyList: _adjacencyList) { adjacencyList.clear(); }

				//BENCHMARK_BEGIN
				//for (std::vector<CDT::V2d<float>>& polyLine: polyLines) { polyLine = simplifyPolyline(polyLine, _lineSimplificationTolerance); }
				//BENCHMARK_END("Line simplification")


				LOG("num polylines {0}", polyLines.size());

				/*LOG("-- Vertices:");
				int i = 0;
				for (CDT::V2d<float> vertex : vertices)
				{
					LOG("[{0}] vertex x: {1} y: {2}", i, vertex.x, vertex.y);
					i++;
				}

				LOG("-- Edges:");
				i = 0;
				for (CDT::Edge& edge : edges)
				{
					LOG("[{0}] edge start: {1} end: {2}", i,  edge.v1(), edge.v2());
					i++;
				}

				_adjacencyList.resize(vertices.size());
				buildAdjacencyList(_adjacencyList, edges);

				LOG("-- Adjency:");
				i = 0;
				for (std::vector<uint32_t> adjacencyList : _adjacencyList)
				{
					LOG("[{0}] adjency ({1})", i, adjacencyList.size());
					int j = 0;
					for (auto list: adjacencyList)
					{
						LOG(" [{0}] {1}", j, list);
						j++;
					}
					i++;
				}

				std::vector<std::vector<uint32_t>> connected = findConnectedComponents(_adjacencyList);

				LOG("num connected {0}", connected.size());*/


				//BENCHMARK_END("Remove duplicates")

				//LineSegment* lineSegment = reinterpret_cast<LineSegment*>(marchingCubes->segments);
				/*std::vector<std::vector<LineSegment>> polyLines;
				BENCHMARK_BEGIN
					polyLines = separateAndSortPolylines(_adjacencyList, vertices, edges);
				BENCHMARK_END("Polyline generation")



				LOG("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! num polylines {0}", polyLines.size());*/


				/*CDT::Triangulation<float> cdt = CDT::Triangulation<float>();

				BENCHMARK_BEGIN
					cdt.insertVertices(vertices);
					cdt.insertEdges(edges);
				BENCHMARK_END("insert vertices")

				BENCHMARK_BEGIN
					cdt.eraseOuterTrianglesAndHoles();
				BENCHMARK_END("erase super triangle")
	*/
				//BENCHMARK_BEGIN
				//_vertexBuffer.CopyData(cdt.vertices.data(), cdt.vertices.size() * sizeof(CDT::V2d<float>));
				//_vertexBuffer.CopyData(vertices.data(), vertices.size() * sizeof(CDT::V2d<float>));
				//_vertexBuffer.CopyData(marchingCubes->segments, marchingCubes->numSegments * 2 * sizeof(glm::vec2));
				//BENCHMARK_END("copy to vertex buffer")


				glm::vec2* vertex = _vertexBuffer.GetMappedData<glm::vec2>();

				size_t numPoints = 0;
				for (std::vector<CDT::V2d<float>>& points: polyLines)
				{
					size_t size = points.size() * sizeof(CDT::V2d<float>);
					memcpy(vertex + numPoints, points.data(), size);
					numPoints += points.size();
				}


				/*uint16_t* index = reinterpret_cast<uint16_t*>(_vertexBuffer.GetMappedData<glm::vec2>() + _vertexBuffer.GetDataElementNum(0));


				size_t j = 0;
				for (CDT::Edge& edge: edges)
				{
					index[j]     = edge.v1();
					index[j + 1] = edge.v2();
					j += 2;
				}*/

				/*size_t j = 0;
				for (CDT::Triangle triangle: cdt.triangles)
				{
					index[j]     = triangle.vertices[0];
					index[j + 1] = triangle.vertices[1];
					index[j + 2] = triangle.vertices[2];
					j += 3;
				}*/


				size_t offset = 0;

				commandBuffer.bindVertexBuffers(0, _vertexBuffer.GetVkBuffer(), offset);
				//commandBuffer.bindIndexBuffer(_vertexBuffer.GetVkBuffer(), _vertexBuffer.GetSizeInBytes(0), vk::IndexType::eUint16);
				//commandBuffer.drawIndexed(edges.size() * 2, 1, 0, 0, 0);
				commandBuffer.draw(numPoints, 1, 0, 0);
			}
		}
	}
}
