@compute
#version 450

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

struct Node {

    float mass;
    float radius;
    vec2 position;
    vec2 velocity;

};

layout(std430, binding = 0) restrict readonly buffer input_nodes {

    uint node_count;
    Node nodes[];

} in_nodes;

layout(std430, binding = 1) restrict writeonly buffer output_nodes {
    
    readonly uint node_count;
    Node nodes[];

} out_nodes;

layout(std430, binding = 2) restrict readonly buffer in_environment {

    uint lines;
    vec2 points[];

} environment;

uniform float TS;
const float EPSILON = 0.0001;

void main() {

    const uint array_loc = gl_LocalInvocationIndex + gl_WorkGroupID.x * gl_WorkGroupSize.x;
    Node local = in_nodes.nodes[array_loc];
    vec2 additional_velocity = vec2(0, 0);
    for (uint i = 0; i < in_nodes.node_count; i++) {
        if (i == array_loc) continue;
        const Node other = in_nodes.nodes[i];
        const float radius_together = other.radius + local.radius;
        const vec2 between = local.position - other.position;
        const float len_between2 = dot(between, between);
        //if (len_between2 < EPSILON*EPSILON) continue;
        if (len_between2 > radius_together*radius_together) continue;
        const vec2 velo_diff = local.velocity - other.velocity;
        if (dot(velo_diff, between) >= 0) continue;
        const vec2 velocity = dot(between, velo_diff) * (between/len_between2);
        additional_velocity += -velocity * other.mass * 2.0/(other.mass + local.mass);
    }
    local.velocity += additional_velocity;
    additional_velocity = vec2(0,0);

    for (uint i = 0; i < environment.lines; i++) {
        const vec2 line = (environment.points[i*2+1]-environment.points[i*2]);
        const vec2 between = local.position-environment.points[i*2];
        if (dot(line, between) < 0) continue;
        const vec2 orth_line = vec2(-line.y, line.x);
        const vec2 normal_orth_line = normalize(orth_line);
        if (dot(orth_line, orth_line) < dot(between, between)) continue;
        const float len_between = dot(normal_orth_line, between);
        if (abs(len_between) > local.radius) continue;
        const vec2 to_line = dot(normal_orth_line, local.velocity) * normal_orth_line;
        const vec2 sum = -2.0 * to_line;
        additional_velocity += sum;
    }

    local.velocity += additional_velocity;
    local.position += local.velocity.xy * TS;

    out_nodes.nodes[array_loc] = local;
}

@end
