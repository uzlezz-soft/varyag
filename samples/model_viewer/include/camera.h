#pragma once

#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

class Camera
{
public:
	float rotationSpeed = 10.0f;
	float zoomSpeed = 5.0f;
	float sensitivity = 0.002f;
	bool rotating = false;
	bool panning = false;
	bool firstMouse = true;

	Camera() = default;
	Camera(glm::vec3 target, float radius)
		: _target(target), _distance(radius), _targetDistance(radius), _position{0, 0, radius}
	{}

	void Update(float deltaTime);

	glm::vec3 GetPosition() const { return _position; }
	glm::vec3 GetForwardVector() const { return glm::normalize(_target - _position); }
	glm::mat4 GetViewMatrix() const { return glm::lookAtLH(_position, _target, glm::vec3(0, 0, 1)); }

	void Zoom(float deltaRadius)
	{
		_targetDistance = glm::max(0.01f, _targetDistance + deltaRadius);
	}
	void ProcessMouseMovement(double xPos, double yPos);

private:
	glm::vec3 _target;
	float _distance{ 0.01f };
	float _targetDistance{ 0.01f };
	glm::quat _orientation{ 1, 0, 0, 0 };
	glm::quat _targetOrientation{ 1, 0, 0, 0 };
	glm::vec3 _position{};
	double _lastX{};
	double _lastY{};
};