#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

Camera::Camera(
    glm::vec3 position,
    glm::vec3 worldup,
    float yaw,
    float pitch,
    float lastX,
    float lastY,
    float sensitivity,
    float speed
)    
    :
    Position(position),
    WorldUp(worldup),
    Yaw(yaw),
    Pitch(pitch),
    LastX(lastX),
    LastY(lastY),
    MouseSensitivity(sensitivity),
    MovementSpeed(speed),
    firstMouse(true)
{
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;
    glm::vec3 flatFront = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));
    if (glm::length(flatFront) < 0.001f)
        flatFront = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 flatRight = glm::cross(flatFront, WorldUp); 
    if (direction == FORWARD)
        Position += flatFront * velocity;
    if (direction == BACKWARD)
        Position -= flatFront * velocity;
    if (direction == LEFT)
        Position -= flatRight * velocity;
    if (direction == RIGHT)
        Position += flatRight * velocity;
    if (direction == UP)
        Position += WorldUp * velocity;
    if (direction == DOWN)
        Position -= WorldUp * velocity;
}

void Camera::ProcessMouseMovement(float xpos, float ypos)
{
    if (firstMouse)
    {
        LastX = xpos;
        LastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - LastX;
    float yoffset = LastY - ypos; // 反转y轴
    LastX = xpos;
    LastY = ypos;

    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    // 限制俯仰角度，避免翻转
    if (Pitch > 89.0f)
        Pitch = 89.0f;
    if (Pitch < -89.0f)
        Pitch = -89.0f;

    updateCameraVectors();
}

void Camera::Resetmouse()
{
    firstMouse = true;
}

void Camera::updateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    
    glm::vec3 rightVec = glm::cross(Front, WorldUp);
    if (glm::length(rightVec) > 0.0f) 
    {
        Right = glm::normalize(rightVec);
    } 
    else 
    {
        Right = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    
    glm::vec3 upVec = glm::cross(Right, Front);
    if (glm::length(upVec) > 0.0f) {
        Up = glm::normalize(upVec);
    } else {
        Up = WorldUp;
    }
}