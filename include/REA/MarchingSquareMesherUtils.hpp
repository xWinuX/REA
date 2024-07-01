#pragma once

#include <CDTUtils.h>
#include <stack>
#include <Triangulation.h>
#include <vector>
#include <box2d/b2_collision.h>

namespace REA
{
	class MarchingSquareMesherUtils
	{
		public:
			struct Polyline
			{
				b2AABB                       AABB;
				std::vector<CDT::V2d<float>> Vertices;
			};

		public:
			/**
			 * Find all seperate polylines and sorts them so they can be traversed without a gap
			 * @param vertices A span of vertices that SHOULD NOT contain any duplicates (use function RemoveDuplicatesAndRemapEdges in this class)
			 * @param edges A vector of edges (make sure the edges are remapped when deleting duplicates))
			 * @return A vector of polylines
			 */
			static std::vector<Polyline> SeperateAndSortPolylines(std::span<CDT::V2d<float>>& vertices, std::vector<CDT::Edge>& edges);

			/**
			 * Simplifies a polyline with the Douglas-Peucker Algorith
			 * Source: Partially written by ChatGPT because I'm bad at reading math expressions
			 * @param polyline Input polyline
			 * @param threshold Treshold for simplification (higher means less vertices)
			 * @return New Polyline with simplified vertices (the AABB will stay the same)
			 */
			static Polyline SimplifyPolylines(const Polyline& polyline, float threshold);

			/**
			 * Removes duplicates and remaps edges inplace (Use this before sending data to SeperateAndSortPolylines)
			 * @param vertices span of vertices
			 * @param edges vector of edges (typically just numbers starting at 0 and counting upwards in single steps)
			 */
			static void RemoveDuplicatesAndRemapEdges(std::span<CDT::V2d<float>>& vertices, std::vector<CDT::Edge>& edges);

			static CDT::Triangulation<float> GenerateTriangulation(const Polyline& polyline);

		private:
			static std::vector<std::vector<uint32_t>> _adjacencyList;
	};
}
