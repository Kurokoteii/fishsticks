#pragma once

#include <stdint.h>

#include "parse.h"
#include "window.h"
#include "mem.h"
#include "input.h"
#include "config.h"

extern bool beatmap_loaded;
extern bool start_parse_replay;
extern BeatmapData current_beatmap;
extern ReplayData current_replay;

void process_hitobject();
bool is_replay_mode(uintptr_t osu_manager_ptr);

template <typename T>
Vector2<T> prepare_hitcircle_target(uintptr_t osu_manager_ptr, const Vector2<float> &position, Vector2<T> &mouse_position)
{
    uintptr_t osu_manager = *(uintptr_t *)(osu_manager_ptr);
    uintptr_t osu_ruleset_ptr = *(uintptr_t *)(osu_manager + 0x68);
    float mouse_x = *(float *)(osu_ruleset_ptr + 0x80);
    float mouse_y = *(float *)(osu_ruleset_ptr + 0x84);
    mouse_position.x = mouse_x;
    mouse_position.y = mouse_y;
    Vector2 circle_position_on_screen = playfield_to_screen(position);
    Vector2 direction = circle_position_on_screen - mouse_position;
    return direction;
}
