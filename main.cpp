#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define _HOST
#include "glcommon.comp"
#undef _HOST

#define RES 512
#define NSSTP 4
#define NSSBO 4

float b_x[N * 2];
float b_v[N * 2];
float b_C[N * 4];
float b_J[N];
float b_grid_v[G * G * 2];
float b_grid_m[G * G];

int ssbos[NSSBO];
int substeps[NSSTP];


const char vertex_shader_text[] =
"#version 330 core\n"
"in vec3 vPosition;\n"
"\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(vPosition * 2 - 1, 1);\n"
"  gl_PointSize = 4;\n"
"}\n"
;

const char fragment_shader_text[] =
"#version 330 core\n"
"out vec4 fragColor;\n"
"\n"
"void main()\n"
"{\n"
"  fragColor = vec4(1.0, 0.8, 0.2, 0.0);\n"
"}\n"
;

int vao, vbo;
int render_program;


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

void link_program(int program) {
  glLinkProgram(program);
  int status = GL_TRUE;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    char log[1024];
    int logLength = sizeof(log) - 1;
    glGetProgramInfoLog(program, logLength, &logLength, log);
    log[logLength] = 0;
    fprintf(stderr, "error while linking shader:\n%s\n", log);
  }
}

int create_buffer(void)
{
  GLuint buf;
  glGenBuffers(1, &buf);
  return buf;
}

void bind_buffer_data(int type, int buf, size_t size, void *data, int access)
{
  glBindBuffer(type, buf);
  glBufferData(type, size, data, access);
}

void init_render_shaders(void)
{
  int vertex_shader = load_shader(GL_VERTEX_SHADER, vertex_shader_text);
  int fragment_shader = load_shader(GL_FRAGMENT_SHADER, fragment_shader_text);
  render_program = glCreateProgram();
  glAttachShader(render_program, vertex_shader);
  glAttachShader(render_program, fragment_shader);
  link_program(render_program);
  glUseProgram(render_program);
}

void init_render_buffers(void)
{
  vbo = create_buffer();
  bind_buffer_data(GL_ARRAY_BUFFER, vbo, sizeof(b_x), b_x, GL_STATIC_DRAW);

  glGenVertexArrays(1, (GLuint *)&vao);
  glBindVertexArray(vao);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
}

int create_compute_shader(const char *source)
{
  int shader = load_shader(GL_COMPUTE_SHADER, source);
  int program = glCreateProgram();
  glAttachShader(program, shader);
  link_program(program);
  return program;
}

char *file_get_content(const char *name)
{
  FILE *fp = fopen(name, "r");
  if (!fp) {
    perror(name);
    return NULL;
  }
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  char *buf = (char *)malloc(len + 1);
  fseek(fp, 0, SEEK_SET);
  fread(buf, len, 1, fp);
  buf[len] = 0;
  return buf;
}

void init_compute_shaders(void)
{
  char name[512];
  strcpy(name, "/glcommon.h");
  char *header = file_get_content("glcommon.comp");
  glNamedStringARB(GL_SHADER_INCLUDE_ARB, strlen(name), name,
      strlen(header), header);
  for (int i = 0; i < NSSTP; i++) {
    sprintf(name, "substep%d.comp", i);
    substeps[i] = create_compute_shader(file_get_content(name));
  }
}

int create_ssbo_buffer(int i, void *data, size_t size)
{
  ssbos[i] = create_buffer();
  bind_buffer_data(GL_SHADER_STORAGE_BUFFER, ssbos[i],
      size, data, GL_DYNAMIC_READ);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, ssbos[i]);
  return ssbos[i];
}

void init_compute_buffers(void)
{
  create_ssbo_buffer(0, b_x, sizeof(b_x));
  create_ssbo_buffer(1, b_v, sizeof(b_v));
  create_ssbo_buffer(2, b_C, sizeof(b_C));
  create_ssbo_buffer(3, b_J, sizeof(b_J));
  create_ssbo_buffer(4, b_grid_v, sizeof(b_grid_v));
  create_ssbo_buffer(5, b_grid_m, sizeof(b_grid_m));
}

void do_compute(void)
{
  glUseProgram(substeps[0]);
  glDispatchCompute(G0, G0, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  glUseProgram(substeps[1]);
  glDispatchCompute(N0, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  glUseProgram(substeps[2]);
  glDispatchCompute(G0, G0, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  glUseProgram(substeps[3]);
  glDispatchCompute(N0, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[0]);
  void *map_x = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
  memcpy(b_x, map_x, sizeof(b_x));
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
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
  do_compute();
  glPointSize(4);
  glClearColor(0.1f, 0.2f, 0.1f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(render_program);
  bind_buffer_data(GL_ARRAY_BUFFER,
      vbo, sizeof(b_x), b_x, GL_STATIC_DRAW);
  glDrawArrays(GL_POINTS, 0, N);
}

void random_init(void)
{
  for (int i = 0; i < N; i++) {
    b_x[i * 2 + 0] = (float)drand48() * 0.4 + 0.2;
    b_x[i * 2 + 1] = (float)drand48() * 0.4 + 0.2;
    b_v[i * 2 + 1] = -1.0;
    b_J[i] = 1.0;
  }
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

  random_init();
  init_compute_shaders();
  init_compute_buffers();
  init_render_shaders();
  init_render_buffers();

  while (!glfwWindowShouldClose(window)) {
    display_callback();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
