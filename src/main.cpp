#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

static K_SEM_DEFINE(app_hid_init, 0, 1);
static K_SEM_DEFINE(app_hid_ready, 0, 1); // main.cpp

#define LED0_NODE DT_ALIAS(led0)

/*
 * Helper macro for initializing a gpio_dt_spec from the devicetree
 * with fallback values when the nodes are missing.
 */
#define GPIO_SPEC(node_id) GPIO_DT_SPEC_GET_OR(node_id, gpios, {0})

/*
 * Create gpio_dt_spec structures from the devicetree.
 */
static const struct gpio_dt_spec led0 = GPIO_SPEC(LED0_NODE);

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

// Host --> Device (LEDs)
static void output_ready_cb(const device *kbd_dev)
{
    static uint8_t report;
    hid_int_ep_read(kbd_dev, &report, sizeof(report), NULL);

    gpio_pin_set(led0.port, led0.pin, report & HID_KBD_LED_CAPS_LOCK);

    // if (report & HID_KBD_LED_CAPS_LOCK)
    // {
    //     // led::on<led::Led1>();
    //     gpio_pin_toggle(led0.port, led0.pin);
    // }
    // else
    // {
    //     // led::off<led::Led1>();
    // }
}

int main(void)
{
    // k_sem_take(&app_hid_init, K_FOREVER);

    int err __unused = usb_enable(usb_status_cb);
    __ASSERT(!err, "usb_enable failed");

    if (!device_is_ready(led0.port))
    {
        LOG_ERR("LED device %s is not ready", led0.port->name);
        return 0;
    }

    const device *kbd_dev = device_get_binding("HID_0");
    __ASSERT_NO_MSG(kbd_dev);

    const uint8_t kbd_desc[] = HID_KEYBOARD_REPORT_DESC();
    const struct hid_ops callbacks = {
        .int_out_ready = output_ready_cb,
    };
    usb_hid_register_device(kbd_dev, kbd_desc, sizeof(kbd_desc),
                            &callbacks);

    err = usb_hid_init(kbd_dev);
    __ASSERT(!err, "usb_hid_init failed");

    gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
    gpio_pin_set(led0.port, led0.pin, 1U);

    // k_sem_give(&app_hid_init);

    while (true)
    {
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
