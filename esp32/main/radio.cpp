#include "radio.h"
#include "protocol.h"

#include <algorithm>

using namespace std;

// Radio pipe addresses for the 2 nodes to communicate.
static const uint8_t pipes[][6] = { "1BULL", "2BULL" };

bool init_radio(NRF24_t& dev)
{
    Nrf24_init(&dev);
    const uint8_t payload_size = std::max(sizeof(ForwardAirFrame), sizeof(ReturnAirFrame));
    const uint8_t channel = CONFIG_RADIO_CHANNEL;

    Nrf24_SetOutputRF_PWR(&dev, 3); // 3 = 0 dBm (max)

    Nrf24_config(&dev, channel, payload_size);

    // Set my own address using 5 characters
    if (Nrf24_setRADDR(&dev, pipes[1]) != ESP_OK)
    {
        printf("Nrf24_setRADDR() failed\n");
        return false;
    }

    // Set destination address using 5 characters
    if (Nrf24_setTADDR(&dev, pipes[0]) != ESP_OK)
    {
        printf("Nrf24_setTADDR() failed\n");
        return false;
    }
    return true;
}

// Local Variables:
// compile-command: "(cd ..; idf.py build)"
// End:
