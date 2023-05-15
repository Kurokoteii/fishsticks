#include "hitobject.h"

// ...

void process_hitobject()
{
    static double keydown_time = 0.0;
    static double keyup_delay = 0.0;
    static float fraction_of_the_distance = 0.0f;
    static Vector2 direction(0.0f, 0.0f);
    static Vector2 mouse_position(0.0f, 0.0f);
    static Vector2 last_mouse_position(0.0f, 0.0f); // Added: Track the last mouse position

    // ...

    if ((cfg_relax_lock || cfg_aimbot_lock) && scene_is_game(current_scene_ptr) && current_beatmap.ready && is_playing(audio_time_ptr) && !is_replay_mode(osu_manager_ptr))
    {
        int32_t audio_time = *(int32_t *)audio_time_ptr;
        Circle* circle = current_beatmap.current_circle();
        if (cfg_aimbot_lock)
        {
            if (fraction_of_the_distance)
            {
                if (fraction_of_the_distance > 1.0f)
                {
                    fraction_of_the_distance = 0.0f;
                }
                else
                {
                    Vector2 next_mouse_position = mouse_position + direction * fraction_of_the_distance;
                    Vector2 movement_vector = next_mouse_position - last_mouse_position; // Added: Calculate the movement vector
                    move_mouse_relative(movement_vector.x, movement_vector.y); // Modified: Move the mouse relatively
                    fraction_of_the_distance += cfg_fraction_modifier;
                    last_mouse_position = next_mouse_position; // Added: Update the last mouse position
                }
            }
            if (target_first_circle)
            {
                direction = prepare_hitcircle_target(osu_manager_ptr, circle->position, mouse_position);
                fraction_of_the_distance = cfg_fraction_modifier;
                target_first_circle = false;
            }
        }

        // ...

        if (audio_time >= circle->end_time)
        {
            current_beatmap.hit_object_idx++;
            if (current_beatmap.hit_object_idx >= current_beatmap.hit_objects.size())
            {
                current_beatmap.ready = false;
            }
            else if (cfg_aimbot_lock)
            {
                Circle* next_circle = current_beatmap.current_circle();
                switch (next_circle->type)
                {
                    case HitObjectType::Circle:
                    case HitObjectType::Slider:
                    case HitObjectType::Spinner:
                    {
                        direction = prepare_hitcircle_target(osu_manager_ptr, next_circle->position, mouse_position);
                        fraction_of_the_distance = cfg_fraction_modifier;
                    } break;
                }
            }
        }
    }

    if (cfg_relax_lock && keydown_time && ((ImGui::GetTime() - keydown_time) * 1000.0 > keyup_delay))
    {
        keydown_time = 0.0;
        send_keyboard_input(current_click, KEYEVENTF_KEYUP);
    }
}

// ...

int main()
{
    // ...

    while (running)
    {
        // ...

        process_hitobject();

        // ...

        // Other code

        // ...
    }

    // ...

    return 0;
}
