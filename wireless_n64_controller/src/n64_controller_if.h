/*
* Robin Sayers December 2020
*/

#ifndef GUARD_N64_CONSOLE_SIM
#define GUARD_N64_CONSOLE_SIM

#define EXPECTED_STATUS_RESPONSE 0x50000
#define EXPECTED_STARTUP_CONTROLLER_VALUE 0x00000000

#ifdef __cplusplus
extern "C" {
#endif

// Prototypes for the two controller commands supported:
unsigned long getControllerStatus(void);
unsigned long pollController(void);

#ifdef __cplusplus
}
#endif

#endif

