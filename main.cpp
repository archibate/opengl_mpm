#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define RES 512


struct {
  float x, y;
} vertices[3] = {
  -0.6f, -0.4f,
  0.6f, -0.4f,
  0.0f, 0.6f,
};

const char vertex_shader_text[] =
"#version 330 core\n"
"in vec2 vPosition;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(vPosition, 0, 1);\n"
"}\n"
;

const char fragment_shader_text[] =
"#version 330 core\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"  fragColor = vec4(1.0, 0.8, 0.2, 0.0);\n"
"}\n"
;


int load_shader(int type, const char *source) {
  int shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  int status = GL_TRUE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    char log[1024];
    int logLength = sizeof(log) - 1;
    glGetShaderInfoLog(shader, logLength, &logLength, log);
    log[logLength] = 0;
    fprintf(stderr, "error while compiling shader:\n%s\n%s\n", source, log);
  }
  return shader;
}

void init_shaders(void)
{
  int vertex_shader = load_shader(GL_VERTEX_SHADER, vertex_shader_text);
  int fragment_shader = load_shader(GL_FRAGMENT_SHADER, fragment_shader_text);
  int render_program = glCreateProgram();
  glAttachShader(render_program, vertex_shader);
  glAttachShader(render_program, fragment_shader);
  glLinkProgram(render_program);
  int status = GL_TRUE;
  glGetProgramiv(render_program, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    char log[1024];
    int logLength = sizeof(log) - 1;
    glGetProgramInfoLog(render_program, logLength, &logLength, log);
    log[logLength] = 0;
    fprintf(stderr, "error while linking shader:\n%s\n", log);
  }
  glUseProgram(render_program);
}


void error_callback(int error, const char *msg)
{
  fprintf(stderr, "error %d: %s\n", error, msg);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_SPACE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void display_callback(void)
{
    glClearColor(0.1f, 0.2f, 0.1f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.9f, 0.8f, 0.2f);
    glRectf(-0.5f, -0.5f, 0.5f, 0.5f);
}


int main(void)
{
  glfwInit();
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  GLFWwindow *window = glfwCreateWindow(RES, RES, "OpenGL Context", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSetErrorCallback(error_callback);
  glfwSetKeyCallback(window, key_callback);
  glewInit();
  glViewport(0, 0, RES, RES);

  init_shaders();

  while (!glfwWindowShouldClose(window)) {
    display_callback();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
