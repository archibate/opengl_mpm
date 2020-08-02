#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define RES 512
#define N 256

float particles[N * 2 * 2];

const char vertex_shader_text[] =
"#version 330 core\n"
"in vec3 vPosition;\n"
"\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(vPosition, 1);\n"
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

const char compute_shader_text[] =
"#version 430 core\n"
"\n"
"layout (std430, binding = 0) buffer b_particles {\n"
"  vec2 pos[3];\n"
"  vec2 vel[3];\n"
"};\n"
"\n"
"layout (local_size_x = 3) in;\n"
"\n"
"void main()\n"
"{\n"
"  float dt = 0.02;\n"
"  uint i = gl_GlobalInvocationID.x;\n"
"  vel[i] -= vec2(0.0, 1.0) * dt;\n"
"  if (pos[i].x > +1 && vel[i].x > 0) vel[i].x *= -1;\n"
"  if (pos[i].x < -1 && vel[i].x < 0) vel[i].x *= -1;\n"
"  if (pos[i].y > +1 && vel[i].y > 0) vel[i].y *= -1;\n"
"  if (pos[i].y < -1 && vel[i].y < 0) vel[i].y *= -1;\n"
"  pos[i] += vel[i] * dt;\n"
"}\n"
;


int vao, vbo;
int render_program;

int ssbo_particles;
int program_substep1;
int program_substep2;


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
  bind_buffer_data(GL_ARRAY_BUFFER,
      vbo, sizeof(particles), particles, GL_STATIC_DRAW);

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
  program_substep1 = create_compute_shader(file_get_content("substep1.comp"));
  program_substep2 = create_compute_shader(file_get_content("substep2.comp"));
}

void init_compute_buffers(void)
{
  ssbo_particles = create_buffer();
  bind_buffer_data(GL_SHADER_STORAGE_BUFFER, ssbo_particles,
      sizeof(particles), particles, GL_DYNAMIC_READ);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_particles);
}

void do_compute(void)
{
  glUseProgram(program_substep1);
  glDispatchCompute(1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  glUseProgram(program_substep2);
  glDispatchCompute(1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles);
  void *p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
  memcpy(particles, p, sizeof(particles));
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
      vbo, sizeof(particles), particles, GL_STATIC_DRAW);
  glDrawArrays(GL_POINTS, 0, N);
}

void random_init(void)
{
  for (int i = 0; i < N * 2 * 2; i++) {
    particles[i] = (float)drand48() * 2 - 1;
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
