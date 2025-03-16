#pragma once

void init_nvs();

bool set_peer_mac(const char* peer_mac);

const char* get_peer_mac();

bool set_my_mac(const char* my_mac);

const char* get_my_mac();

// Local Variables:
// compile-command: "cd .. && idf.py build"
// End:

