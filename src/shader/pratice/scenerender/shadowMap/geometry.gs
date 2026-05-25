#version 330 core

// 接受一个三角形
layout (triangles) in;
// 六个面
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 shadowMatrices[6];

out vec4 FragPos;

void main()
{
    for (int face = 0; face < 6; ++face)
    {
        // OpenGL内建变量，指定你要讲最终输出的光栅化结果输出到哪个面
        // 这里EmitVertex发送的顶点数据是被矩阵投影处理过的
        // 所以一个片段永远只能被投影到一个面上
        // 真正的分层不是在发送顶点数据的时候，而是在光栅化之后
        gl_Layer = face;

        for (int i = 0; i < 3; ++i)
        {
            FragPos = gl_in[i].gl_Position;
            // 将当前顶点转换到某个面的观察空间
            gl_Position = shadowMatrices[face] * FragPos;
            // 把顶点数据发给GPU,光栅化
            EmitVertex();
        }

        // 结束当前图元，开始下一个图元
        EndPrimitive();
    }
}