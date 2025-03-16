/******************************************************************************
 * Blink Service
 *****************************************************************************/
#ifndef BLINK_SERVICE_H
#define BLINK_SERVICE_H

#include <zephyr/kernel.h>
#include <exalt/common.h>

/**
 * @brief Initialize the blinking service
 * 
 * @param led_gpio_node DT node for the LED
 * @return int 0 on success, negative error code otherwise
 */
int blink_service_init(void);

/**
 * @brief Send a message to the blink service
 * 
 * @param msg Pointer to the message
 * @return int 0 on success, negative error code otherwise
 */
int blink_service_send_msg(struct service_msg *msg);

#endif /* BLINK_SERVICE_H */