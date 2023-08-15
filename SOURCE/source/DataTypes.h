#pragma once
#include <cassert>

#include "Math.h"
#include "vector"

namespace dae
{
#pragma region GEOMETRY
	struct Sphere
	{
		Vector3 origin{};
		float radius{};

		unsigned char materialIndex{ 0 };
	};

	struct Plane
	{
		Vector3 origin{};
		Vector3 normal{};

		unsigned char materialIndex{ 0 };
	};

	struct BVHNode
	{
		// The bounding box for this node
		Vector3 aabbMin, aabbMax;
		// The left node of this node (right node = left node + 1)
		unsigned int leftNode;
		// Storing the index to the triangle vector + the number of triangles in the node
		unsigned int firstTriIdx, triCount;
		bool IsLeaf() const { return triCount > 0; }

	};

	enum class TriangleCullMode
	{
		FrontFaceCulling,
		BackFaceCulling,
		NoCulling
	};

	struct Triangle
	{
		Triangle() = default;
		Triangle(const Vector3& _v0, const Vector3& _v1, const Vector3& _v2, const Vector3& _normal) :
			v0{ _v0 }, v1{ _v1 }, v2{ _v2 }, normal{ _normal.Normalized() }
		{
			centroid = (v0 + v1 + v2) * 0.3333f;
		}

		Triangle(const Vector3& _v0, const Vector3& _v1, const Vector3& _v2) :
			v0{ _v0 }, v1{ _v1 }, v2{ _v2 }
		{
			const Vector3 edgeV0V1 = v1 - v0;
			const Vector3 edgeV0V2 = v2 - v0;
			normal = Vector3::Cross(edgeV0V1, edgeV0V2).Normalized();

			centroid = (v0 + v1 + v2) * 0.3333f;
		}

		Vector3 v0{};
		Vector3 v1{};
		Vector3 v2{};

		Vector3 normal{};
		Vector3 centroid{};


		TriangleCullMode cullMode{};
		unsigned char materialIndex{};
	};

	struct TriangleMesh
	{
		TriangleMesh() = default;
		TriangleMesh(const std::vector<Vector3>& _positions, const std::vector<int>& _indices, TriangleCullMode _cullMode) :
			positions(_positions), indices(_indices), cullMode(_cullMode)
		{

			//Calculate Normals
			CalculateNormals();

			//Update Transforms
			UpdateTransforms();


			//Fill the triangle list
			FillTriangleList();
		}

		TriangleMesh(const std::vector<Vector3>& _positions, const std::vector<int>& _indices, const std::vector<Vector3>& _normals, TriangleCullMode _cullMode) :
			positions(_positions), indices(_indices), normals(_normals), cullMode(_cullMode)
		{

			UpdateTransforms();

			//Fill the triangle list
			FillTriangleList();
		}

		std::vector<Vector3> positions{};
		std::vector<Vector3> normals{};
		std::vector<int> indices{};

		std::vector<Triangle> triangles{};
		std::vector<int> triIdx{};

		unsigned char materialIndex{};

		TriangleCullMode cullMode{ TriangleCullMode::BackFaceCulling };

		Matrix rotationTransform{};
		Matrix translationTransform{};
		Matrix scaleTransform{};

		Vector3 minAABB;
		Vector3 maxAABB;

		Vector3 transformedMinAABB;
		Vector3 transformedMaxAABB;

		std::vector<Vector3> transformedPositions{};
		std::vector<Vector3> transformedNormals{};

		std::vector<BVHNode> bvhNodePool;
		unsigned int rootNodeIdx{ 0 };
		unsigned int nodesUsed{ 1 };


		void Translate(const Vector3& translation)
		{
			translationTransform = Matrix::CreateTranslation(translation);
		}

		void RotateY(float yaw)
		{
			rotationTransform = Matrix::CreateRotationY(yaw);
		}

		void Scale(const Vector3& scale)
		{
			scaleTransform = Matrix::CreateScale(scale);
		}

		void AppendTriangle(const Triangle& triangle, bool ignoreTransformUpdate = false)
		{
			int startIndex = static_cast<int>(positions.size());

			positions.push_back(triangle.v0);
			positions.push_back(triangle.v1);
			positions.push_back(triangle.v2);

			indices.push_back(startIndex);
			indices.push_back(++startIndex);
			indices.push_back(++startIndex);

			normals.push_back(triangle.normal);

			//Not ideal, but making sure all vertices are updated
			if (!ignoreTransformUpdate)
				UpdateTransforms();
		}

		void CalculateNormals()
		{
			for (size_t i{}; i <= indices.size() - 3; i += 3)
			{
				Vector3 v0{ positions[indices[i]] };
				Vector3 v1{ positions[indices[i + 1]] };
				Vector3 v2{ positions[indices[i + 2]] };


				Vector3 a{ v1 - v0 };
				Vector3 b{ v2 - v0 };
				Vector3 normal{ Vector3::Cross(a,b).Normalized() };


				normals.emplace_back(normal);
			}

		}

		void UpdateTransforms()
		{
			transformedPositions.clear();
			transformedNormals.clear();

			//Calculate Final Transform 
			const Matrix finalTransform = scaleTransform * rotationTransform * translationTransform;

			//Transform Positions (positions > transformedPositions)
			for (const auto& p : positions)
			{
				transformedPositions.emplace_back(finalTransform.TransformPoint(p));
			}
			UpdateTransformedAABB(finalTransform);
			//Transform Normals (normals > transformedNormals)
			for (const auto& n : normals)
			{
				transformedNormals.emplace_back(finalTransform.TransformVector(n));

			}


		}
		void FillTriangleList()
		{
			triangles.resize(indices.size() / 3);
			triIdx.resize(indices.size() / 3);
			int triCounter{};

			for (size_t i{}; i < indices.size(); i += 3)
			{
				//make triangle out of mesh indices
				Triangle tri = {
					transformedPositions[indices[i]],
					transformedPositions[indices[i + 1]],
					transformedPositions[indices[i + 2]],
					transformedNormals[triCounter]
				};

				tri.cullMode = cullMode;
				tri.materialIndex = materialIndex;


				triangles[triCounter] = tri;
				triIdx[triCounter] = triCounter;
				++triCounter;
			}
		}

		void UpdateTriangleList()
		{

			int triCounter{};
			for (size_t i{}; i < indices.size(); i += 3)
			{
				//make triangle out of mesh indices
				Triangle tri = {
					transformedPositions[indices[i]],
					transformedPositions[indices[i + 1]],
					transformedPositions[indices[i + 2]],
					transformedNormals[triCounter]
				};

				tri.cullMode = cullMode;
				tri.materialIndex = materialIndex;
				triangles[i / 3] = tri;
				++triCounter;
			}
			for (unsigned i{}; i < nodesUsed; ++i)
				UpdateNodeBounds(i);
		}
		void UpdateAABB()
		{
			if (!positions.empty())
			{
				minAABB = positions[0];
				maxAABB = positions[0];
				for (auto& p : positions)
				{
					minAABB = Vector3::Min(p, minAABB);
					maxAABB = Vector3::Max(p, maxAABB);
				}
			}
		}

		void UpdateTransformedAABB(const Matrix& finalTransform)
		{
			Vector3 tMinAABB = finalTransform.TransformPoint(minAABB);
			Vector3 tMaxAABB = tMinAABB;

			Vector3 tAABB = finalTransform.TransformPoint(maxAABB.x, minAABB.y, minAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);

			tAABB = finalTransform.TransformPoint(maxAABB.x, minAABB.y, maxAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);

			tAABB = finalTransform.TransformPoint(minAABB.x, minAABB.y, maxAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);

			tAABB = finalTransform.TransformPoint(minAABB.x, maxAABB.y, minAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);

			tAABB = finalTransform.TransformPoint(maxAABB.x, maxAABB.y, minAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);

			tAABB = finalTransform.TransformPoint(maxAABB.x, maxAABB.y, maxAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);

			tAABB = finalTransform.TransformPoint(minAABB.x, maxAABB.y, maxAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);

			transformedMinAABB = tMinAABB;
			transformedMaxAABB = tMaxAABB;

		}

		void BuildBVH()
		{
			const size_t numberOfTriangles{ indices.size() / 3 };
			bvhNodePool.resize(2 * numberOfTriangles - 1);


			// Get the root node out of the pool
			BVHNode& root = bvhNodePool[rootNodeIdx];
			// The node has no children yet
			root.leftNode = 0;
			// Assign all triangles to this node
			root.firstTriIdx = 0;
			root.triCount = static_cast<int>(numberOfTriangles);

			UpdateNodeBounds(rootNodeIdx);
			Subdivide(rootNodeIdx);
		}

		void UpdateNodeBounds(unsigned int nodeIdx)
		{
			BVHNode& node = bvhNodePool[nodeIdx];
			node.aabbMin = { INFINITY, INFINITY, INFINITY };
			node.aabbMax = { -INFINITY, -INFINITY, -INFINITY };

			// Loop over all the stored triangles in the node
			for (unsigned int first = node.firstTriIdx, i = 0; i < node.triCount; ++i)
			{

				unsigned leafTriIdx = triIdx[first + i];
				Triangle& leafTri = triangles[leafTriIdx];
				// Find the bounding box around the stored triangles
				node.aabbMin = Vector3::Min(node.aabbMin, leafTri.v0);
				node.aabbMin = Vector3::Min(node.aabbMin, leafTri.v1);
				node.aabbMin = Vector3::Min(node.aabbMin, leafTri.v2);
				node.aabbMax = Vector3::Max(node.aabbMax, leafTri.v0);
				node.aabbMax = Vector3::Max(node.aabbMax, leafTri.v1);
				node.aabbMax = Vector3::Max(node.aabbMax, leafTri.v2);
			}
		}

		void Subdivide(unsigned int nodeIdx)
		{
			// Terminate recursion when a node contains 2 or less triangles
			BVHNode& node = bvhNodePool[nodeIdx];
			if (node.triCount <= 2) return;

			// Determine the axis and position of the split plane
			Vector3 extent = node.aabbMax - node.aabbMin;
			int axis = 0;
			if (extent.y > extent.x) axis = 1;
			if (extent.z > extent[axis]) axis = 2;
			float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;

			// Split the group of primitives in two halves using the split plane
			int i = node.firstTriIdx;
			int j = i + node.triCount - 1;
			while (i <= j)
			{
				if (triangles[triIdx[i]].centroid[axis] < splitPos)
					i++;
				else
					std::swap(triIdx[i], triIdx[j--]);
			}

			// abort split if one of the sides is empty
			int leftCount = i - node.firstTriIdx;
			if (leftCount == 0 || leftCount == node.triCount) return;

			// Create child nodes for each half
			int leftChildIdx = nodesUsed++;
			int rightChildIdx = nodesUsed++;
			bvhNodePool[leftChildIdx].firstTriIdx = node.firstTriIdx;
			bvhNodePool[leftChildIdx].triCount = leftCount;
			bvhNodePool[rightChildIdx].firstTriIdx = i;
			bvhNodePool[rightChildIdx].triCount = node.triCount - leftCount;
			node.leftNode = leftChildIdx;
			node.triCount = 0;
			UpdateNodeBounds(leftChildIdx);
			UpdateNodeBounds(rightChildIdx);

			// Recurse into each of the child nodes.
			Subdivide(leftChildIdx);
			Subdivide(rightChildIdx);
		}
	};


#pragma endregion
#pragma region LIGHT
	enum class LightType
	{
		Point,
		Directional
	};

	struct Light
	{
		Vector3 origin{};
		Vector3 direction{};
		ColorRGB color{};
		float intensity{};

		LightType type{};
	};
#pragma endregion
#pragma region MISC
	struct Ray
	{
		Vector3 origin{};
		Vector3 direction{};

		float min{ 0.0001f };
		float max{ FLT_MAX };
	};

	struct HitRecord
	{
		Vector3 origin{};
		Vector3 normal{};
		float t = FLT_MAX;

		bool didHit{ false };
		unsigned char materialIndex{ 0 };
	};
#pragma endregion
}