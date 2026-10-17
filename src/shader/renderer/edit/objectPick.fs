#version 330 core

layout(location = 0) out uint PickID;   

uniform uint objectID;

void main()
{
    PickID = objectID;
}
