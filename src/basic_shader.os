@vertex
#version 450 core

layout(location=0) in vec2 in_Pos;

void main() {

    gl_Position = vec4(in_Pos.xy, 0, 1);

}

@fragment
#version 450 core

out vec4 FragColor;

void main() {
    FragColor = vec4(1, 0, 0, 1);
}

@end
