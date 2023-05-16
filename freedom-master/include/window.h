#pragma once

#include "vector.h"
#include "freedom.h"

extern Vector2<float> window_size;
extern Vector2<float> playfield_size;
extern Vector2<float> playfield_position;

extern float window_ratio;
extern float playfield_ratio;

template <typename T>
Vector2<T> screen_to_playfield(Vector2<T> screen_coords)
{
    return (screen_coords - playfield_position) / playfield_ratio;
}

template <typename T>
Vector2<T> playfield_to_screen(Vector2<T> playfield_coords)
{
    return (playfield_coords * playfield_ratio) + playfield_position;
}

void calc_playfield_manual(float window_x, float window_y);
bool calc_playfield_from_window(uintptr_t window_manager_ptr);
