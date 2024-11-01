#pragma once

void init_nvs();

bool set_peer_mac(const char* peer_mac);

const char* get_peer_mac();

// Local Variables:
// compile-command: "cd .. && idf.py build"
// End:

