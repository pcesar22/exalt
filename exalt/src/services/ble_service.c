/******************************************************************************
 * Includes
 *****************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>

#include <exalt/ble_service.h>
#include <exalt/blink_service.h>
#include <exalt/common.h>

/******************************************************************************
 * Definitions
 *****************************************************************************/
#define LED_CHAR_UUID BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x87654321, 0x1234, 0x5678, 0x1234, 0x56789abcdef1))

LOG_MODULE_REGISTER(ble_service, CONFIG_EXALT_LOG_LEVEL);

/******************************************************************************
 * Static Variables
 *****************************************************************************/
static struct bt_conn *current_conn;

static const struct bt_uuid_128 led_service_uuid =
    BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
                 0xf1, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
                 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
};

/******************************************************************************
 * Function Prototypes
 *****************************************************************************/
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static ssize_t write_led_ctrl(struct bt_conn *conn, 
                             const struct bt_gatt_attr *attr,
                             const void *buf, uint16_t len, 
                             uint16_t offset, uint8_t flags);
static void bt_ready(int err);

/******************************************************************************
 * Bluetooth Callbacks
 *****************************************************************************/
static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

/******************************************************************************
 * GATT Service Definition
 *****************************************************************************/
BT_GATT_SERVICE_DEFINE(
    led_svc,
    BT_GATT_PRIMARY_SERVICE(&led_service_uuid),
    BT_GATT_CHARACTERISTIC(LED_CHAR_UUID,
                          BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                          BT_GATT_PERM_WRITE,
                          NULL, write_led_ctrl, NULL)
);

/******************************************************************************
 * Function Implementations
 *****************************************************************************/
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection failed (err %u)", err);
        return;
    }

    LOG_INF("Connected");
    current_conn = bt_conn_ref(conn);
    
    // Send a message to blink service to change pattern when connected
    struct service_msg msg = {
        .type = MSG_LED_PATTERN_FAST_BLINK,
        .data.led_config.blink_rate_ms = 200
    };
    
    blink_service_send_msg(&msg);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected (reason %u)", reason);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
    
    // Send a message to blink service to change pattern when disconnected
    struct service_msg msg = {
        .type = MSG_LED_PATTERN_SLOW_BLINK,
        .data.led_config.blink_rate_ms = 1000
    };
    
    blink_service_send_msg(&msg);
}

static ssize_t write_led_ctrl(struct bt_conn *conn, 
                             const struct bt_gatt_attr *attr,
                             const void *buf, uint16_t len, 
                             uint16_t offset, uint8_t flags)
{
    LOG_INF("Received LED control command");
    
    if (offset != 0 || len < 1) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    
    const uint8_t *pattern = buf;
    struct service_msg msg;
    
    switch (pattern[0]) {
    case 0:
        msg.type = MSG_LED_OFF;
        break;
    case 1:
        msg.type = MSG_LED_PATTERN_SLOW_BLINK;
        msg.data.led_config.blink_rate_ms = 1000;
        if (len >= 3) {
            // Extract blink rate from bytes 1-2 if provided
            msg.data.led_config.blink_rate_ms = (pattern[1] << 8) | pattern[2];
        }
        break;
    case 2:
        msg.type = MSG_LED_PATTERN_FAST_BLINK;
        msg.data.led_config.blink_rate_ms = 200;
        if (len >= 3) {
            // Extract blink rate from bytes 1-2 if provided
            msg.data.led_config.blink_rate_ms = (pattern[1] << 8) | pattern[2];
        }
        break;
    case 3:
        msg.type = MSG_LED_PATTERN_SOLID;
        break;
    case 4:
        msg.type = MSG_LED_PATTERN_SOS;
        break;
    default:
        LOG_WRN("Unknown LED pattern: %d", pattern[0]);
        return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }
    
    blink_service_send_msg(&msg);
    return len;
}

static void bt_ready(int err)
{
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }
    LOG_INF("Bluetooth initialized!");

    err = bt_conn_cb_register(&conn_callbacks);
    if (err) {
        LOG_ERR("Bluetooth callback register failed (err %d)", err);
        return;
    }

    struct bt_le_adv_param adv_param = {
        .options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
    };

    err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }

    LOG_INF("Advertising successfully started!");
}

/******************************************************************************
 * Public Functions
 *****************************************************************************/
int ble_service_init(void)
{
    int err = bt_enable(bt_ready);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }
    
    return 0;
}