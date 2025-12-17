#pragma once
#include "Module.h"

class ModuleCamera : public Module
{
public:
	ModuleCamera();
	~ModuleCamera();
	bool init() override;
	void update() override;

	void CreateLookAt(const Vector3& position, const Vector3& target, const Vector3& up);

	void SetFOV(float fov);
	void SetPlaneDistances(float nearPlane, float farPlane);

	void focusOnPosition(const Vector3& position);

	Matrix* GetViewMatrix() { return &view; }
	Matrix* GetProjectionMatrix() { return &proj; }

private:
	float scrollWheel = 0.0f;

	Matrix view;
	Matrix proj;

	float fov = XM_PIDIV4;
	float nearPlane;
	float farPlane;
	float aspectRatio;

	float translationSpeedMultiplier = 1.0f;
	float translationInitialSpeedBoost = 1.0f;
	
	Vector3 camPos;
	Vector3 focusPos = Vector3::Zero;
	float orbitDistance;
	Quaternion camRot;
	float camYaw = 0.0f;
	float camPitch = 0.0f;

};

