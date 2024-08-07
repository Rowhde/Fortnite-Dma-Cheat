#pragma once

#include <d3dx9.h>

class c_color
{
private:
public:
    int r, g, b, a;

    c_color(int ri = 0, int gi = 0, int bi = 0, int ai = 255) : r(ri), g(gi), b(bi), a(ai) { }

    int to_int()
    {
        return D3DCOLOR_ARGB(a, r, g, b);
    }
};

struct unity_color_t
{
    float r, g, b;
};