#pragma once
#include "Module.h"

class ModuleD3D12;

class ModuleCamera : public Module
{
public:
	ModuleCamera();
	~ModuleCamera();
	bool init() override;
	void update() override;

	void createLookAt(const Vector3& position, const Vector3& target, const Vector3& up);

	void setFOV(float fov);
	void setPlaneDistances(float nearPlane, float farPlane);

	void focusOnPosition(const Vector3& position, const Vector3& scale);

	const Matrix& GetViewMatrix() const { return view; }
	const Matrix& GetProjectionMatrix() const { return proj; }

private:
	ModuleD3D12* d3d12 = nullptr;

	float scrollWheel = 0.0f;
	float dragPosX = 0.0f;
	float dragPosY = 0.0f;

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

