/******************************************************************************
 * Includes
 *****************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <exalt/blink_service.h>
#include <exalt/common.h>

/******************************************************************************
 * Definitions
 *****************************************************************************/
#define LED0_NODE DT_ALIAS(led0)
#define BLINK_THREAD_STACK_SIZE CONFIG_EXALT_BLINK_THREAD_STACK_SIZE
#define BLINK_THREAD_PRIORITY CONFIG_EXALT_BLINK_THREAD_PRIORITY
#define BLINK_QUEUE_SIZE 10

LOG_MODULE_REGISTER(blink_service, CONFIG_EXALT_LOG_LEVEL);

/******************************************************************************
 * Static Variables
 *****************************************************************************/
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static struct k_thread blink_thread_data;
static K_THREAD_STACK_DEFINE(blink_thread_stack, BLINK_THREAD_STACK_SIZE);
static struct k_msgq blink_msgq;
static struct service_msg blink_msg_buffer[BLINK_QUEUE_SIZE];
static int current_blink_rate_ms = 1000; // Default to 1 second
static enum message_type current_pattern = MSG_LED_PATTERN_SLOW_BLINK;

/******************************************************************************
 * Static Function Prototypes
 *****************************************************************************/
static void blink_thread_fn(void *arg1, void *arg2, void *arg3);
static void pattern_slow_blink(void);
static void pattern_fast_blink(void);
static void pattern_solid(void);
static void pattern_sos(void);
static void pattern_off(void);

/******************************************************************************
 * Public Functions
 *****************************************************************************/
int blink_service_init(void)
{
    int ret;

    // Initialize the message queue
    k_msgq_init(&blink_msgq, (char *)blink_msg_buffer, 
                sizeof(struct service_msg), BLINK_QUEUE_SIZE);

    // Configure LED
    if (!device_is_ready(led.port)) {
        LOG_ERR("LED device not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Error %d: failed to configure LED pin", ret);
        return ret;
    }

    // Start the blinking thread
    k_thread_create(&blink_thread_data, blink_thread_stack,
                    K_THREAD_STACK_SIZEOF(blink_thread_stack),
                    blink_thread_fn, NULL, NULL, NULL,
                    BLINK_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&blink_thread_data, "blink_thread");

    LOG_INF("Blink service initialized");
    return 0;
}

int blink_service_send_msg(struct service_msg *msg)
{
    if (k_msgq_put(&blink_msgq, msg, K_NO_WAIT) != 0) {
        LOG_WRN("Blink service message queue full");
        return -EAGAIN;
    }

    LOG_DBG("Message sent to blink service: type=%d", msg->type);
    return 0;
}

/******************************************************************************
 * Static Functions
 *****************************************************************************/
static void blink_thread_fn(void *arg1, void *arg2, void *arg3)
{
    struct service_msg msg;

    LOG_INF("Blink thread started");

    while (1) {
        // Check for new messages
        if (k_msgq_get(&blink_msgq, &msg, K_NO_WAIT) == 0) {
            // Process the message
            switch (msg.type) {
            case MSG_LED_PATTERN_SLOW_BLINK:
                current_pattern = MSG_LED_PATTERN_SLOW_BLINK;
                current_blink_rate_ms = msg.data.led_config.blink_rate_ms ? 
                                      msg.data.led_config.blink_rate_ms : 1000;
                LOG_INF("Changing to slow blink pattern (%d ms)", current_blink_rate_ms);
                break;
            case MSG_LED_PATTERN_FAST_BLINK:
                current_pattern = MSG_LED_PATTERN_FAST_BLINK;
                current_blink_rate_ms = msg.data.led_config.blink_rate_ms ? 
                                      msg.data.led_config.blink_rate_ms : 200;
                LOG_INF("Changing to fast blink pattern (%d ms)", current_blink_rate_ms);
                break;
            case MSG_LED_PATTERN_SOLID:
                current_pattern = MSG_LED_PATTERN_SOLID;
                LOG_INF("Changing to solid pattern");
                break;
            case MSG_LED_PATTERN_SOS:
                current_pattern = MSG_LED_PATTERN_SOS;
                LOG_INF("Changing to SOS pattern");
                break;
            case MSG_LED_OFF:
                current_pattern = MSG_LED_OFF;
                LOG_INF("Turning LED off");
                break;
            default:
                LOG_WRN("Unknown message type: %d", msg.type);
                break;
            }
        }

        // Execute the current pattern
        switch (current_pattern) {
        case MSG_LED_PATTERN_SLOW_BLINK:
            pattern_slow_blink();
            break;
        case MSG_LED_PATTERN_FAST_BLINK:
            pattern_fast_blink();
            break;
        case MSG_LED_PATTERN_SOLID:
            pattern_solid();
            break;
        case MSG_LED_PATTERN_SOS:
            pattern_sos();
            break;
        case MSG_LED_OFF:
            pattern_off();
            break;
        default:
            k_sleep(K_MSEC(100));
            break;
        }
    }
}

static void pattern_slow_blink(void)
{
    gpio_pin_toggle_dt(&led);
    k_sleep(K_MSEC(current_blink_rate_ms));
}

static void pattern_fast_blink(void)
{
    gpio_pin_toggle_dt(&led);
    k_sleep(K_MSEC(current_blink_rate_ms));
}

static void pattern_solid(void)
{
    gpio_pin_set_dt(&led, 1);
    k_sleep(K_MSEC(100)); // Sleep a little to avoid busy-waiting
}

static void pattern_off(void)
{
    gpio_pin_set_dt(&led, 0);
    k_sleep(K_MSEC(100)); // Sleep a little to avoid busy-waiting
}

static void pattern_sos(void)
{
    // S: ... (3 short blinks)
    for (int i = 0; i < 3; i++) {
        gpio_pin_set_dt(&led, 1);
        k_sleep(K_MSEC(200));
        gpio_pin_set_dt(&led, 0);
        k_sleep(K_MSEC(200));
    }
    
    k_sleep(K_MSEC(300)); // Pause between letters
    
    // O: --- (3 long blinks)
    for (int i = 0; i < 3; i++) {
        gpio_pin_set_dt(&led, 1);
        k_sleep(K_MSEC(600));
        gpio_pin_set_dt(&led, 0);
        k_sleep(K_MSEC(200));
    }
    
    k_sleep(K_MSEC(300)); // Pause between letters
    
    // S: ... (3 short blinks)
    for (int i = 0; i < 3; i++) {
        gpio_pin_set_dt(&led, 1);
        k_sleep(K_MSEC(200));
        gpio_pin_set_dt(&led, 0);
        k_sleep(K_MSEC(200));
    }
    
    k_sleep(K_MSEC(1000)); // Long pause before repeating
}