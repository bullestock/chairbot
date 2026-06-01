#pragma once

#include <string>
#include <vector>

bool i2s_init();

int get_sd_track_count(bool is_effects);

const std::vector<std::string>& sd_get_tracks(bool is_effects);
    
bool start_sd_playback(bool is_effects,
                       int track_index);

bool check_sd_track(bool is_effects,
                    int track_index,
                    std::string& error_msg);

void stop_sd_playback();

void set_sd_volume(int volume);

// Local Variables:
// compile-command: "(cd ..; idf.py build)"
// End:
