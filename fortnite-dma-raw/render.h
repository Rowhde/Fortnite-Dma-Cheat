#pragma once

#include <d3d9.h>
#include <d3dx9.h>

#include <string>
#include <tchar.h>
#include <thread>

#include <vector>

#include "color.h"

#include "vector.h"
#include "globals.h"

struct vertice_t
{
    float x, y, z, rhw;
    int c;
};

inline LPDIRECT3D9 d3d = NULL;
inline LPDIRECT3DDEVICE9 device = NULL;
inline D3DPRESENT_PARAMETERS pp = {};

extern void CacheLevels();
inline uint64_t base_address = 0;
extern void filled_box(int x, int y, int width, int height, c_color c);
extern void bordered_box(int x, int y, int width, int height, int thickness, c_color c);

inline LPD3DXFONT font;

extern void initialize_font();
extern void text(std::string text, int x, int y, c_color color, bool center);
extern vec2_t measure_text(std::string text);

extern void window_thread();

inline bool vec2_invalid(vec2_t vec)
{
    return vec.x < 1 || vec.y < 1 || vec.x > globals::resolution_x || vec.y > globals::resolution_y;
}