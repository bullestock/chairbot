#pragma once

#include <string>
#include <vector>

bool i2s_init();

int get_sd_track_count();

const std::vector<std::string>& sd_get_tracks();
    
void start_sd_playback(int track_index);

void stop_sd_playback();

