#include "sav1.h"

#ifdef _WIN32
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>

// if set to 1, the video will be mapped onto a sphere, if set to 0 it will be
// mapped onto a cylinder
#define USE_SPHERE 0

GLFWwindow *window;

double x_mouse = 0;
double y_mouse = 0;
double x_rot = 0;
double y_rot = 0;
double scale = 1.0;

const double MIN_SCALE = 0.1;
const double SCALE_FACTOR = 0.05;
const double ROT_FACTOR = 0.01;

// read a GLSL shader from a file, compile it, and attach it to a program
GLuint
load_shader(const char *file_path, GLenum shader_type, GLuint shader_program)
{
    GLuint shader_id = glCreateShader(shader_type);

    // read the shader code from the specified file into a string
    std::ifstream shader_stream(file_path, std::ios::in);
    if (!shader_stream.is_open()) {
        std::cerr << "Failed to read shader file: " << file_path << std::endl;
        return 0;
    }
    std::string shader_code((std::istreambuf_iterator<char>(shader_stream)),
                            std::istreambuf_iterator<char>());
    shader_stream.close();

    // attempt to compile the shader code
    GLint compiled_successfully = GL_FALSE;
    int info_log_length;
    char const *const_shader_code = shader_code.c_str();

    glShaderSource(shader_id, 1, &const_shader_code, NULL);
    glCompileShader(shader_id);

    // check if the compilation was successful
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_successfully);
    if (compiled_successfully == GL_FALSE) {
        // print the error message
        std::cerr << "Failed to compile shader file: " << file_path << std::endl;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
        char *error_message = new char[info_log_length + 1];
        glGetShaderInfoLog(shader_id, info_log_length, NULL, error_message);
        std::cerr << error_message << std::endl;
        delete[] error_message;
        return 0;
    }

    // attach the shader to the program
    glAttachShader(shader_program, shader_id);

    return shader_id;
}

// callback to run whenever the mouse moves
void
mouse_callback(GLFWwindow *window, double x_pos, double y_pos)
{
    // calculate offset from previous position
    double dx = x_pos - x_mouse;
    double dy = y_pos - y_mouse;

    // adjust rotation if left mouse button is pressed
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        x_rot += (ROT_FACTOR * dy);
        y_rot += (ROT_FACTOR * dx);
    }

    // save new position as previous
    x_mouse = x_pos;
    y_mouse = y_pos;
}

// callback to run whenever the mouse scrolls
void
scroll_callback(GLFWwindow *window, double x_offset, double y_offset)
{
    // adjust scale
    scale -= SCALE_FACTOR * y_offset;

    // don't let scale go below MIN_SCALE
    if (scale < MIN_SCALE) {
        scale = MIN_SCALE;
    }
}

// create texture-mapped geometry for a sphere. This is the bad kind of sphere
// generation and mapping, but for these purposes it doesn't matter
int
create_sphere_geometry(GLuint *vertex_buffer, GLuint *uv_buffer)
{
    int num_subdivisions = 20;
    int num_points = (num_subdivisions + 1) * num_subdivisions;
    float radius = 1.0;

    float vertex_data[num_points * 3];
    float uv_data[num_points * 2];

    // scalar to convert i or j to an angle in radians
    float conversion_factor = 6.283184 / (float)num_subdivisions;

    // iterate through the vertical angles
    for (int i = 0; i < num_subdivisions / 2; i++) {
        // calculate the vertical information for this row and the next one, because we
        // are doing this as a triangle strip
        float phi = i * conversion_factor;
        float next_phi = (i + 1) * conversion_factor;
        float y = radius * cosf(phi);
        float next_y = radius * cosf(next_phi);
        float h_scale = radius * sinf(phi);
        float next_h_scale = radius * sinf(next_phi);

        // iterate through the horizontal angles
        for (int j = 0; j <= num_subdivisions; j++) {
            float theta = -j * conversion_factor;
            // calculate the current index into the two arrays
            int vertex_index = i * (num_subdivisions + 1) * 6 + j * 6;
            int uv_index = i * (num_subdivisions + 1) * 4 + j * 4;

            // point 1
            vertex_data[vertex_index] = h_scale * cosf(theta);
            vertex_data[vertex_index + 1] = y;
            vertex_data[vertex_index + 2] = h_scale * sinf(theta);
            uv_data[uv_index] = j / (float)num_subdivisions;
            uv_data[uv_index + 1] = i * 2.0 / (float)num_subdivisions;

            // point 2
            vertex_data[vertex_index + 3] = next_h_scale * cosf(theta);
            vertex_data[vertex_index + 4] = next_y;
            vertex_data[vertex_index + 5] = next_h_scale * sinf(theta);
            uv_data[uv_index + 2] = j / (float)num_subdivisions;
            uv_data[uv_index + 3] = (i + 1) * 2.0 / (float)num_subdivisions;
        }
    }

    // attach this data to the OpenGL attribute buffers
    glGenBuffers(1, vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, *vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glGenBuffers(1, uv_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, *uv_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uv_data), uv_data, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

    return num_points;
}

int
create_cylinder_geometry(GLuint *vertex_buffer, GLuint *uv_buffer)
{
    int num_points = 42;
    float radius = 1.0;

    float vertex_data[num_points * 3];
    float uv_data[num_points * 2];

    // scalar to convert i to an angle in radians
    float conversion_factor = 6.283184 / (float)(num_points / 2 - 1);

    for (int i = 0; i < num_points / 2; i++) {
        float angle = -i * conversion_factor;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);

        // point 1
        vertex_data[i * 6] = x;
        vertex_data[i * 6 + 1] = 1.0;
        vertex_data[i * 6 + 2] = z;
        uv_data[i * 4] = i * 2.0 / (num_points - 2);
        uv_data[i * 4 + 1] = 0.0;

        // point 2
        vertex_data[i * 6 + 3] = x;
        vertex_data[i * 6 + 4] = -1.0;
        vertex_data[i * 6 + 5] = z;
        uv_data[i * 4 + 2] = i * 2.0 / (num_points - 2);
        uv_data[i * 4 + 3] = 1.0;
    }

    // attach this data to the OpenGL attribute buffers
    glGenBuffers(1, vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, *vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glGenBuffers(1, uv_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, *uv_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uv_data), uv_data, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

    return num_points;
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Error: no input file specified" << std::endl;
        exit(1);
    }

    // initialize GLFW
    if (glfwInit() == GL_FALSE) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return 1;
    }

    // select OpenGL version
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // define window size
    int width = 1000;
    int height = 760;

    // create a window
    GLFWwindow *window;
    if ((window = glfwCreateWindow(width, height, "SAV3DPLAY", 0, 0)) == 0) {
        std::cerr << "Failed to open window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

#ifdef _WIN32
    // initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return 1;
    }
#endif

    // make sure the escape key can be captured
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // setup mouse callbacks
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // compile and link shaders
    GLuint shader_program = glCreateProgram();
    GLuint fragment_shader =
        load_shader("shader.frag", GL_FRAGMENT_SHADER, shader_program);
    GLuint vertex_shader = load_shader("shader.vert", GL_VERTEX_SHADER, shader_program);
    if (fragment_shader == 0 || vertex_shader == 0) {
        glfwTerminate();
        return 1;
    }
    glLinkProgram(shader_program);

    // set some OpenGL parameters
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glShadeModel(GL_FLAT);
    glDisable(GL_FOG);

    // set background color to SAV1 Blue™
    glClearColor(0.404f, 0.608f, 0.796f, 0.0f);

    // create the vertex array
    GLuint vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    // get a handle for the ModelViewProjection uniform
    GLuint matrix_id = glGetUniformLocation(shader_program, "MVP");

    // projection matrix
    glm::mat4 projection =
        glm::perspective(glm::radians(45.0f), 1000.0f / 760.0f, 0.1f, 100.0f);
    // camera matrix
    glm::mat4 view =
        glm::lookAt(glm::vec3(-5, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    // model matrix
    glm::mat4 model = glm::mat4(1.0f);
    // multiply to create ModelViewProjection
    glm::mat4 mvp = projection * view * model;

    // create the texture to store the video data
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    // create some dummy 5x5 texture data until we can get a frame from SAV1
    void *data[100] = {0};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 5, 5, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    // get a handle for the texture sampler uniform
    GLuint texture_sampler = glGetUniformLocation(shader_program, "tex_sampler");
    // set the sampler to use texture unit 0
    glUniform1i(texture_sampler, 0);

    // fill the vertex buffer with geometry and the uv buffer with texture mappings
    GLuint vertex_buffer;
    GLuint uv_buffer;
    int num_vertices;
    if (USE_SPHERE) {
        num_vertices = create_sphere_geometry(&vertex_buffer, &uv_buffer);
    }
    else {
        num_vertices = create_cylinder_geometry(&vertex_buffer, &uv_buffer);
    }

    // setup SAV1 to play the specified video
    Sav1Settings settings;
    sav1_default_settings(&settings, argv[1]);
    settings.codec_target = SAV1_CODEC_AV1;     // only worry about video, not audio
    settings.on_file_end = SAV1_FILE_END_LOOP;  // set the video to loop

    Sav1Context context = {0};
    sav1_create_context(&context, &settings);
    if (sav1_start_playback(&context)) {
        std::cerr << "Failed to initialize SAV1: " << sav1_get_error(&context)
                  << std::endl;
        glfwTerminate();
        return 1;
    }

    // main loop
    while (!(glfwWindowShouldClose(window) ||
             glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)) {
        // get a video frame from SAV1 if possible
        int frame_ready;
        sav1_get_video_frame_ready(&context, &frame_ready);
        if (frame_ready) {
            Sav1VideoFrame *frame;
            sav1_get_video_frame(&context, &frame);

            // update the OpenGL texture
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame->width, frame->height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, (void *)frame->data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        // clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // use the shader
        glUseProgram(shader_program);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // rotate and scale the model
        model = glm::rotate(glm::mat4(1.0f), (float)x_rot, glm::vec3(0, 0, 1));
        model = glm::rotate(model, (float)y_rot, glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(scale, scale, scale));

        glEnable(GL_NORMALIZE);

        // update ModelViewProjection and send to the shader
        mvp = projection * view * model;
        glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &mvp[0][0]);

        // enable vertex and UV attribute buffers
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        // draw the actual model geometry
        glDrawArrays(GL_TRIANGLE_STRIP, 0, num_vertices);

        // disable attribute buffers
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        // swap display buffers
        glfwSwapBuffers(window);
        glFlush();

        glfwPollEvents();
    }

    // clean up OpenGL
    glDetachShader(shader_program, fragment_shader);
    glDetachShader(shader_program, vertex_shader);
    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);

    glDeleteBuffers(1, &vertex_buffer);
    glDeleteBuffers(1, &uv_buffer);
    glDeleteProgram(shader_program);
    glDeleteTextures(1, &texture);
    glDeleteVertexArrays(1, &vertex_array);

    glfwDestroyWindow(window);
    glfwTerminate();

    // clean up sav1
    sav1_stop_playback(&context);
    sav1_destroy_context(&context);

    return 0;
}
