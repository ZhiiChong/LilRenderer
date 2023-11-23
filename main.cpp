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
const TGAColor yellow = TGAColor(255, 255, 0, 255);

Model *model = NULL;

const int OUTPUT_WIDTH = 800;
const int OUTPUT_HEIGHT = 800;
const int TEXTURE_WIDTH = 1024;
const int TEXTURE_HEIGHT = 1024;
const int DEPTH = 255;

Vec3f barycentric(Vec3f *pts, Vec3f P)
{
    Vec3f u = cross(Vec3f(pts[2][0] - pts[0][0], pts[1][0] - pts[0][0], pts[0][0] - P[0]), Vec3f(pts[2][1] - pts[0][1], pts[1][1] - pts[0][1], pts[0][1] - P[1]));
    // Vec3f v1 = Vec3f(pts[2][0] - pts[0][0], pts[1][0] - pts[0][0], pts[0][0] - P[0]);
    // Vec3f v2 = Vec3f(pts[2][1] - pts[0][1], pts[1][1] - pts[0][1], pts[0][1] - P[1]);
    // Vec3f u = Vec3f(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
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

Vec3f m2v(Matrix m) {
    return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}

Matrix v2m(Vec3f v) {
    Matrix m(4, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}

Matrix viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
    m[0][3] = x + w / 2.f;
    m[1][3] = y + h / 2.f;
    m[2][3] = DEPTH / 2.f;

    m[0][0] = w / 2.f;
    m[1][1] = h / 2.f;
    m[2][2] = DEPTH / 2.f;
    return m;
}

Vec3f world2screen(Vec3f v) {
    // + .5 是为了能四舍五入，不加会统一向下取整
    return Vec3f(int((v.x + 1.) * OUTPUT_WIDTH / 2. + .5), int((v.y + 1.) * OUTPUT_HEIGHT / 2. + .5), int(v.z * DEPTH + .5));
}

void triangle(Vec3f *pts, float *zbuffer, Vec3f *uv_coords, Vec3f *vn_coords, Vec3f light_dir, TGAImage &image, TGAImage &texture)
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
            if (zbuffer[int(P.x + P.y * OUTPUT_WIDTH)] < P.z) {
                zbuffer[int(P.x + P.y * OUTPUT_WIDTH)] = P.z;
                Vec3f uv;
                Vec3f vn;
                for (int i = 0; i < 3; i++) {
                    uv = uv + uv_coords[i] * bc_screen[i];
                    vn = vn + vn_coords[i] * bc_screen[i];
                }
                TGAColor color = texture.get(uv[0] * texture.get_width(), uv[1] * texture.get_height());
                // float intensity = vn[0] * light_dir[0] + vn[1] * light_dir[1] + vn[2] * light_dir[2];
                float intensity = vn * light_dir;
                color.r = color.r * intensity;
                color.g = color.g * intensity;
                color.b = color.b * intensity;
                image.set(P.x, P.y, color);
            }
        }
    }
}

int main(int argc, char **argv) {
    if (2 == argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head/african_head.obj");
    }

    float *zbuffer = new float[OUTPUT_WIDTH * OUTPUT_HEIGHT];
    for (int i = OUTPUT_WIDTH * OUTPUT_HEIGHT; i--; zbuffer[i] = -std::numeric_limits<float>::max());
    TGAImage image(OUTPUT_WIDTH, OUTPUT_HEIGHT, TGAImage::RGB);
    TGAImage texture(TEXTURE_WIDTH, TEXTURE_HEIGHT, TGAImage::RGB);
    texture.read_tga_file("obj/african_head/african_head_diffuse.tga");
    texture.flip_vertically();
    Vec3f light_dir(0, 0, 1);
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f pts[3], world_coords[3];
        Vec3f uv_coords[3];
        Vec3f vn_coords[3];
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j * 3]);
            world_coords[j] = v;
            pts[j] = world2screen(v);
            Vec3f origin_uv = model->uv_vert(face[j * 3 + 1]);
            uv_coords[j] = Vec3f(origin_uv[0], origin_uv[1], origin_uv[2]);
            vn_coords[j] = model->vn_vert(face[j * 3 + 2]);
        }
        // Vec3f n = cross((world_coords[2] - world_coords[0]), (world_coords[1] - world_coords[0]));
        // TGAColor random = TGAColor(rand() % 255 * intensity, rand() % 255 * intensity, rand() % 255 * intensity, 255);
        // TGAColor white = TGAColor(255 * intensity, 255 * intensity, 255 * intensity, 255);
        triangle(pts, zbuffer, uv_coords, vn_coords, light_dir, image, texture);
    }

    image.flip_vertically();
    image.write_tga_file("output.tga");
    delete model;

    return 0;
}