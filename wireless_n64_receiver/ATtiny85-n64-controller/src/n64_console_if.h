/*
* Robin Sayers December 2020
*/

#ifndef GUARD_N64_CONTROLLER_SIM
#define GUARD_N64_CONTROLLER_SIM

//#define EXPECTED_STATUS_RESPONSE 0x50000
//#define EXPECTED_STARTUP_CONTROLLER_VALUE 0x00000000
#define N64_NO_MESSAGE 0
#define N64_STATUS 1
#define N64_POLL 3

#ifdef __cplusplus
extern "C" {
#endif

// Prototypes for the two controller commands supported:
//unsigned long getControllerStatus(void);
//unsigned long pollController(void);
//void handle_command_from_console(volatile byte *N64_CONTROLLER_STATE);
volatile byte get_incoming_command_from_console(void);
//void respond_to_poll_command(volatile byte N64_CONTROLLER_STATE);
void respond_to_status_command(volatile byte N64_CONTROLLER_STATUS);
//volatile byte consoleRequestMessage = N64_NO_MESSAGE;


#ifdef __cplusplus
}
#endif

#endif

