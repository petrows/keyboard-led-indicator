#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/usb/class/usb_cdc.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static K_SEM_DEFINE(app_hid_init, 0, 1);
static K_SEM_DEFINE(app_hid_ready, 0, 1); // main.cpp

/*
    Search device by-alias from DeviceTreee (see zephyr/board.overlay)
    https://docs.zephyrproject.org/latest/build/dts/howtos.html#get-a-struct-device-from-a-devicetree-node
 */
#define LED_BUILT_IN_NODE DT_ALIAS(led0)
#define LED_CAPS_NODE DT_ALIAS(led_caps)
#define LED_SCROLL_NODE DT_ALIAS(led_scrl)
#define LED_NUM_NODE DT_ALIAS(led_num)

/*
    Helper macro for initializing a gpio_dt_spec from the devicetree
    with fallback values when the nodes are missing.
 */
#define GPIO_SPEC(node_id) GPIO_DT_SPEC_GET_OR(node_id, gpios, {0})

/*
    Create gpio_dt_spec structures from the devicetree.
 */
static const struct gpio_dt_spec led_built_in = GPIO_SPEC(LED_BUILT_IN_NODE);
// Our custom leds from device-tree:
static const struct gpio_dt_spec led_caps = GPIO_SPEC(LED_CAPS_NODE);
static const struct gpio_dt_spec led_scroll = GPIO_SPEC(LED_SCROLL_NODE);
static const struct gpio_dt_spec led_num = GPIO_SPEC(LED_NUM_NODE);

/*
    Simple HID device config - to control more LEDs
*/
static const uint8_t hid_led_desc[] = {
    HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
    HID_USAGE(HID_USAGE_GEN_DESKTOP_UNDEFINED),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),
    HID_LOGICAL_MIN8(0x00),
    HID_LOGICAL_MAX16(0xFF, 0x00),
    HID_REPORT_ID(0x01),
    HID_REPORT_SIZE(8),
    HID_REPORT_COUNT(1),
    HID_USAGE(HID_USAGE_GEN_DESKTOP_UNDEFINED),
    HID_INPUT(0x02),
    HID_END_COLLECTION,
};

/*
    Main callback function to react on different USB events
    TODO: Add reset / reconnect on error?
*/
void usb_status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
    switch (status)
    {
    case USB_DC_ERROR:
        LOG_INF("USB error reported by the controller");
        break;
    case USB_DC_RESET:
        LOG_INF("USB reset");
        break;
    case USB_DC_CONNECTED:
        LOG_INF("USB connection established, hardware enumeration is completed");
        break;
    case USB_DC_CONFIGURED:
        k_sem_give(&app_hid_ready);
        LOG_INF("USB configuration done");
        break;
    case USB_DC_DISCONNECTED:
        LOG_INF("USB connection lost");
        break;
    case USB_DC_SUSPEND:
        LOG_INF("USB connection suspended by the HOST");
        break;
    case USB_DC_RESUME:
        LOG_INF("USB connection resumed by the HOST");
        break;
    case USB_DC_INTERFACE:
        LOG_INF("USB interface selected");
        break;
    case USB_DC_SET_HALT:
        LOG_INF("Set Feature ENDPOINT_HALT received");
        break;
    case USB_DC_CLEAR_HALT:
        LOG_INF("Clear Feature ENDPOINT_HALT received");
        break;
    case USB_DC_SOF:
        LOG_INF("Start of Frame received");
        break;
    case USB_DC_UNKNOWN:
        LOG_INF("Initial USB connection status");
        break;
    }
}

/*
    Host --> Keyboard
    Callback function to react on HID events from PC,
    test and (de)activate LEDs here.
*/
static void kb_output_ready_cb(const device *kbd_dev)
{
    static uint8_t report;
    hid_int_ep_read(kbd_dev, &report, sizeof(report), NULL);

    // Turn OFF built-in LED on first report
    gpio_pin_set_dt(&led_built_in, 0U);

    gpio_pin_set_dt(&led_caps, report & HID_KBD_LED_CAPS_LOCK);
    gpio_pin_set_dt(&led_scroll, report & HID_KBD_LED_SCROLL_LOCK);
    gpio_pin_set_dt(&led_num, report & HID_KBD_LED_NUM_LOCK);
}

/*
    Host --> Custom device 1
    Custom command from PC
*/
static void led_output_ready_cb(const device *dev)
{
    static uint8_t report;
    hid_int_ep_read(dev, &report, sizeof(report), NULL);

    // Turn OFF built-in LED on first report
    gpio_pin_toggle_dt(&led_built_in);
}

int main(void)
{
    // Set working mode for GPIO's
    gpio_pin_configure_dt(&led_built_in, GPIO_OUTPUT);
    gpio_pin_configure_dt(&led_caps, GPIO_OUTPUT);
    gpio_pin_configure_dt(&led_scroll, GPIO_OUTPUT);
    gpio_pin_configure_dt(&led_num, GPIO_OUTPUT);

    // Initial states
    gpio_pin_set_dt(&led_built_in, 1U);
    gpio_pin_set_dt(&led_caps, 1U);
    gpio_pin_set_dt(&led_scroll, 1U);
    gpio_pin_set_dt(&led_num, 1U);

    // Activate USB
    int err __unused = usb_enable(usb_status_cb);
    __ASSERT(!err, "usb_enable failed");

    // Activate and register HID device 0 (Keyboard)
    const device *kbd_dev = device_get_binding("HID_0");
    __ASSERT_NO_MSG(kbd_dev);
    // Add "Keyboard" report descriptor temaplte
    const uint8_t kbd_desc[] = HID_KEYBOARD_REPORT_DESC();
    // We do not send any keys to PC, so we have only
    // "out" function (event from PC to us).
    const struct hid_ops kb_callbacks = {
        .int_out_ready = kb_output_ready_cb,
    };
    usb_hid_register_device(kbd_dev, kbd_desc, sizeof(kbd_desc),
                            &kb_callbacks);

    // Activate and register HID device 1 (LED control)
    const device *led_dev = device_get_binding("HID_1");
    __ASSERT_NO_MSG(led_dev);
    // We do not send any data to PC, so we have only
    // "out" function (event from PC to us).
    const struct hid_ops led_callbacks = {
        .int_out_ready = led_output_ready_cb,
    };
    usb_hid_register_device(led_dev, hid_led_desc, sizeof(hid_led_desc),
                            &led_callbacks);

    // Initalize USB
    err = usb_hid_init(kbd_dev);
    __ASSERT(!err, "usb_hid_init failed");
    err = usb_hid_init(led_dev);
    __ASSERT(!err, "usb_hid_init failed");

    // Wait for USb activation
    k_sem_take(&app_hid_ready, K_FOREVER);

    // USB Ready, built-in: ON
    gpio_pin_set_dt(&led_built_in, 1U);

    while (true)
    {
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
