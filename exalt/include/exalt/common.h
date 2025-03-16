/******************************************************************************
 * Common Definitions
 *****************************************************************************/
#ifndef COMMON_H
#define COMMON_H

#include <zephyr/kernel.h>

/******************************************************************************
 * Message Types
 *****************************************************************************/
enum message_type {
    MSG_LED_PATTERN_SLOW_BLINK,
    MSG_LED_PATTERN_FAST_BLINK,
    MSG_LED_PATTERN_SOLID,
    MSG_LED_PATTERN_SOS,
    MSG_LED_OFF
};

/******************************************************************************
 * Message Structure
 *****************************************************************************/
struct service_msg {
    enum message_type type;
    union {
        struct {
            int blink_rate_ms;
        } led_config;
        
        // Add other message data types as needed
    } data;
};

#endif /* COMMON_H */