#include "REA/MarchingSquareMesherUtils.hpp"

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <SplitEngine/ErrorHandler.hpp>
#include <SplitEngine/Debug/Log.hpp>

namespace REA
{
	std::vector<std::vector<uint32_t>> MarchingSquareMesherUtils::_adjacencyList{};

	MarchingSquareMesherUtils::Polyline MarchingSquareMesherUtils::SimplifyPolylines(const Polyline& polyline, float threshold)
	{
		if (polyline.Vertices.size() < 2) { SplitEngine::ErrorHandler::ThrowRuntimeError("Not enough points to simplify"); }

		Polyline resultPolyline = { polyline.AABB, {} };

		std::vector<bool> include = std::vector<bool>(polyline.Vertices.size(), false);
		include[0]                = include[polyline.Vertices.size() - 1] = true;

		std::stack<std::pair<size_t, size_t>> toProcess{};
		toProcess.emplace(0, polyline.Vertices.size() - 1);

		while (!toProcess.empty())
		{
			auto [start, end] = toProcess.top();
			toProcess.pop();

			double maxDist = 0.0;
			size_t index   = start;

			for (size_t i = start + 1; i < end; ++i)
			{
				CDT::V2d<float> lineStart = polyline.Vertices[start];
				CDT::V2d<float> lineEnd   = polyline.Vertices[end];
				CDT::V2d<float> point     = polyline.Vertices[i];

				float dx     = lineEnd.x - lineStart.x;
				float dy     = lineEnd.y - lineStart.y;
				float area   = glm::abs(dx * (lineStart.y - point.y) - dy * (lineStart.x - point.x));
				float bottom = glm::sqrt(dx * dx + dy * dy);

				float dist = area / bottom;
				if (dist > maxDist)
				{
					index   = i;
					maxDist = dist;
				}
			}

			if (maxDist > threshold)
			{
				include[index] = true;
				toProcess.emplace(start, index);
				toProcess.emplace(index, end);
			}
		}

		for (size_t i = 0; i < polyline.Vertices.size(); ++i) { if (include[i]) { resultPolyline.Vertices.push_back(polyline.Vertices[i]); } }

		return resultPolyline;
	}


	std::vector<MarchingSquareMesherUtils::Polyline> MarchingSquareMesherUtils::SeperateAndSortPolylines(std::span<CDT::V2d<float>>& vertices, std::vector<CDT::Edge>& edges)
	{
		if (vertices.empty() || edges.empty()) { return {}; }

		std::vector<Polyline>        polylines;
		std::unordered_set<uint32_t> visited;

		// Build adjecency list
		_adjacencyList.resize(vertices.size());
		for (const auto& edge: edges)
		{
			_adjacencyList[edge.v1()].push_back(edge.v2());
			_adjacencyList[edge.v2()].push_back(edge.v1());
		}

		// Find connected components
		std::vector<std::vector<uint32_t>> separatePolylineIndices;
		for (uint32_t vertexID = 0; vertexID < _adjacencyList.size(); ++vertexID)
		{
			if (!visited.contains(vertexID))
			{
				std::vector<uint32_t> component;

				// DFS
				std::stack<uint32_t> stack;
				stack.push(vertexID);

				while (!stack.empty())
				{
					uint32_t currentNode = stack.top();
					stack.pop();

					if (visited.contains(currentNode)) { continue; }

					visited.insert(currentNode);
					component.push_back(currentNode);

					for (const uint32_t neighborIndex: _adjacencyList[currentNode]) { if (!visited.contains(neighborIndex)) { stack.push(neighborIndex); } }
				}

				separatePolylineIndices.push_back(std::move(component));
			}
		}

		// Build polylines
		for (const auto& polylineIndices: separatePolylineIndices)
		{
			visited.clear();

			std::vector<CDT::V2d<float>> sortedPolyline;

			if (polylineIndices.empty())
			{
				polylines.push_back({ {}, sortedPolyline });
				continue;
			}

			uint32_t        currentIndex = polylineIndices[0];
			CDT::V2d<float> current      = vertices[currentIndex];
			b2AABB          aabb         = b2AABB({ current.x, current.y }, { current.x, current.y });
			visited.insert(currentIndex);

			sortedPolyline.push_back(current);
			while (sortedPolyline.size() < polylineIndices.size())
			{
				bool found = false;
				for (uint32_t neighbor: _adjacencyList[currentIndex])
				{
					if (visited.contains(neighbor)) { continue; }

					CDT::V2d<float> vert = vertices[neighbor];

					aabb = b2AABB({ glm::min(aabb.lowerBound.x, vert.x), glm::min(aabb.lowerBound.y, vert.y) },
					              { glm::max(aabb.upperBound.x, vert.x), glm::max(aabb.upperBound.y, vert.y) });

					sortedPolyline.push_back(vert);
					visited.insert(neighbor);
					currentIndex = neighbor;
					found        = true;
					break;
				}

				if (!found) { break; }
			}

			auto b2center = aabb.GetCenter();
			for (CDT::V2d<float>& polylineVert: sortedPolyline)
			{
				polylineVert = { polylineVert.x - b2center.x, polylineVert.y - b2center.y };
				polylineVert = { polylineVert.x * 0.99f, polylineVert.y * 0.99f };
				polylineVert = { polylineVert.x + b2center.x, polylineVert.y + b2center.y };
			}

			polylines.push_back({ aabb, std::move(sortedPolyline) });
		}

		// Clear adjecency list
		for (std::vector<uint32_t>& adjacencyList: _adjacencyList) { adjacencyList.clear(); }

		return polylines;
	}

	CDT::DuplicatesInfo MarchingSquareMesherUtils::RemoveDuplicatesAndRemapEdges(std::span<CDT::V2d<float>>& vertices, std::vector<CDT::Edge>& edges)
	{
		if (vertices.empty() || edges.empty()) { return {}; }

		const CDT::DuplicatesInfo duplicatesInfo = RemoveDuplicates(vertices);

		CDT::RemapEdges(edges, duplicatesInfo.mapping);

		return duplicatesInfo;
	}

	CDT::DuplicatesInfo MarchingSquareMesherUtils::RemoveDuplicates(std::span<CDT::V2d<float>>& vertices)
	{
		if (vertices.empty()) { return {}; }

		CDT::DuplicatesInfo duplicatesInfo = CDT::FindDuplicates<float>(vertices.begin(),
		                                                                vertices.end(),
		                                                                [](const CDT::V2d<float>& vert) { return vert.x; },
		                                                                [](const CDT::V2d<float>& vert) { return vert.y; });

		const auto newEnd = remove_at(vertices.begin(), vertices.end(), duplicatesInfo.duplicates.begin(), duplicatesInfo.duplicates.end());

		size_t numVertices = std::distance(vertices.begin(), newEnd);

		vertices = std::span<CDT::V2d<float>>(vertices.begin(), numVertices);

		return duplicatesInfo;
	}

	CDT::Triangulation<float> MarchingSquareMesherUtils::GenerateTriangulation(const Polyline& polyline)
	{
		std::vector<CDT::Edge> edges{};

		for (int i = 0; i < polyline.Vertices.size(); ++i) { edges.emplace_back(i, (i + 1) % polyline.Vertices.size()); }

		CDT::Triangulation<float> cdt = CDT::Triangulation<float>(CDT::VertexInsertionOrder::Auto, CDT::IntersectingConstraintEdges::TryResolve, 0.1f);
		cdt.insertVertices(polyline.Vertices);
		cdt.insertEdges(edges);
		cdt.eraseOuterTriangles();
		return cdt;
	}
}
