#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#define ROTATE_CAMERA true
#define ROTATE_LIGHT false

struct Vec3 {
  float x, y, z;

  Vec3() : x(0), y(0), z(0) {}

  Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

  Vec3 normalize() const {
    float mag = sqrt(x * x + y * y + z * z);
    return Vec3(x / mag, y / mag, z / mag);
  }

  Vec3 operator+(const Vec3 &v) const {
    return Vec3(x + v.x, y + v.y, z + v.z);
  }

  Vec3 operator-(const Vec3 &v) const {
    return Vec3(x - v.x, y - v.y, z - v.z);
  }

  Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }

  float dot(const Vec3 &v) const { return x * v.x + y * v.y + z * v.z; }
};

struct Ray {
  Vec3 origin;
  Vec3 direction;

  Ray(const Vec3 &o, const Vec3 &d) : origin(o), direction(d) {}
};

struct Sphere {
  Vec3 center;
  float radious;
  Vec3 color;

  Sphere(const Vec3 &c, float r, const Vec3 &col)
      : center(c), radious(r), color(col) {}

  bool intersect(const Ray &ray, float &t) const {
    Vec3 offset = ray.origin - center;
    float a = ray.direction.dot(ray.direction);
    float b = 2.0f * offset.dot(ray.direction);
    float c = offset.dot(offset) - pow(radious, 2.0f);

    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
      return false;
    }

    float sqrt_d = sqrt(discriminant);
    float t0 = (-b - sqrt_d) / (2 * a);
    float t1 = (-b + sqrt_d) / (2 * a);

    t = (t0 < t1 && t0 >= 0) ? t0 : t1;
    return t >= 0;
  }
};

void configure_terminal(int cols, int rows) {
  struct winsize ws;
  ws.ws_col = cols;
  ws.ws_row = rows;
  if (ioctl(STDOUT_FILENO, TIOCSWINSZ, &ws) == -1) {
    std::cerr << "Failed to set terminal size.\n";
  }

  std::cout << "\033]50;Terminal\007";
  std::cout << "\033[8;" << rows << ";" << cols << "t";
  struct termios t;
  tcgetattr(STDIN_FILENO, &t);
  t.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

Vec3 trace(const Ray &ray, const std::vector<Sphere> &spheres,
           const Vec3 &light) {

  float min_t = INFINITY;
  const Sphere *closest = nullptr;

  for (const auto &sphere : spheres) {
    float t;
    if (sphere.intersect(ray, t) && t < min_t) {
      min_t = t;
      closest = &sphere;
    }
  }
  if (!closest) {
    return Vec3(0.2f, 0.7f, 0.8f);
  }

  Vec3 hit_point = ray.origin + ray.direction * min_t;
  Vec3 normal = (hit_point - closest->center).normalize();
  Vec3 light_dir = (light - hit_point).normalize();

  float diffuse = std::max(0.0f, normal.dot(light_dir));

  Vec3 view_dir = (ray.origin - hit_point).normalize();
  Vec3 reflect_dir = light_dir - normal * 2.0f * normal.dot(light_dir);
  float specular = pow(std::max(0.0f, view_dir.dot(reflect_dir)), 32);

  return closest->color * (diffuse + 0.3f) + Vec3(1, 1, 1) * specular * 0.5f;
}

void clearScreen() {
  // ANSI escape code to clear screen and move cursor to top-left
  std::cout << "\033[2J\033[1;1H";
}

int main() {

  const int width = 80;
  const int height = 40;
  configure_terminal(width, height);
  const std::string gradient = " .:-=+*#%@";

  const float camera_distance = 4;
  Vec3 camera(0, 0, camera_distance);
  Vec3 light(-5, 5, 5);

  // std::cout << camera.z << std::endl;

  std::vector<Sphere> spheres = {
      Sphere(Vec3(0, 0, 0), 1, Vec3(1, 0.2f, 0.2f)),
      Sphere(Vec3(2, 0.5f, -1), 0.5f, Vec3(1, 0.2f, 0.2f)),
      Sphere(Vec3(-2, 0, 0), 0.8f, Vec3(1, 0.2f, 0.2f))};

  float angle = 0;

  std::vector<float> us;
  std::vector<float> vs;
  std::vector<float> zs;

  for (int y = 0; y < height; ++y) {
    float v1 = (height / 2.0f - y) / height;
    vs.push_back(v1);
  }

  for (int x = 0; x < width; ++x) {
    float u1 = (x - width / 2.0f) / width * 2.0f;

    us.push_back(u1);
  }

  while (true) {
    auto now = std::chrono::system_clock::now();
    std::this_thread::sleep_until(now + std::chrono::microseconds(100000));

    clearScreen();
    //
    // camera.z +=  0.2 * cos(angle);
    if (ROTATE_LIGHT) {
    light.x = (-5) * cos(angle) - 5 * sin(angle);
    light.z = (-5)* sin(angle) + 5 * cos(angle);
    }

    if (ROTATE_CAMERA) {
      camera.x = 0 * cos(angle) - camera_distance * sin(angle);
      camera.z = 0 * sin(angle) + camera_distance * cos(angle);
    }

    std::cout << "x: " << camera.x << " y: " << camera.y << " z: " << camera.z
              << std::endl;

    angle += 0.1;

    // spheres[0].center.y += sin(angle);
    // spheres[0].center.x += cos(angle);

    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        float u = (x - width / 2.0f) / width * 2.0f;
        float v = (height / 2.0f - y) / height;
        float z = -1;

        // float z = -1 * cos(angle) + us[x] * sin(angle);

        if (ROTATE_CAMERA) {
          u = us[x] * cos(angle) + sin(angle);

          v = vs[y];

          z = -1 * cos(angle) + us[x] * sin(angle);
        }

        //

        // std::cout << "u: " << u << std::endl;

        Vec3 direction(u, v, z);
        // Vec3 direction(u, v, z);

        Ray ray(camera, direction.normalize());

        // std::cout << "u: " << u << " v: " << v << " z: " << z << std::endl;
        Vec3 color = trace(ray, spheres, light);

        float luminance =
            0.2126f * color.x + 0.7152f * color.y + 0.0722f * color.z;

        luminance = std::clamp(luminance, 0.0f, 1.0f);

        int char_index = static_cast<int>(luminance * gradient.length() - 1);

        std::cout << gradient[char_index];
      }
      std::cout << "\n";
    }
  }
}
//}
