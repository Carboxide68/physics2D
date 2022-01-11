#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "common.h"
#include "buffer.h"
#include "shader.h"

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <tiny_gltf/tiny_gltf.h>

#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

#include <random>
#include <chrono>

constexpr uint circle_points = 30;
constexpr float circle_radius = 0.01f;
float TS = 0.0002;
const unsigned long long SEED = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

void glfw_error_callback(int error, const char* description) {
    printf("Glfw Error %d: %s\n", error, description);
}

void onglerror(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userparam) {
    if (severity == GL_DEBUG_SEVERITY_HIGH) 
        printf("OpenGL Error! Severity: High | Description: %s\n", message);
}

class Camera {
public:

    glm::mat4 perspective_matrix = glm::mat4(1);

};

int init_graphics_env(GLFWwindow*& window) {
     /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        return -1;
    }
    glDebugMessageCallback(onglerror, NULL);
glEnable(GL_DEBUG_OUTPUT);

    #ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    #endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    TracyGpuContext(window);

    return 0;

}

struct Environment {

    uint64_t lines;
    std::vector<glm::vec2> points;

};

class VoidData {
public:

    VoidData(size_t size) {
        start = malloc(size);
        head = start;
        end = start + size;
    }

    ~VoidData() {
        free(start);
    }

    template <typename T>
    void addData(T data) {    
        if (head + sizeof(T) > end) {printf("Reached maximum capacity!\n");return;}
        *((T*)head) = data;
        head += sizeof(T);
    }

    void begin(const std::string& label) {
        labels[label] = (size_t)head - (size_t)start;     
    }

    std::unordered_map<std::string, size_t> labels;

    void *head;
    void *start;
    void *end;

};

struct Node {
    float weight;
    float radius;
    glm::vec2 postion;
    glm::vec2 velocity;
};

Ref<Buffer> createBufferedObjects(Environment& env, uint nodecount, std::unordered_map<std::string, size_t>& labels) {

    std::vector<Node> nodes(nodecount);

    std::random_device rand_dev;
    std::mt19937 rng(SEED);
    std::uniform_real_distribution<float> distribution(0.0, 1.0);
    float bounding = 0.9f;
    for (uint i = 0; i < nodecount; i++) {
        float radius = circle_radius;
        float weight = 0.7 * distribution(rng) + 0.3;
        glm::vec2 position;
        position.x = (bounding - radius) * (distribution(rng) * 2.0f - 1.0f);
        position.y = (bounding - radius) * (distribution(rng) * 2.0f - 1.0f);
        float angle = distribution(rng) * 2.0f * glm::pi<float>();
        glm::vec2 velocity = glm::vec2(glm::cos(angle), glm::sin(angle)) * distribution(rng) * 20.0f;
        nodes[i] = {weight, radius, position, velocity};
    }

    size_t buffer_size = (sizeof(uint64_t) + nodecount * sizeof(Node)) * 2; //2 Nodethings
    buffer_size += sizeof(glm::vec2) * env.points.size() + sizeof(uint64_t) + 100;
    printf("Buffer Size: %lu\n", buffer_size);
    Ref<Buffer> SSBO_object = Buffer::Create(buffer_size, GL_STATIC_DRAW);

    VoidData data = VoidData(buffer_size);
    data.begin("environment");
    data.addData(env.lines);
    data.begin("points");
    for (auto& point : env.points) {
        data.addData(point);
    }
    data.begin("environment#END");
    
    data.head += 16 - ((size_t)data.head - (size_t)data.start) % 16;
    data.begin("nodes1");
    data.addData((uint64_t)nodecount);

    for (auto& node : nodes) {
        data.addData(node);
    }
    data.begin("nodes1#END");

    data.head += 16 - ((size_t)data.head - (size_t)data.start) % 16;

    data.begin("nodes2");
    const size_t datasize = data.labels["nodes1#END"] - data.labels["nodes1"];
    memset(data.head, 0, datasize);
    data.head += datasize;
    data.begin("nodes2#END");

    SSBO_object->subData(data.start, 0, buffer_size);
    data.begin("end");
    labels = data.labels;
    return SSBO_object;
}

int main(int argc, char *argv[]) {
    
    glfwSetErrorCallback(glfw_error_callback);
    GLFWwindow* window;
    if (init_graphics_env(window) == -1) {
        printf("Couldn't properly create graphics a environment!\n");
        return -1;
    }

    //Generate the vertex data for a circle, drawn with GL_TRIANGLE_FAN
    float data[(circle_points + 1) * 2];
    data[0] = 0.0;
    data[1] = 0.0;

    for (uint i = 1; i < circle_points+1; i++) {
        const double angle = glm::pi<double>() * 2.0 * ((double)(i-1)/(double)(circle_points-1));
        data[i*2+0] = glm::cos(angle);
        data[i*2+1] = glm::sin(angle);
    }
    const float border = 0.9;
    Environment env = {
        4,
        {
            { border,  border},
            {-border,  border},
            {-border,  border},
            {-border, -border},
            {-border, -border},
            { border, -border},
            { border, -border},
            { border,  border},
        },
    };

    //Load the vertex data into a buffer
    Ref<Buffer> vertex_buffer = Buffer::Create(sizeof(data), GL_STATIC_DRAW);
    vertex_buffer->subData(data, 0, sizeof(data));
    std::unordered_map<std::string, size_t> labels;
    const uint nodecount = 1000;

    //Generate the points with velocities, and load everything into a GPU buffer
    Ref<Buffer> SSBO = createBufferedObjects(env, nodecount, labels);

    //Bind the draw data to a VAO with the correct layout
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);
    vertex_buffer->bind(GL_ARRAY_BUFFER);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
    glEnableVertexAttribArray(0);

    Buffer::unbind(GL_ARRAY_BUFFER);
    glBindVertexArray(0);

    //Make a VAO for the lines that are to be drawn
    unsigned int line_drawer;
    glGenVertexArrays(1, &line_drawer);
    glBindVertexArray(line_drawer);
    SSBO->bind(GL_ARRAY_BUFFER);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)labels["points"]);
    glEnableVertexAttribArray(0);

    Buffer::unbind(GL_ARRAY_BUFFER);
    glBindVertexArray(0);

    //Load shaders
    Ref<Shader> basic_shader = Shader::Create("src/physics_sphere.os");
    Ref<Shader> line_shader = Shader::Create("src/line_shader.os");
    Ref<Shader> compute_shader = Shader::Create("src/physics.os");

    glClearColor(0.7, 0.7, 0.7, 1.0); 
    glm::vec3 color = glm::vec3(0.7, 0, 0);
    bool do_tick = false;
    int tpf = 1;
    uint color_mode = 0;

    if (argc > 1) {
        printf("argv[1]: %s\n", argv[1]);
        return 0;
    }
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        const int biggest = (display_w > display_h) ? display_w : display_h;
        const int smallest = (display_w < display_h) ? display_w : display_h;
        glViewport((display_w-smallest)/2, (display_h-smallest)/2, smallest, smallest);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Window");

        if (ImGui::Button("Do tick")) do_tick = !do_tick;
        ImGui::InputInt("Times per frame", &tpf);

        ImGui::Text("Color Mode: "); ImGui::SameLine(); 
        if (ImGui::Button("Weights")) color_mode = 0; ImGui::SameLine();
        if (ImGui::Button("Speed")) color_mode = 1; ImGui::SameLine();
        if (ImGui::Button("Circle color")) color_mode = 2;
        
        if (color_mode == 2)
            ImGui::ColorEdit3("Circle color", glm::value_ptr(color));

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::End();
        
        //Do physics computation
        if (do_tick) {
            ZoneScopedN("All ticks");
            for (int i = 0; i < tpf; i++) {
                ZoneScopedN("PhysicsTicks");
                TracyGpuZone("PhysicsCompute");

                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                compute_shader->Bind();
                compute_shader->SetUniform("TS", TS);
                glBindBufferRange(  GL_SHADER_STORAGE_BUFFER, 0, SSBO->getHandle(), 
                                    labels["nodes1"], labels["nodes1#END"] - labels["nodes1"]);
                glBindBufferRange(  GL_SHADER_STORAGE_BUFFER, 1, SSBO->getHandle(), 
                                    labels["nodes2"], labels["nodes2#END"] - labels["nodes2"]);
                glBindBufferRange(  GL_SHADER_STORAGE_BUFFER, 2, SSBO->getHandle(), 0, 
                                    labels["environment#END"] - labels["environment"]);
                glDispatchCompute(nodecount, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                glBindBufferRange(  GL_SHADER_STORAGE_BUFFER, 1, SSBO->getHandle(), 
                                    labels["nodes1"], labels["nodes1#END"] - labels["nodes1"]);
                glBindBufferRange(  GL_SHADER_STORAGE_BUFFER, 0, SSBO->getHandle(), 
                                    labels["nodes2"], labels["nodes2#END"] - labels["nodes2"]);
                glDispatchCompute(nodecount, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }
        }

        //Draw spheres
        glBindVertexArray(VAO);
        basic_shader->Bind();
        basic_shader->SetUniform("u_color", color);
        basic_shader->SetUniform("u_color_mode", color_mode);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glBindBufferRange(  GL_SHADER_STORAGE_BUFFER, 0, SSBO->getHandle(), 
                            labels["nodes1"], labels["nodes1#END"] - labels["nodes1"]);
        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, circle_points+1, nodecount);

        //Draw lines
        glBindVertexArray(line_drawer);
        line_shader->Bind();
        glDrawArrays(GL_LINES, 0, 2 * env.lines);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        FrameMark;
        TracyGpuCollect;
        glfwSwapBuffers(window);
    }

    //Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &line_drawer);

    glfwDestroyWindow(window);
    glfwTerminate();
   
    return 0;
}

