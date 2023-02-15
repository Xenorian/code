#include <opencv2/highgui/highgui_c.h>
#include <stdio.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "json/json.h"
#include "stdlib.h"
#define GL_SILENCE_DEPRECATION

#include <GLFW/glfw3.h>  // Will drag system OpenGL headers
const int color_num = 15;
std::vector<cv::Point2f> control_points;
class Color {
 public:
  float a;
  float b;
  float c;
  bool operator==(const Color &p) { return a == p.a && b == p.b && c == p.c; }
  Color() {}
  Color(float _a, float _b, float _c) {
    a = _a;
    b = _b;
    c = _c;
  }
};
class Poly {
 public:
  int label;
  std::vector<cv::Point2f> points;
  Color c;
};
class Img {
 public:
  std::string file;
  cv::Mat content;
  std::vector<int> pixel_type;
  std::vector<Poly> poly_list;
  void delete_poly(int index);
  void write_file(const std::string &content, const std::string &filename);
  void add_poly(int type);
  void output();
  ImVec2 window_pos;
  ImVec2 window_size;
  int index;
};

std::list<Color> free_color;
std::list<Color> used_color;

void Img::delete_poly(int index) {
  // todo::删除多边形
  free_color.push_back(poly_list[index].c);
  auto this_color = used_color.begin();
  for (auto it = used_color.begin(); it != used_color.end(); it++) {
    if (*it == poly_list[index].c) {
      this_color = it;
      break;
    }
  }
  used_color.erase(this_color);
  poly_list.erase(poly_list.begin() + index);
}
void Img::add_poly(int type) {
  // todo::添加多边形
  if (free_color.size() == 0) {
    std::cerr << "color num < poly number" << std::endl;
    exit(1);
  }
  Poly p;
  p.c = *free_color.begin();
  used_color.push_back(p.c);
  free_color.pop_front();
  p.label = type;
  for (int i = 0; i < control_points.size(); i++) {
    p.points.push_back(control_points[i]);
  }
  poly_list.push_back(p);
}
void Img::output() {
  // pixel type map
  FILE *ofile = NULL;
  char *Buffer = new char[pixel_type.size()];
  memset(Buffer, 0, sizeof(char) * pixel_type.size());

  ofile = fopen(("../data/" + file + "pixel_map.txt").c_str(), "w");

  for (int i = 0; i < pixel_type.size(); i++) {
    Buffer[i] = pixel_type[i];
  }
  fwrite(Buffer, sizeof(char), pixel_type.size(), ofile);
  fclose(ofile);
  delete[] Buffer;

  // json for poly-list
  Json::Value root;
  Json::Value poly;
  Json::Value point;
  for (int i = 0; i < poly_list.size(); i++) {
    Poly p = poly_list[i];
    poly.resize(0);
    // add each point to poly
    for (int j = 0; j < p.points.size(); j++) {
      point["x"] = std::to_string(p.points[j].x);
      point["y"] = std::to_string(p.points[j].y);
      poly.append(point);
    }
    root[std::to_string(p.label)].append(poly);
  }

  Json::FastWriter writer;
  std::string poly_list_str = writer.write(root);
  write_file(poly_list_str, "../data/" + file + "poly_list.json");
}
void Img::write_file(const std::string &content, const std::string &filename) {
  FILE *ofile = NULL;
  char *Buffer = new char[content.size()];
  for (int i = 0; i < content.size(); i++) {
    Buffer[i] = content[i];
  }

  ofile = fopen("filename", "w");

  fwrite(Buffer, sizeof(char), content.size(), ofile);
  fclose(ofile);
  delete[] Buffer;
}
void get_files(std::string dir, std::vector<std::string> &files);
int intersect(const cv::Point2f &a, const cv::Point2f &b, const cv::Point2f &c);
bool inside_circle(const std::vector<cv::Point2f> &control_points, const cv::Point2f &point);
void record_pixel_type(const cv::Mat &img, std::vector<int> &res, int type);
void init_img(const std::string &filename, Img &img);

Img img;
std::vector<std::string> files;

ImGuiIO *io = nullptr;

void mouse_handler(GLFWwindow *window, int button, int action, int mods) {
  static double ox, oy;
  if (action == GLFW_PRESS) {
    ox = io->MousePos.x;
    oy = io->MousePos.y;
  } else if (action == GLFW_RELEASE) {
    if (io->MousePos.x == ox && io->MousePos.y == oy) {
      double relative_x = io->MousePos.x - img.window_pos.x;
      double relative_y = io->MousePos.y - img.window_pos.y - 18;
      if (relative_x >= 0 && relative_x <= img.window_size.x && relative_y >= 0 && relative_y <= img.window_size.y) {
        std::cout << "Left button of the mouse is clicked - position (" << relative_x << ", " << relative_y << ")"
                  << '\n';
        control_points.emplace_back(relative_x - 7, relative_y - 7);
      }
    }
  }
}

void keyboard_handler(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_UP) {
      if (img.index == 0) {
        init_img(files[files.size() - 1], img);
        img.index = files.size() - 1;
      } else {
        init_img(files[img.index - 1], img);
        img.index--;
      }
      img.output();
      control_points.clear();
    } else if (key == GLFW_KEY_DOWN) {
      if (img.index == files.size() - 1) {
        init_img(files[0], img);
        img.index = 0;
      } else {
        init_img(files[img.index + 1], img);
        img.index++;
      }
      img.output();
      control_points.clear();
    } else if (key == GLFW_KEY_S) {
      // 保存
      std::cout << "record type" << std::endl;
      int type = -1;
      std::cin >> type;
      record_pixel_type(img.content, img.pixel_type, type);
      img.add_poly(type);
      control_points.clear();
    }
  }
}

// 0 不相交 1相交 -1在多边形边上
int intersect(const cv::Point2f &a, const cv::Point2f &b, const cv::Point2f &c) {
  if (a.x == b.x && a.y == b.y) {
    std::cerr << "key point duplicated\n";
  }

  // 多边形水平边不考虑
  if (a.y == b.y) {
    float min_x = a.x < b.x ? a.x : b.x;
    float max_x = a.x < b.x ? b.x : a.x;
    if (c.x >= min_x && c.x <= max_x && c.y == a.y) {
      return -1;
    } else {
      return 0;
    }
  }

  float intersect_x = b.x + (c.y - b.y) * (a.x - b.x) / (a.y - b.y);
  float min_y = a.y < b.y ? a.y : b.y;
  float max_y = a.y < b.y ? b.y : a.y;
  // 在线段上
  if (c.y < min_y || c.y > max_y) {
    return 0;
  }

  // 在多边形边上
  if (intersect_x == c.x) {
    return -1;
  }

  // 作射线朝x轴正方向
  if (intersect_x > c.x) {
    // 若与顶点相交，只取纵坐标较大的
    if (intersect_x == a.x && c.y == a.y) {
      return a.y > b.y ? 1 : 0;
    } else if (intersect_x == b.x && c.y == b.y) {
      return a.y > b.y ? 0 : 1;
    }

    return 1;
  } else {
    return 0;
  }
}

bool inside_circle(const std::vector<cv::Point2f> &control_points, const cv::Point2f &point) {
  int intersec_count = 0;
  for (int i = 0; i < control_points.size(); i++) {
    cv::Point2f p1(control_points[i]);
    cv::Point2f p2(control_points[(i + 1) % control_points.size()]);

    int ret_val = intersect(p1, p2, point);
    if (ret_val == 1) {
      intersec_count++;
    } else if (ret_val == -1) {
      // 在边上，认为在多边形内
      return true;
    }
  }
  return intersec_count % 2 == 1 ? true : false;
}
void record_pixel_type(const cv::Mat &img, std::vector<int> &res, int type) {
  for (int x = 0; x < img.cols; x++) {
    for (int y = 0; y < img.rows; y++) {
      if (inside_circle(control_points, cv::Point2f(x + 0.5, y + 0.5))) {
        res[y * img.rows + x] = type;
      }
    }
  }
}

void init_img(const std::string &filename, Img &img) {
  // todo::初始化img
  img.file = filename;

  // read image
  img.content = cv::imread(filename);
  cv::cvtColor(img.content, img.content, cv::COLOR_BGR2RGBA);
  if (!img.content.data) {
    std::cerr << "invalid picture path" << std::endl;
    exit(1);
  }
  // read pixel - type: files+pmap.txt
  img.pixel_type.resize(img.content.rows * img.content.cols);
  FILE *infile = NULL;
  char *Buffer = new char[img.pixel_type.size()];
  memset(Buffer, 0, sizeof(char) * img.pixel_type.size());

  infile = fopen(("../data/" + filename + "pixel_map.txt").c_str(), "r");
  if (!infile) {
    std::cout << "no pixel_map.txt" << std::endl;
  } else {
    fread(Buffer, sizeof(char), img.pixel_type.size(), infile);
    fclose(infile);
  }

  for (int i = 0; i < img.pixel_type.size(); i++) {
    Buffer[i] = img.pixel_type[i];
  }

  delete[] Buffer;

  // read poly-list: json格式
  // type:[[point1,point2...],[]...]
  Json::Reader reader;
  std::ifstream ifile("../data/" + filename + "poly-list.json", std::ios::binary);
  if (!ifile.is_open()) {
    std::cout << filename << ": no poly-list " << std::endl;
    return;
  }
  Json::Value root;
  Json::Value poly;
  Json::Value point;
  if (reader.parse(ifile, root)) {
    Json::Value::Members mem = root.getMemberNames();
    for (auto it = mem.begin(); it != mem.end(); it++) {
      std::string label = *it;
      for (int i = 0; i < root[label].size(); i++) {
        // for each poly of the same label
        Poly p;
        p.label = atoi(label.c_str());
        for (int j = 0; j < root[label][i].size(); i++) {
          // for each point
          cv::Point2f point(root[label][i][j]["x"].asFloat(), root[label][i][j]["y"].asFloat());
          p.points.push_back(point);
        }
        img.poly_list.push_back(p);
      }
    }
  } else {
    std::cout << "parse error" << std::endl;
  }
  ifile.close();

  if (img.poly_list.size() > color_num) {
    std::cout << "too many poly > color_num" << std::endl;
    exit(1);
  }
  for (int i = 0; i < img.poly_list.size(); i++) {
    img.poly_list[i].c = *free_color.begin();
    free_color.pop_front();
    used_color.push_back(img.poly_list[i].c);
  }
  for (int j = 0; j < (int)img.poly_list.size() - 1; j++) {
    Poly p = img.poly_list[j];
    for (int i = 0; i < (int)p.points.size() - 1; i++) {
      cv::line(img.content, p.points[i], p.points[i + 1], {p.c.a * 255, p.c.b * 255, p.c.c * 255});
    }
    if (p.points.size() > 1) {
      cv::line(img.content, p.points[p.points.size() - 1], p.points[0], {p.c.a * 255, p.c.b * 255, p.c.c * 255});
    }
  }
}

void get_files(std::string dir, std::vector<std::string> &files) {
  files.clear();

  // 打开指定目录，遍历该文件夹下所有文件和文件夹
  // 指定目录下最好不包含文件夹
  std::filesystem::path target_path(dir);
  auto iterator = std::filesystem::directory_iterator(target_path);
  for (auto &entry : iterator) {
    files.emplace_back(entry.path().string());
  }

  // 文件路径肯定不一样，不用纠结稳定性的问题
  std::sort(files.begin(), files.end());

  // for (auto &a : files) {
  //   std::cout << a << std::endl;
  // }
}

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
void initialize_color(int num) {
  for (int i = 0; i < num; i++) {
    free_color.push_back(Color(float(i) / (num + 5), 0.5f, 0.5f));
  }
}
int main(int argc, const char **argv) {
  if (argc != 2) {
    return 0;
  }
  // color initialize
  initialize_color(color_num);

  std::string dir = argv[1];

  get_files(dir, files);
  if (files.size() == 0) {
    return 0;
  }

  // img now
  init_img(files[0], img);
  img.index = 0;

  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) return 1;

  // Decide GL+GLSL versions
  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

  // Create window with graphics context
  GLFWwindow *window = glfwCreateWindow(1280, 1080, "Label", NULL, NULL);
  if (window == NULL) return 1;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);  // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  io = &(ImGui::GetIO());
  (void)(*io);
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Our state
  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // Add callback function
  glfwSetKeyCallback(window, keyboard_handler);
  glfwSetMouseButtonCallback(window, mouse_handler);

  // Main loop
#ifdef __EMSCRIPTEN__
  // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini
  // file. You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
  io.IniFilename = NULL;
  EMSCRIPTEN_MAINLOOP_BEGIN
#else
  while (!glfwWindowShouldClose(window))
#endif
  {
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to
    // learn more about Dear ImGui!).
    if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
      static float f = 0.0f;
      static int counter = 0;

      ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!" and append into it.

      ImGui::Text("This is some useful text.");           // Display some text (you can use a format strings too)
      ImGui::Checkbox("Demo Window", &show_demo_window);  // Edit bools storing our window open/close state
      ImGui::Checkbox("Another Window", &show_another_window);

      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);              // Edit 1 float using a slider from 0.0f to 1.0f
      ImGui::ColorEdit3("clear color", (float *)&clear_color);  // Edit 3 floats representing a color

      if (ImGui::Button("Button"))  // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
      ImGui::SameLine();
      ImGui::Text("counter = %d", counter);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                  ImGui::GetIO().Framerate);
      ImGui::End();
    }

    // 图片窗口
    {
      ImGui::Begin("img");  // Create a window called "Hello, world!" and append into it.

      ImGui::SetWindowSize(ImVec2{(float)(img.content.cols + 20), (float)(img.content.rows + 40)});

      img.window_pos = ImGui::GetWindowPos();
      img.window_size = ImGui::GetWindowSize();

      for (auto &point : control_points) {
        cv::circle(img.content, point, 1, {255, 255, 255}, 3);
      }

      for (int i = 0; i < (int)control_points.size() - 1; i++) {
        cv::line(img.content, control_points[i], control_points[i + 1], {255, 255, 255});
      }

      GLuint texture;
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.content.cols, img.content.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                   img.content.data);
      ImGui::Image(reinterpret_cast<void *>(static_cast<intptr_t>(texture)),
                   ImVec2(img.content.cols, img.content.rows));
      ImGui::End();
    }
    //
    {
      ImGui::Begin("Img Info");
      const std::string name = "delete##";
      for (int i = 0; i < img.poly_list.size(); i++) {
        ImGui::PushID(i);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(i / 7.0f, 0.8f, 0.8f));
        if (ImGui::Button((name + std::to_string(i)).c_str())) {
          img.delete_poly(i);
          // std::cout << "delete!" << i << std::endl;
        }
        ImGui::PopStyleColor(3);
        ImGui::PopID();
      }
      ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window) {
      ImGui::Begin("Another Window",
                   &show_another_window);  // Pass a pointer to our bool variable (the window will have a closing button
                                           // that will clear the bool when clicked)
      ImGui::Text("Hello from another window!");
      if (ImGui::Button("Close Me")) show_another_window = false;
      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                 clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  exit(0);
}
