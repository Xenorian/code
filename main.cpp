#include <opencv2/highgui/highgui_c.h>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
class Poly {
  int label;
  std::vector<cv::Point2f> points;
};
class Img {
  std::string file;
  cv::Mat content;
  std::vector<int> pixel_type;
  std::vector<Poly> poly_list;
  void delete_poly(int index);
  void add_poly();
};

void Img::delete_poly(int index) {
  // todo::删除多边形
}
void Img::add_poly() {
  // todo::添加多边形
}
std::vector<cv::Point2f> control_points;

void mouse_handler(int event, int x, int y, int flags, void *userdata) {
  if (event == cv::EVENT_LBUTTONDOWN) {
    std::cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << '\n';
    control_points.emplace_back(x + 0.5, y + 0.5);
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

void init_img(const std::string &file, Img &img) {
  // todo::初始化img

  // read image

  // read pixel - type: files+pmap.txt

  // read poly-list: json格式
}
void get_files(std::string dir, std::vector<std::string> &files) {
  // todo::拿到所有文件
}
int main(int argc, const char **argv) {
  if (argc != 2) {
    return 0;
  }
  //
  std::string dir = argv[1];
  std::vector<std::string> files;

  get_files(dir, files);

  // init window
  cv::namedWindow("pic", CV_WINDOW_NORMAL);
  cv::setMouseCallback("pic", mouse_handler, nullptr);

  // read image
  cv::Mat img = cv::imread(files[0]);
  if (!img.data) {
    std::cerr << "invalid picture path" << std::endl;
    return 1;
  }

  // read pixel - type: files+pmap.txt
  std::vector<int> pixel_type(img.rows * img.cols, 0);
  std::cout << "size: " << img.cols << " * " << img.rows << std::endl;

  // read poly-list: json格式

  int key = -1;
  while (key != 27) {
    for (auto &point : control_points) {
      cv::circle(img, point, 1, {255, 255, 255}, 3);
    }

    cv::imshow("pic", img);
    key = cv::waitKey(20);

    if (key == 's') {
      std::cout << "record type" << std::endl;
      int type = -1;
      std::cin >> type;
      record_pixel_type(img, pixel_type, type);
      control_points.clear();
    }
    // 上下键切换图片
    if (key == ' ') {
    }
    if (key == ' ') {
    }
  }

  // write file
  FILE *ofile = NULL;
  char *Buffer = new char[pixel_type.size() * 10];
  memset(Buffer, '0', sizeof(char) * pixel_type.size() * 10);

  int cnt = 0;
  int i = 0;
  ofile = fopen("output.txt", "w");

  for (; cnt < pixel_type.size(); cnt++) {
    Buffer[i] = '0' + pixel_type[cnt];
    i++;
    if (cnt % img.cols == img.cols - 1) {
      Buffer[i] = '\n';
      i++;
    }
  }
  fwrite(Buffer, sizeof(char), i, ofile);
  fclose(ofile);
  delete[] Buffer;

  return 0;
}