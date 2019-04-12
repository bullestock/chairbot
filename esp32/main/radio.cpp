#include "radio.h"
#include "assert.h"

#include "RF24.h"

using namespace std;

// Radio pipe addresses for the 2 nodes to communicate.
static const uint8_t pipes[][6] = { "1BULL", "2BULL" };

bool radio_init(RF24& radio)
{
    // Setup and configure rf radio
    assert(radio.begin());
    radio.setChannel(108);

    radio.setRetries(15, 15);

    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1, pipes[0]);

	radio.startListening();

    return true;
}
