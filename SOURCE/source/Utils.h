#pragma once
#include <cassert>
#include <fstream>
#include "Math.h"
#include "DataTypes.h"

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS
		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const Vector3 sphereToRay = ray.origin - sphere.origin;
			const float a = Vector3::Dot(ray.direction, ray.direction);
			const float b = 2.0f * Vector3::Dot(ray.direction, sphereToRay);
			const float c = Vector3::Dot(sphereToRay, sphereToRay) - Square(sphere.radius);
			const float discriminant = Square(b) - (4.0f * a * c);
			float t0;
			float t1;

			if (discriminant < 0) return false;

			if (discriminant > 0)
			{
				float q{};
				if (b > 0)
				{
					q = -0.5f * (b + sqrt(discriminant));
				}
				else
				{
					q = -0.5f * (b - sqrt(discriminant));
				}
				t0 = q / a;
				t1 = c / q;

			}
			else
			{
				t0 = t1 = -0.5f * b / a;
			}

			//ensures that the smallest value is always used
			if (t0 > t1) std::swap(t0, t1);
			if (t0 < 0) {
				t0 = t1;  //if t0 is negative, let's use t1 instead 
				if (t0 < 0) return false;  //both t0 and t1 are negative 
			}


			const float t = t0;
			if (t < ray.min || t > ray.max)
			{
				return false;
			}
			if (ignoreHitRecord == false)
			{
				hitRecord.origin = ray.origin + ray.direction * t;
				hitRecord.didHit = true;
				hitRecord.t = t;
				hitRecord.materialIndex = sphere.materialIndex;
				hitRecord.normal = Vector3{ (hitRecord.origin - sphere.origin).Normalized() };
				return true;
			}
			else
			{
				return true;
			}

		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const float nominator{ Vector3::Dot((plane.origin - ray.origin),plane.normal) };
			const float denominator{ Vector3::Dot(ray.direction,plane.normal) };
			const float t{ nominator / denominator };
			const float epsilon{ FLT_EPSILON };
			if (t < ray.min || t > ray.max)
			{
				return false;
			}
			if (t > epsilon && ignoreHitRecord == false)
			{
				hitRecord.normal = plane.normal;
				hitRecord.didHit = true;
				hitRecord.materialIndex = plane.materialIndex;
				hitRecord.origin = ray.origin + (ray.direction * t);
				hitRecord.t = t;
				return true;
			}

		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest
		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//const float epsilon{ FLT_EPSILON };
			//const Vector3 center{ (triangle.v0 + triangle.v1 + triangle.v2) * 0.3333333f }; //center of the triangle
			//const Vector3 v{ ray.direction };

			//const Vector3 L{ (center - ray.origin) };
			//const float nom{Vector3::Dot(L,triangle.normal)};
			//const float denom{ 1 / Vector3::Dot(v,triangle.normal)};

			//const float t{ nom * denom };

			//if(Vector3::Dot(triangle.normal,v) == 0.0f)
			//	return false;
			//if (t <= ray.min)
			//	return false;
			//if (t >= ray.max)
			//	return false;

			//switch (triangle.cullMode)
			//{
			//case TriangleCullMode::FrontFaceCulling:
			//	if (Vector3::Dot(v, triangle.normal) < 0)
			//		return false;
			//	break;
			//case TriangleCullMode::BackFaceCulling:
			//	if (Vector3::Dot(v, triangle.normal) > 0)
			//		return false;
			//	break;
			//case TriangleCullMode::NoCulling:
			//	break;
			//}

			//const Vector3 p{ ray.origin + (t * v) };

			//const Vector3 edgeA{ triangle.v1 - triangle.v0 };
			//const Vector3 edgeB{ triangle.v2 - triangle.v1 };
			//const Vector3 edgeC{ triangle.v0 - triangle.v2 };

			//Vector3 pointToSideA{ p - triangle.v0 };
			//Vector3 pointToSideB{ p - triangle.v1 };
			//Vector3 pointToSideC{ p - triangle.v2 };

			//if (Vector3::Dot(triangle.normal, Vector3::Cross(edgeA, pointToSideA)) < 0)
			//	return false;
			//if (Vector3::Dot(triangle.normal, Vector3::Cross(edgeB, pointToSideB)) < 0)
			//	return false;
			//if (Vector3::Dot(triangle.normal, Vector3::Cross(edgeC, pointToSideC)) < 0)
			//	return false;

			//if (t > 0 && ignoreHitRecord == true)
			//{
			//	return true;
			//}
			//if (t > 0 && ignoreHitRecord == false)
			//{
			//	hitRecord.normal = triangle.normal;
			//	hitRecord.didHit = true;
			//	hitRecord.materialIndex = triangle.materialIndex;
			//	hitRecord.origin = p;
			//	hitRecord.t = t;

			//	return true;
			//}

			//return false;

			// find vectors for two edges sharing vert0
			const Vector3 edge1 = triangle.v1 - triangle.v0;
			const Vector3 edge2 = triangle.v2 - triangle.v0;

			// begin calculating determinant
			const Vector3 pVec = Vector3::Cross(ray.direction, edge2);

			// if determinant is near zero, ray lies in plane of triangle
			const float det = Vector3::Dot(edge1, pVec);
			if (det > -FLT_EPSILON && det < FLT_EPSILON) return false; // ray parallel to triangle

			// Determine whether the triangle is front-facing or back-facing
			bool isFrontFacing = true; // Assume front-facing
			switch (triangle.cullMode) {
			case TriangleCullMode::FrontFaceCulling: {
				if (det > 0.0f) return false; // Cull front-facing triangles
				break;
			}
			case TriangleCullMode::BackFaceCulling: {
				if (det < 0.0f) return false; // Cull back-facing triangles
				isFrontFacing = false;
				break;
			}
			case TriangleCullMode::NoCulling: {
				break; // No culling
			}
			}

			const float invDet = 1.0f / det;

			// calculate distance from vert to ray origin
			const Vector3 tVec = ray.origin - triangle.v0;

			// calculate U parameter and test bounds
			const float u = invDet * Vector3::Dot(tVec, pVec);

			if (u < 0.0f || u > 1.0f) return false;

			const Vector3 qVec = Vector3::Cross(tVec, edge1);

			// calculate V parameter and test bounds
			const float v = invDet * Vector3::Dot(ray.direction, qVec);
			if (v < 0.0f || u + v > 1.0f) return false;

			// calculate t
			const float t = invDet * Vector3::Dot(edge2, qVec);
			if (t < ray.min || t > ray.max) return false;

			hitRecord.normal = isFrontFacing ? -triangle.normal : triangle.normal;
			hitRecord.didHit = true;
			hitRecord.materialIndex = triangle.materialIndex;
			hitRecord.origin = ray.origin + (ray.direction * t);
			hitRecord.t = t;

			return true;

		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region TriangeMesh HitTest
		inline bool SlabTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			float tx1 = (mesh.transformedMinAABB.x - ray.origin.x) / ray.direction.x;
			float tx2 = (mesh.transformedMaxAABB.x - ray.origin.x) / ray.direction.x;

			float tmin = std::min(tx1, tx2);
			float tmax = std::max(tx1, tx2);

			float ty1 = (mesh.transformedMinAABB.y - ray.origin.y) / ray.direction.y;
			float ty2 = (mesh.transformedMaxAABB.y - ray.origin.y) / ray.direction.y;

			tmin = std::max(tmin, std::min(ty1, ty2));
			tmax = std::min(tmax, std::max(ty1, ty2));

			float tz1 = (mesh.transformedMinAABB.x - ray.origin.x) / ray.direction.x;
			float tz2 = (mesh.transformedMaxAABB.x - ray.origin.x) / ray.direction.x;

			tmin = std::max(tmin, std::min(tz1, tz2));
			tmax = std::min(tmax, std::max(tz1, tz2));

			return tmax > 0 && tmax >= tmin;
		}
		inline bool SlabTest_BVH(const Ray& ray, const Vector3& bmin, const Vector3& bmax)
		{
			float tx1 = (bmin.x - ray.origin.x) / ray.direction.x, tx2 = (bmax.x - ray.origin.x) / ray.direction.x;
			float tmin = std::min(tx1, tx2), tmax = std::max(tx1, tx2);
			float ty1 = (bmin.y - ray.origin.y) / ray.direction.y, ty2 = (bmax.y - ray.origin.y) / ray.direction.y;
			tmin = std::max(tmin, std::min(ty1, ty2)), tmax = std::min(tmax, std::max(ty1, ty2));
			float tz1 = (bmin.z - ray.origin.z) / ray.direction.z, tz2 = (bmax.z - ray.origin.z) / ray.direction.z;
			tmin = std::max(tmin, std::min(tz1, tz2)), tmax = std::min(tmax, std::max(tz1, tz2));
			return tmax >= tmin && tmax > 0;
		}

		inline bool HitTest_BVH(TriangleMesh& mesh, const Ray& ray, const unsigned int nodeIdx, HitRecord& temp, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			BVHNode& node = mesh.bvhNodePool[nodeIdx];
			//If the node's AABB is not hit, return
			if (!SlabTest_BVH(ray, node.aabbMin, node.aabbMax)) return false;

			// Check if node has triangles ( IsLeaf() { return triCount > 0; } )
			if (node.IsLeaf())
			{
				for (int i{}; i < (int)node.triCount; ++i)
				{

					if (HitTest_Triangle(mesh.triangles[mesh.triIdx[node.firstTriIdx + i]], ray, temp))
					{
						if (ignoreHitRecord)
							return true;
						if (temp.t < hitRecord.t)
							hitRecord = temp;
					}
				}

			}
			else
			{
				HitTest_BVH(mesh, ray, node.leftNode, temp, hitRecord);
				HitTest_BVH(mesh, ray, node.leftNode + 1, temp, hitRecord);
			}
			return hitRecord.didHit;

		}
		inline bool HitTest_TriangleMesh(TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//temporary hitrecord to store triangle hits
			HitRecord temp{};
			//check if the ray intersects the AABB first
			if (SlabTest_TriangleMesh(mesh, ray) == false)
			{
				return false;
			}

			return HitTest_BVH(mesh, ray, mesh.rootNodeIdx, temp, hitRecord, ignoreHitRecord);


		}



		inline bool HitTest_TriangleMesh(TriangleMesh& mesh, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_TriangleMesh(mesh, ray, temp, true);
		}



#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3 origin)
		{
			if (light.type == LightType::Point)
			{
				const Vector3 direction{ light.origin - origin };
				return direction;
			}
			if (light.type == LightType::Directional)
			{
				return Vector3{ FLT_MAX,FLT_MAX,FLT_MAX };
			}
			return {};
			//assert(false && "No Implemented Yet!");

		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			const Vector3 pointToShade{ light.origin - target };
			if (light.type == LightType::Point)
			{
				return { light.color * light.intensity / Vector3::Dot(pointToShade , pointToShade) };
			}
			else
			{
				return { light.color * light.intensity };

			}

			//return {light.color * light.intensity};
		}
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((int)i0 - 1);
					indices.push_back((int)i1 - 1);
					indices.push_back((int)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof())
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if (isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}