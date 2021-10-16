/*
* Robin Sayers December 2020
*/

#ifndef GUARD_N64_CONTROLLER_SIM
#define GUARD_N64_CONTROLLER_SIM

#define N64_REQUEST_STATUS 1
#define N64_REQUEST_POLL 3

#ifdef __cplusplus
extern "C" {
#endif

// Prototypes for the controller commands:
volatile byte n64_read(void);
void n64_send(volatile byte n64_message, volatile byte length_in_bytes);

#ifdef __cplusplus
}
#endif

#endif

