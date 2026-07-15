#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera
{

public:
//构造函数
    Camera( 
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), 
        glm::vec3 worldup = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = -90.0f,
        float pitch = 0.0f,
        float lastX = 400.0f, 
        float lastY = 300.0f,
        float sensitivity = 0.1f,
        float speed = 18.0f
);

//对外接口
glm::mat4 GetViewMatrix() const;
	glm::vec3 Getposition () const { return Position;}
	glm::vec3 GetFront() const { return Front; }
	void ProcessKeyboard(Camera_Movement direction, float deltaTime);
	void ProcessSmoothKeyboard(
		bool forward,
		bool backward,
		bool left,
		bool right,
		bool up,
		bool down,
		float deltaTime
	);
	void ProcessMouseMovement(float xpos, float ypos);
	void Resetmouse();

private:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;  
    float Yaw;
    float Pitch;
	float MovementSpeed;
	float MouseSensitivity;
	glm::vec3 MovementVelocity;
	float LastX;
	float LastY;
    bool firstMouse;
    void updateCameraVectors();
};

#endif



    
