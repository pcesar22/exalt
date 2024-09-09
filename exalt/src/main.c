/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>
#include <bluetooth/services/nus.h>

/******************************************************************************
 * Definitions
 *****************************************************************************/
#define SLEEP_TIME_MS 1000
#define LED0_NODE DT_ALIAS(led0)

#define IMU_CHAR_UUID BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x87654321, 0x1234, 0x5678, 0x1234, 0x56789abcdef0))

/******************************************************************************
 * Log Module Registration
 *****************************************************************************/
LOG_MODULE_REGISTER(EXALT, CONFIG_EXALT_LOG_LEVEL);

/******************************************************************************
 * Static Variables
 *****************************************************************************/
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static struct k_timer led_timer;
static struct bt_conn *current_conn;

static const struct bt_uuid_128 imu_service_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0));

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
				  0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
				  0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
};

/******************************************************************************
 * Function Prototypes
 *****************************************************************************/
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static ssize_t read_imu_data(struct bt_conn *conn,
							 const struct bt_gatt_attr *attr,
							 void *buf, uint16_t len, uint16_t offset);
static void imu_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
static void bt_ready(int err);
static void bluetooth_testing(void);
static void led_timer_handler(struct k_timer *timer);

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
	imu_svc,
	BT_GATT_PRIMARY_SERVICE(&imu_service_uuid),
	BT_GATT_CHARACTERISTIC(IMU_CHAR_UUID,
						   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
						   BT_GATT_PERM_READ,
						   read_imu_data, NULL, NULL),
	BT_GATT_CCC(imu_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

/******************************************************************************
 * Function Implementations
 *****************************************************************************/
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}

	LOG_INF("Connected");
	current_conn = bt_conn_ref(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason %u)", reason);

	if (current_conn)
	{
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
}

static ssize_t read_imu_data(struct bt_conn *conn,
							 const struct bt_gatt_attr *attr,
							 void *buf, uint16_t len, uint16_t offset)
{
	LOG_INF("Generating fake IMU data");
	int16_t imu_data[10];

	// Generate fake IMU data
	for (int i = 0; i < 10; i++)
	{
		imu_data[i] = (int16_t)sys_rand32_get();
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, imu_data, sizeof(imu_data));
}

static void imu_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
	LOG_INF("Notifications %s", notif_enabled ? "enabled" : "disabled");
}

static void bt_ready(int err)
{
	if (err)
	{
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
	LOG_INF("Bluetooth initialized!");

	struct bt_le_adv_param adv_param = {
		.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
		.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
		.interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
	};

	err = bt_conn_cb_register(&conn_callbacks);
	if (err)
	{
		LOG_ERR("Bluetooth callback register failed (err %d)", err);
		return;
	}

	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err)
	{
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_INF("Advertising successfully started!");
}

static void bluetooth_testing()
{
	int err = bt_enable(bt_ready);
	if (err)
	{
		LOG_ERR("Bluetooth init failed (err %d)", err);
	}

	while (1)
	{
		k_sleep(K_FOREVER);
	}
}

static void led_timer_handler(struct k_timer *timer)
{
	gpio_pin_toggle_dt(&led);
}

/******************************************************************************
 * Main Function
 *****************************************************************************/
int main(void)
{
	LOG_INF("Hello Exalted!");

	int ret;
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0)
	{
		LOG_ERR("Error %d: failed to configure LED Pin", ret);
		return 0;
	}

	k_timer_init(&led_timer, led_timer_handler, NULL);
	k_timer_start(&led_timer, K_MSEC(1000), K_MSEC(1000));

	bluetooth_testing();
}