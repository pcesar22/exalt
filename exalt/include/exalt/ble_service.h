/******************************************************************************
 * BLE Service
 *****************************************************************************/
#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <zephyr/kernel.h>

/**
 * @brief Initialize the BLE service
 * 
 * @return int 0 on success, negative error code otherwise
 */
int ble_service_init(void);

#endif /* BLE_SERVICE_H */