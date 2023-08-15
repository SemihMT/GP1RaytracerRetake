#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle) :
			origin{ _origin },
			fovAngle{ _fovAngle }
		{
		}


		Vector3 origin{};
		float fovAngle{ 90.f };

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{ 0.f };
		float totalYaw{ 0.f };

		Matrix cameraToWorld{};


		Matrix CalculateCameraToWorld() 
		{
			//see slide 19
			//todo: W2
			Vector3 worldUp{ Vector3::UnitY }; //{0,1,0}
			Vector3 right{ Vector3::Cross(worldUp,forward).Normalized() };
			Vector3 up{ Vector3::Cross(forward,right).Normalized() };

			//Temp change to verify
			//forward = {0.266f,-0.453f,0.860f};
			cameraToWorld = { Vector4(right,0),Vector4(up,0),Vector4(forward,0),Vector4(origin,1) };

			return { cameraToWorld };
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();
			const float movementSpeed{ 5.f };
			const float rotationSpeed{ .1f };
			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			if (pKeyboardState[SDL_SCANCODE_W])
			{
				origin += forward.Normalized() * movementSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_A])
			{
				origin -= right.Normalized() * movementSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin -= forward.Normalized() * movementSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_D])
			{
				origin += right.Normalized() * movementSpeed * deltaTime;
			}
			if (mouseState == SDL_BUTTON(1))
			{
				origin -= forward.Normalized() * float(mouseY) * deltaTime;
				totalYaw += float(mouseX) * rotationSpeed * deltaTime;
			}
			if (mouseState == SDL_BUTTON(3))
			{
				totalYaw += float(mouseX) * rotationSpeed * deltaTime;
				totalPitch -= float(mouseY) * rotationSpeed * deltaTime;
			}
			if (mouseState == (SDL_BUTTON(1) | SDL_BUTTON(3)))
			{ 
				origin += up.Normalized() * float(mouseY) * deltaTime;
			}

			const Matrix finalRotation{ Matrix::CreateRotation(totalPitch,totalYaw,0) };
			forward = finalRotation.TransformVector(Vector3::UnitZ).Normalized();
			right = finalRotation.TransformVector(Vector3::UnitX).Normalized();

			
		}
	};
}
