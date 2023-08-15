#pragma once
#include <cassert>
#include "Math.h"

namespace dae
{
	namespace BRDF
	{
		/**
		 * \param kd Diffuse Reflection Coefficient
		 * \param cd Diffuse Color
		 * \return Lambert Diffuse Color
		 */
		static ColorRGB Lambert(float kd, const ColorRGB& cd)
		{
			ColorRGB rho{ cd * kd };

			return rho / PI;
		}

		static ColorRGB Lambert(const ColorRGB& kd, const ColorRGB& cd)
		{
			ColorRGB rho{ cd * kd };

			return  rho / PI;
		}

		/**
		 * \brief todo
		 * \param ks Specular Reflection Coefficient
		 * \param exp Phong Exponent
		 * \param l Incoming (incident) Light Direction
		 * \param v View Direction
		 * \param n Normal of the Surface
		 * \return Phong Specular Color
		 */
		static ColorRGB Phong(float ks, float exp, const Vector3& l, const Vector3& v, const Vector3& n)
		{
			const auto reflect = l - (2.f * (Vector3::Dot(n, l) * n));
			const float cosAlpha = Vector3::Dot(reflect, v);
			const float specularPow{ ks * powf(cosAlpha,exp) };
			ColorRGB specular{ specularPow ,specularPow,specularPow };
			//todo: W3
			//assert(false && "Not Implemented Yet");
			return specular;
		}

		/**
		 * \brief BRDF Fresnel Function >> Schlick
		 * \param h Normalized Halfvector between View and Light directions
		 * \param v Normalized View direction
		 * \param f0 Base reflectivity of a surface based on IOR (Indices Of Refrection), this is different for Dielectrics (Non-Metal) and Conductors (Metal)
		 * return
		 */
		static ColorRGB FresnelFunction_Schlick(const Vector3& h, const Vector3& v, const ColorRGB& f0)
		{
			ColorRGB One{1.f,1.f,1.f};
			//ColorRGB f0Complement{ 1.f - f0.r,1.f - f0.g,1.f - f0.b };
			return f0 + (One - f0) * powf((1.f - Vector3::Dot(h,v)),5);
		}

		/**
		 * \brief BRDF NormalDistribution >> Trowbridge-Reitz GGX (UE4 implemetation - squared(roughness))
		 * \param n Surface normal
		 * \param h Normalized half vector
		 * \param roughness Roughness of the material
		 * \return BRDF Normal Distribution Term using Trowbridge-Reitz GGX
		 */
		static float NormalDistribution_GGX(const Vector3& n, const Vector3& h, float roughness)
		{
			//todo: W3
			const float a{Square(roughness)};
			const float aSquared{ Square(a) };
			const float nDotH{ Vector3::Dot(n,h) };

			//assert(false && "Not Implemented Yet");
			return aSquared / (PI * Square(Square(nDotH) * (aSquared - 1.f) + 1.f));
		}


		/**
		 * \brief BRDF Geometry Function >> Schlick GGX (Direct Lighting + UE4 implementation - squared(roughness))
		 * \param n Normal of the surface
		 * \param v Normalized view direction
		 * \param roughness Roughness of the material
		 * \return BRDF Geometry Term using SchlickGGX
		 */
		static float GeometryFunction_SchlickGGX(const Vector3& n, const Vector3& v, float roughness)
		{
			const float nDotV{ Vector3::Dot(n,v) };
			const float k{ Square(Square(roughness) + 1.f) / 8.f };

			return nDotV / (nDotV * (1.f - k) + k);
		}

		/**
		 * \brief BRDF Geometry Function >> Smith (Direct Lighting)
		 * \param n Normal of the surface
		 * \param v Normalized view direction
		 * \param l Normalized light direction
		 * \param roughness Roughness of the material
		 * \return BRDF Geometry Term using Smith (> SchlickGGX(n,v,roughness) * SchlickGGX(n,l,roughness))
		 */
		static float GeometryFunction_Smith(const Vector3& n, const Vector3& v, const Vector3& l, float roughness)
		{
			//const float k{ Square(Square(roughness) + 1.f) / 8.f };

			const float shadowingGeo{ GeometryFunction_SchlickGGX(n, v, roughness) };
			const float maskingGeo{ GeometryFunction_SchlickGGX(n, l, roughness) };

			return  maskingGeo * shadowingGeo;
		}

	}
}