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

out vec4 color;

uniform mat4 u_model_matrix;

void main() {
    const Node in_node = in_nodes.nodes[gl_InstanceID];
    gl_Position = vec4(in_Pos * in_node.radius, 0, 1) - vec4(in_node.position, 0, 0);
    color = vec4(1 - in_node.mass, 0, 1 - in_node.mass, 1);

}

@fragment
#version 450 core

uniform vec3 u_color;

in vec4 color;
out vec4 FragColor;

void main() {
    FragColor = vec4(color);
}

@end
