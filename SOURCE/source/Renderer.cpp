//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"

#include <future> //async
#include <ppl.h> //parallel_for
using namespace dae;
//#define ASYNC
#define PARALLEL_FOR
Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow))

{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

void Renderer::Render(Scene* pScene)
{
	Camera& camera = pScene->GetCamera();
	camera.CalculateCameraToWorld();

	const float aspectRatio{ float(m_Width) / float(m_Height) };
	const float fov{ tanf(camera.fovAngle * TO_RADIANS / 2.0f) };

	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();

	const uint32_t numPixels = m_Width * m_Height;

#if defined(ASYNC)
	const uint32_t numCores = std::thread::hardware_concurrency();
	std::vector<std::future<void>> async_futures{};
	const uint32_t numPixelsPerTask = numPixels / numCores;
	uint32_t numUnassignedPixels = numPixels % numCores;
	uint32_t currPixelIndex = 0;


	for (uint32_t coreId{ 0 }; coreId < numCores; ++coreId)
	{
		uint32_t taskSize = numPixelsPerTask;
		if (numUnassignedPixels > 0)
		{
			++taskSize;
			--numUnassignedPixels;
		}
		async_futures.push_back(std::async(std::launch::async, [=, this]
			{
				//Render all pixels for this task (currPixelIndex > currPixelIndex + taskSize)
				const uint32_t pixelIndexEnd = currPixelIndex + taskSize;
				for (uint32_t pixelIndex{ currPixelIndex }; pixelIndex < pixelIndexEnd; ++pixelIndex)
				{
					RenderPixel(pScene, pixelIndex, fov, aspectRatio, camera, lights, materials);
				}
			}));
		currPixelIndex += taskSize;

	}

	for (const std::future<void>& f : async_futures)
	{
		f.wait();
	}

#elif defined(PARALLEL_FOR)
	concurrency::parallel_for(0u, numPixels, [=, this](int i)
		{
			RenderPixel(pScene, i, fov, aspectRatio, camera, lights, materials);
		});
#else
	//Synchronous exec
	for (uint32_t i{ 0 }; i < numPixels; ++i)
	{
		RenderPixel(pScene, i, fov, aspectRatio, camera, lights, materials);
	}
#endif

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

void dae::Renderer::RenderPixel(Scene* pScene, uint32_t pixelIndex, float fov, float aspectRatio, const Camera& camera, const std::vector<Light>& lights, const std::vector<Material*>& materials)
{
	const int px = int(pixelIndex) % m_Width;
	const int py = int(pixelIndex) / m_Width;

	const float pxc{ px + 0.5f };
	const float pyc{ py + 0.5f };

	//from raster (pixel) space to camera space calculation
	const float cx{ (2.f * pxc / float(m_Width) - 1.0f) * (aspectRatio * fov) };
	const float cy{ (1.f - 2.f * pyc / float(m_Height)) * fov };

	//Linear combination of scaled unit vectors defining the direction of a ray through the middle of a given pixel
	Vector3 rayDirection{ cx,cy,1 };
	rayDirection = camera.cameraToWorld.TransformVector(rayDirection);
	rayDirection.Normalize();


	const Ray viewRay{ camera.origin,rayDirection };

	ColorRGB finalColor{};

	HitRecord closestHit{};

	pScene->GetClosestHit(viewRay, closestHit);


	if (closestHit.didHit)
	{
		for (const auto& light : lights)
		{
			const Vector3 lightDir{ LightUtils::GetDirectionToLight(light, closestHit.origin + (closestHit.normal * 0.0001f)) };
			const Vector3 normalizedLightDir{ lightDir.Normalized() };

			//Vector pointing from closesthit.origin to light.origin
			const Ray lightRay{ closestHit.origin + (closestHit.normal * 0.0001f),normalizedLightDir,0.0001f, lightDir.Magnitude() };


			//if we want shadows,
			if (m_ShadowsEnabled)
			{
				//check if lightRay is obstructed by anything 
				if (pScene->DoesHit(lightRay))
				{
					//if so, don't bother with calculating lighting for this pixel
					continue;
				}
			}

			switch (m_CurrentLightingMode)
			{
			case LightingMode::Combined:
			{
				const float observedArea{ Vector3::Dot(closestHit.normal,normalizedLightDir) };
				if (observedArea < 0)
					continue;

				finalColor += LightUtils::GetRadiance(light, closestHit.origin)
					* materials[closestHit.materialIndex]->Shade(closestHit, normalizedLightDir, -camera.forward) * observedArea;
			}
			break;

			case LightingMode::ObservedArea:
			{
				const float observedArea{ Vector3::Dot(closestHit.normal,normalizedLightDir) };
				if (observedArea < 0)
					continue;
				finalColor += {observedArea, observedArea, observedArea};
			}
			break;

			case LightingMode::Radiance:
			{
				finalColor += LightUtils::GetRadiance(light, closestHit.origin);
			}
			break;

			case LightingMode::BRDF:
			{
				finalColor += materials[closestHit.materialIndex]->Shade(closestHit, normalizedLightDir, -camera.forward);
			}
			break;

			}
		}
	}
	//Update Color in Buffer
	finalColor.MaxToOne();

	m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
		static_cast<uint8_t>(finalColor.r * 255),
		static_cast<uint8_t>(finalColor.g * 255),
		static_cast<uint8_t>(finalColor.b * 255));
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}

void dae::Renderer::CycleLightingMode()
{

	m_CurrentLightingMode = LightingMode((int(m_CurrentLightingMode) + 1) % 4);

}

