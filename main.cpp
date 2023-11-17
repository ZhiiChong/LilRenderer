#include <vector>
#include <cmath>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);

Model *model = NULL;

const int width = 200;
const int height = 200;

// Vec3f barycentric(Vec2i *pts, Vec2i P)
// {
//     Vec3f u = Vec3f(pts[2].x - pts[0].x, pts[1].x - pts[0].x, pts[0].x - P.x) ^ Vec3f(pts[2].y - pts[0].y, pts[1].y - pts[0].y, pts[0].y - P.y);
//     if (std::abs(u.z) < 1)
//         return Vec3f(-1, -1, -1);
//     return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
// }

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
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    for (int x = x0; x <= x1; x++)
    {
        float t = (x - x0) / (float)(x1 - x0);
        int y = y0 * (1. - t) + y1 * t;
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

void triangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage &image, TGAColor color)
{
    if (t0.y > t1.y)
        std::swap(t0, t1);
    if (t0.y > t2.y)
        std::swap(t0, t2);
    if (t1.y > t2.y)
        std::swap(t1, t2);

    int segA_h = t2.y - t0.y;
    int segB_h = t1.y - t0.y;
    int segC_h = t2.y - t1.y;
    for (int y = t0.y; y <= t2.y; y++)
    {

        Vec2i tempA = t0 + (t2 - t0) * ((float)(y - t0.y) / segA_h);
        Vec2i tempS = y > t1.y ? t1 + (t2 - t1) * ((float)(y - t1.y) / segC_h) : t0 + (t1 - t0) * ((float)(y - t0.y) / segB_h);
        if (tempA.x > tempS.x)
            std::swap(tempA, tempS);
        for (int x = tempA.x; x <= tempS.x; x++)
        {
            image.set(x, y, color);
        }
    }

    // line(t0, t1, image, white);
    // line(t1, t2, image, green);
    // line(t2, t0, image, red);
}

int main(int argc, char **argv)
{
    // if (2 == argc)
    // {
    //     model = new Model(argv[1]);
    // }
    // else
    // {
    //     model = new Model("obj/african_head.obj");
    // }

    TGAImage image(width, height, TGAImage::RGB);

    Vec2i t0[3] = {Vec2i(10, 70), Vec2i(50, 160), Vec2i(70, 80)};
    Vec2i t1[3] = {Vec2i(180, 50), Vec2i(150, 1), Vec2i(70, 180)};
    Vec2i t2[3] = {Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180)};
    triangle(t0[0], t0[1], t0[2], image, red);
    triangle(t1[0], t1[1], t1[2], image, white);
    triangle(t2[0], t2[1], t2[2], image, green);

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");

    return 0;
}