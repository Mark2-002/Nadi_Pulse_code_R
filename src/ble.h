#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/printk.h>


#define SDC_HCI_VS_LL_HANDLE_TYPE_ADV 0x00
#define BT_UUID_CUSTOM_SERVICE_VAL \
BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x9abc, 0xdef012345678)
#define BT_UUID_CUSTOM_CHAR_VAL \
BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x9abc, 0xdef012345679)

static struct bt_uuid_128 custom_service_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_SERVICE_VAL);
static struct bt_uuid_128 custom_char_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_CHAR_VAL);
static struct bt_conn *default_conn;
static int sensor_status = 0;
static bool ble_connected = false;
uint8_t r;
int8_t led_status = 0; 
uint8_t start_adv =0;
static uint8_t data[50]="";
static bool notify_enabled = false;

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	printk("Notifications %s\n", notify_enabled ? "enabled" : "disabled");
}
static ssize_t read_data(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr,
			    void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &data, sizeof(data));
}
static uint8_t rx_buf[20];
static ssize_t write_my_char(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             const void *buf, uint16_t len,
                             uint16_t offset, uint8_t flags)
{
    uint16_t copy_len = MIN(len, sizeof(rx_buf) - 1);
    memcpy(rx_buf, buf, copy_len);
    rx_buf[copy_len] = '\0';
    if (len == 0) {
        return 0;
    }
    sensor_status=1;
    return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
}
BT_GATT_SERVICE_DEFINE(custom_svc,
	BT_GATT_PRIMARY_SERVICE(&custom_service_uuid),
	BT_GATT_CHARACTERISTIC(&custom_char_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_data, write_my_char, &data),
	BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection failed (err 0x%02x)\n", err);
    } else {
        char addr[BT_ADDR_LE_STR_LEN];
        bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
        printk("Connected to: %s\n", addr);
		default_conn = bt_conn_ref(conn);
        ble_connected=true;
        led_status=2;
    }
}
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Disconnected from: %s (reason 0x%02x)\n", addr, reason);
    r=reason;
    ble_connected=false;
    led_status=0;
    start_adv=1;
     if (default_conn) {
        bt_conn_unref(default_conn);
        default_conn = NULL;
     }
}
int8_t ble_init(void) {
    int err;
    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return err;
    }
    printk("Bluetooth initialized\n");
    bt_conn_cb_register(&conn_callbacks);
    return 0;
}
int8_t ble_deinit(void) {
    int err;
    if (default_conn) {
        err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        if (err) {
            printk("Disconnection failed (err %d)\n", err);
            return err;
        }
        bt_conn_unref(default_conn);
        default_conn = NULL;
        printk("Disconnection initiated\n");
    }
    err = bt_disable();
    if (err) {
        printk("Bluetooth disable failed (err %d)\n", err);
        return err;
    }
    printk("Bluetooth disabled\n");
    led_status=0;
    return 0;
}   

int8_t start_advertising(void) {
    int err;
    struct bt_le_adv_param *adv_params =
        BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
                        BT_GAP_ADV_FAST_INT_MIN_2,
                        BT_GAP_ADV_FAST_INT_MAX_2,
                        NULL);

    const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
    };

    err = bt_le_adv_start(adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return err;
    }
    printk("Advertising successfully started\n");
    led_status=1;
    return 0;
}   
int8_t stop_advertising(void) {
    int err;

    err = bt_le_adv_stop();
    if (err) {
        printk("Failed to stop advertising (err %d)\n", err);
        return err;
    }
    printk("Advertising successfully stopped\n");
    led_status=0;
    return 0;
}
int8_t disconnect(void) {
    int err;
    if (default_conn) {
        err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        if (err) {
            printk("Disconnection failed (err %d)\n", err);
            return err;
        }
        bt_conn_unref(default_conn);
        default_conn = NULL;
        printk("Disconnection initiated\n");
    } else {
        printk("No active connection to disconnect\n");
    }
    led_status=0;
    return 0;
}
int8_t notify_data(void) {
    int err;
    if (!default_conn) {
        printk("No active connection. Cannot send notification.\n");
        return -ENOTCONN;
    }
    if (!notify_enabled) {
        printk("Notifications not enabled by the client.\n");
        return -EACCES;
    }

    err = bt_gatt_notify(default_conn, &custom_svc.attrs[1], &data, sizeof(data));
    if (err) {
        printk("Notification failed (err %d)\n", err);
        return err;
    }
    printk("Notification sent successfully\n");
    return 0;
}
