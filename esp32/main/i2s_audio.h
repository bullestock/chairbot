#pragma once

#include <string>
#include <vector>

bool i2s_init();

int get_sd_track_count();

const std::vector<std::string>& sd_get_tracks();
    
bool start_sd_playback(int track_index);

bool check_sd_track(int track_index,
                    std::string& error_msg);

void stop_sd_playback();

