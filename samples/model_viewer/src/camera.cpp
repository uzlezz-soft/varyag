#include "camera.h"

void Camera::Update(float deltaTime)
{
    _distance = std::lerp(_distance, _targetDistance, deltaTime * zoomSpeed);
    _orientation = glm::slerp(_orientation, _targetOrientation, deltaTime * rotationSpeed);

    _position = _target + _orientation * glm::vec3(0, 0, _distance);
}

void Camera::ProcessMouseMovement(double xPos, double yPos)
{
    if (!rotating && !panning) return;

    if (firstMouse)
    {
        _lastX = xPos;
        _lastY = yPos;
        firstMouse = false;
    }

    float deltaX = static_cast<float>(xPos - _lastX) * sensitivity;
    float deltaY = static_cast<float>(yPos - _lastY) * sensitivity;
    _lastX = xPos;
    _lastY = yPos;

    if (rotating)
    {
        glm::quat yaw = glm::angleAxis(-deltaX, glm::vec3(0, 0, 1));
        glm::quat pitch = glm::angleAxis(deltaY, glm::vec3(1, 0, 0));
        _targetOrientation = glm::normalize(yaw * _targetOrientation * pitch);
    }
    else if (panning)
    {
        glm::vec3 worldUp = { 0, 0, 1 };
        glm::vec3 right = glm::normalize(glm::cross(worldUp, GetForwardVector()));
        glm::vec3 up = glm::normalize(glm::cross(right, GetForwardVector()));

        _target += -right * deltaX * _distance;
        _target += up * -deltaY * _distance;
    }
}