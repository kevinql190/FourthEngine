#include "Globals.h"
#include "ModuleCamera.h"
#include "Application.h"

#include "Mouse.h"
#include "Keyboard.h"

namespace
{
	constexpr float MOUSE_ROT_SENSITIVITY = 0.005f;
	constexpr float MOUSE_PAN_SENSITIVITY = 0.002f;
	constexpr float MOUSE_WHEEL_ZOOM_SENSITIVITY = 0.003f;
	constexpr float MOUSE_DRAG_ZOOM_SENSITIVITY = 0.007f;
	constexpr float ROTATION_SPEED = 25.0f;
	constexpr float TRANSLATION_SPEED = 12.0f;
	constexpr float INITIAL_SPEED_BOOST = 12.0f;
	constexpr float INITIAL_ORBIT_DISTANCE = 3.0f;
}

ModuleCamera::ModuleCamera() : Module()
{
}
ModuleCamera::~ModuleCamera()
{

}

bool ModuleCamera::init()
{
	nearPlane = 0.1f;
	farPlane = 10000.0f;

	camPitch = XMConvertToRadians(-30.0f);
	camYaw = XMConvertToRadians(45.0f);
	camRot = Quaternion::CreateFromAxisAngle(Vector3::Right, camPitch) * Quaternion::CreateFromAxisAngle(Vector3::Up, camYaw);
	focusOnPosition(focusPos); // focusPoint set to origin by default

	return true;
}

void ModuleCamera::update()
{
	Mouse& mouse = Mouse::Get();
	const Mouse::State& mouseState = mouse.GetState();
	Keyboard& keyboard = Keyboard::Get();
	const Keyboard::State& keyState = keyboard.GetState();

	float elapsedSec = app->getElapsedMilis() * 0.001f;

	// Handle Mouse mode and visibility
	bool isClicking = false;
	if (mouseState.rightButton || mouseState.leftButton || mouseState.middleButton)
	{
		mouse.SetMode(Mouse::MODE_RELATIVE);
		mouse.SetVisible(false);
		// Check relative values to avoid large jumps when passing from absolute to relative mode
		if (std::abs(mouseState.x) < 300 && std::abs(mouseState.y) < 300)
			isClicking = true;
	}
	else
	{
		mouse.SetMode(Mouse::MODE_ABSOLUTE);
		mouse.SetVisible(true);
		translationSpeedMultiplier = 1.0f;
	}

	// Handle input
	Vector3 translate = Vector3::Zero; // WASDQE + right button / scroll / middle button drag
	Vector2 rotate = Vector2::Zero; // right button drag
	Vector2 orbit = Vector2::Zero; // Alt + left button drag

	// Handle mouse Input
	if (isClicking)
	{
		if (mouseState.rightButton && !keyState.LeftAlt)
		{
			// Rotate camera (1st person)
			rotate.x = float(-mouseState.x) * MOUSE_ROT_SENSITIVITY;
			rotate.y = float(-mouseState.y) * MOUSE_ROT_SENSITIVITY;

			// Translate camera
			if (keyState.W) translate.z -= 0.45f * elapsedSec;
			if (keyState.S) translate.z += 0.45f * elapsedSec;
			if (keyState.A) translate.x -= 0.45f * elapsedSec;
			if (keyState.D) translate.x += 0.45f * elapsedSec;
			if (keyState.Q) translate.y -= 0.45f * elapsedSec;
			if (keyState.E) translate.y += 0.45f * elapsedSec;
			if (translate.LengthSquared() > 0.0f && keyState.LeftShift) translationSpeedMultiplier += 0.45f * elapsedSec;
			else translationSpeedMultiplier = 1.0f;
		}
		else if (mouseState.middleButton)
		{
			// Pan camera
			translate.x = float(-mouseState.x) * MOUSE_PAN_SENSITIVITY;
			translate.y = float(mouseState.y) * MOUSE_PAN_SENSITIVITY;
		}
		else if (mouseState.leftButton && keyState.LeftAlt)
		{
			// Orbit camera (around focus point)
			orbit.x = float(-mouseState.x) * MOUSE_ROT_SENSITIVITY;
			orbit.y = float(-mouseState.y) * MOUSE_ROT_SENSITIVITY;
		}
		else if (mouseState.rightButton && keyState.LeftAlt)
		{
			// Zoom camera (changing orbit distance)
			orbitDistance += float(-mouseState.x) * MOUSE_DRAG_ZOOM_SENSITIVITY;
			orbitDistance += float(-mouseState.y) * MOUSE_DRAG_ZOOM_SENSITIVITY;
			camPos = focusPos - Vector3::Transform(Vector3::Forward * orbitDistance, camRot);
		}
	}

	// Handle mouse wheel zoom (changing orbit distance)
	float scrollDelta = float(mouseState.scrollWheelValue - scrollWheel);
	if (scrollDelta && !mouseState.rightButton)
	{
		orbitDistance += float(-scrollDelta) * MOUSE_WHEEL_ZOOM_SENSITIVITY;
		camPos = focusPos - Vector3::Transform(Vector3::Forward * orbitDistance, camRot);
	}

	// Handle focus input
	if (keyState.F)
	{
		if (keyState.LeftShift) focusOnPosition(Vector3::Zero); // TEMPORAL: Focus on world origin
		else focusOnPosition(focusPos);
	}

	// Update camera based on input
	if (orbit.LengthSquared() > 0.0f)
	{
		// Orbiting rotation
		camYaw += XMConvertToRadians(ROTATION_SPEED * orbit.x);
		camPitch += XMConvertToRadians(ROTATION_SPEED * orbit.y);
		// Clamp pitch to avoid gimbal lock
		camPitch = std::max(-XM_PIDIV2 + 0.01f, std::min(XM_PIDIV2 - 0.01f, camPitch));
		// Wrap yaw
		if (camYaw > XM_2PI) camYaw -= XM_2PI;
		if (camYaw < -XM_2PI) camYaw += XM_2PI;

		camRot = Quaternion::CreateFromAxisAngle(Vector3::Right, camPitch) * Quaternion::CreateFromAxisAngle(Vector3::Up, camYaw);

		// Update camera position based on orbit
		camPos = focusPos - Vector3::Transform(Vector3::Forward * orbitDistance, camRot);
	}
	else
	{
		// Camera Rotation
		camYaw += XMConvertToRadians(ROTATION_SPEED * rotate.x);
		camPitch += XMConvertToRadians(ROTATION_SPEED * rotate.y);
		// Clamp pitch to avoid gimbal lock
		camPitch = std::max(-XM_PIDIV2 + 0.01f, std::min(XM_PIDIV2 - 0.01f, camPitch));
		// Wrap yaw
		if (camYaw > XM_2PI) camYaw -= XM_2PI;
		if (camYaw < -XM_2PI) camYaw += XM_2PI;

		camRot = Quaternion::CreateFromAxisAngle(Vector3::Right, camPitch) * Quaternion::CreateFromAxisAngle(Vector3::Up, camYaw);

		// Camera position
		Vector3 localDir = Vector3::Transform(translate, camRot);
		float speed = TRANSLATION_SPEED * translationSpeedMultiplier * translationSpeedMultiplier * translationSpeedMultiplier;
		if (keyState.LeftShift) speed += INITIAL_SPEED_BOOST;
		camPos += localDir * speed;
		// Translate and orbit focus point as well
		focusPos = camPos + Vector3::Transform(Vector3::Forward * orbitDistance, camRot);
	}

	// Set view matrix
	Quaternion invRot;
	camRot.Inverse(invRot);
	view = Matrix::CreateFromQuaternion(invRot);
	view.Translation(Vector3::Transform(-camPos, invRot));

	// Set projection matrix
	LONG windowWidth = (LONG)app->getD3D12()->getWindowWidth();
	LONG windowHeight = (LONG)app->getD3D12()->getWindowHeight();
	float aspect = float(windowWidth) / float(windowHeight);
	proj = Matrix::CreatePerspectiveFieldOfView(fov, aspect, nearPlane, farPlane);

	scrollWheel = mouseState.scrollWheelValue;
}

void ModuleCamera::CreateLookAt(const Vector3& position, const Vector3& target, const Vector3& up)
{
	camPos = position;
	view = Matrix::CreateLookAt(position, target, up);
	camRot = Quaternion::CreateFromRotationMatrix(view.Invert());
	camYaw = atan2f(-view._13, view._11);
	camPitch = asinf(view._12);
}

void ModuleCamera::SetFOV(float fovAngle)
{
	fov = fovAngle;
}

void ModuleCamera::SetPlaneDistances(float nearP, float farP)
{
	nearPlane = nearP;
	farPlane = farP;
}

void ModuleCamera::focusOnPosition(const Vector3& position)
{
	camPos = position;
	orbitDistance = INITIAL_ORBIT_DISTANCE;
	camPos += Vector3::Transform(Vector3::Backward * orbitDistance, camRot);
}