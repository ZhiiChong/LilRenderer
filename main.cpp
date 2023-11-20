#include <vector>
#include <cmath>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

Model *model = NULL;

const int WIDTH = 1024;
const int HEIGHT = 1024;

Vec3f barycentric(Vec3f *pts, Vec3f P)
{
    Vec3f u = cross(Vec3f(pts[2][0] - pts[0][0], pts[1][0] - pts[0][0], pts[0][0] - P[0]), Vec3f(pts[2][1] - pts[0][1], pts[1][1] - pts[0][1], pts[0][1] - P[1]));
    if (std::abs(u.z) <= 1e-2)
        return Vec3f(-1, 1, 1);
    return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}

void line(Vec2i t1, Vec2i t2, TGAImage &image, TGAColor color)
{
    int x0 = t1.x;
    int y0 = t1.y;
    int x1 = t2.x;
    int y1 = t2.y;
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1)
    {
        std::swap(t1, t2);
    }

    for (int x = x0; x <= x1; x++)
    {
        float t = (x - x0) / (float)(x1 - x0);
        int y = y0 * (1. - t) + y1 * t + .5;
        if (steep)
        {
            image.set(y, x, color);
        }
        else
        {
            image.set(x, y, color);
        }
    }
}

void triangle(Vec3f *pts, float *zbuffer, Vec2f *uv_coords, float intensity, TGAImage &image, TGAImage texture)
{
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }
    Vec3f P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
    {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
        {
            Vec3f bc_screen = barycentric(pts, P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                continue;
            P.z = 0;
            for (int i = 0; i < 3; i++) {
                P.z += pts[i][2] * bc_screen[i];
            }
            if (zbuffer[int(P.x + P.y * WIDTH)] < P.z) {
                zbuffer[int(P.x + P.y * WIDTH)] = P.z;
                Vec2f uv = uv_coords[0] * bc_screen[0] + uv_coords[1] * bc_screen[1] + uv_coords[2] * bc_screen[2];
                TGAColor color = texture.get(uv[0] * texture.get_width(), uv[1] * texture.get_height());
                color.r = color.r * intensity;
                color.g = color.g * intensity;
                color.b = color.b * intensity;
                image.set(P.x, P.y, color);
            }
        }
    }
}

void rasterize(Vec2i p0, Vec2i p1, TGAImage &image, TGAColor color, int ybuffer[]) {
    if (p0.x > p1.x)
    {
        std::swap(p0, p1);
    }
    for (int x = p0.x; x <= p1.x; x++)
    {
        float t = (x - p0.x) / (float)(p1.x - p0.x);
        int y = p0.y * (1. - t) + p1.y * t;
        if (ybuffer[x] < y)
        {
            ybuffer[x] = y;
            image.set(x, 0, color);
        }
    }
}

Vec3f world2screen(Vec3f v) {
    return Vec3f(int((v.x + 1.) * WIDTH / 2. + .5), int((v.y + 1.) * HEIGHT / 2. + .5), v.z);
}

int main(int argc, char **argv) {
    if (2 == argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head/african_head.obj");
    }

    float *zbuffer = new float[WIDTH * HEIGHT];
    for (int i = WIDTH * HEIGHT; i--; zbuffer[i] = -std::numeric_limits<float>::max());
    TGAImage image(WIDTH, HEIGHT, TGAImage::RGB);
    TGAImage texture(WIDTH, HEIGHT, TGAImage::RGB);
    texture.read_tga_file("obj/african_head/african_head_diffuse.tga");
    Vec3f light_dir(0, 0, -1);
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f pts[3], world_coords[3];
        Vec2f uv_coords[3];
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[i]);
            world_coords[j] = v;
            pts[j] = world2screen(v);
            uv_coords[j] = model->uv_vert((i - 1) * 3 + j);
        }
        Vec3f n = cross((world_coords[2] - world_coords[0]), (world_coords[1] - world_coords[0]));
        n.normalize();
        float intensity = n * light_dir;
        // TGAColor random = TGAColor(rand() % 255 * intensity, rand() % 255 * intensity, rand() % 255 * intensity, 255);
        // TGAColor white = TGAColor(255 * intensity, 255 * intensity, 255 * intensity, 255);
        triangle(pts, zbuffer, uv_coords, intensity, image, texture);
    }

    image.flip_vertically();
    image.write_tga_file("output.tga");
    delete model;

    return 0;
}