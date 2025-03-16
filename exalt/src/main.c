/******************************************************************************
 * Includes
 *****************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <exalt/blink_service.h>
#include <exalt/ble_service.h>
#include <exalt/common.h>

/******************************************************************************
 * Log Module Registration
 *****************************************************************************/
LOG_MODULE_REGISTER(EXALT, CONFIG_EXALT_LOG_LEVEL);

/******************************************************************************
 * Main Function
 *****************************************************************************/
int main(void)
{
    LOG_INF("Exalt Application Starting...");

    // Initialize blink service
    int err = blink_service_init();
    if (err) {
        LOG_ERR("Failed to initialize blink service (err %d)", err);
        return err;
    }

    // Initialize BLE service
    err = ble_service_init();
    if (err) {
        LOG_ERR("Failed to initialize BLE service (err %d)", err);
        return err;
    }

    LOG_INF("All services initialized successfully");

    // Main application loop
    while (1) {
        // The main thread can perform other tasks or monitor overall system status
        k_sleep(K_SECONDS(10));
        LOG_INF("Main thread still running...");
    }

    return 0;
}