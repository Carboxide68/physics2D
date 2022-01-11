@vertex
#version 450 core

layout(location=0) in vec2 in_Pos;

struct Node {

    float mass;
    float radius;
    vec2 position;
    vec2 velocity;

};

layout(std430, binding = 0) readonly buffer input_nodes {
    
    uint node_count;
    Node nodes[];

} in_nodes;

uniform vec3 u_color;
uniform uint u_color_mode;

out vec4 color;

void main() {
    const Node in_node = in_nodes.nodes[gl_InstanceID];
    gl_Position = vec4(in_Pos * in_node.radius, 0, 1) - vec4(in_node.position, 0, 0);
    if (u_color_mode == 0) {
        color = vec4(1 - in_node.mass, 0, 1 - in_node.mass, 1);
    } else if (u_color_mode == 1) {
        color = vec4((in_node.velocity.x * 0.1 + 1)/2, 0, (in_node.velocity.y * 0.1 + 1)/2, 1);
    } else {
        color = vec4(u_color, 1);
    }
}

@fragment
#version 450 core


in vec4 color;
out vec4 FragColor;

void main() {
    FragColor = vec4(color);
}

@end
