#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "fuel_gauge.h"
#include "ble.h"

#define I2C_DEV_LABEL0 DT_ALIAS(i2c0)
#define I2C_DEV_LABEL1 DT_ALIAS(i2c1)
#define I2C_DEV_LABEL2 DT_ALIAS(i2c2)

const struct device *i2c_dev0, *i2c_dev1, *i2c_dev2;
uint8_t x;
uint8_t y = 0;

#define MAX30102_I2C_ADDR 0x57

// MAX30102 Register map
#define REG_INTR_STATUS_1 0x00
#define REG_INTR_ENABLE_1 0x02
#define REG_FIFO_WR_PTR 0x04
#define REG_OVF_COUNTER 0x05
#define REG_FIFO_RD_PTR 0x06
#define REG_FIFO_DATA 0x07
#define REG_FIFO_CONFIG 0x08
#define REG_MODE_CONFIG 0x09
#define REG_SPO2_CONFIG 0x0A
#define REG_LED1_PA 0x0C
#define REG_LED2_PA 0x0D

/* npm1300 */
#define NPM1300_ADDR 0x6B

#define REG_VSYS_MV 0x3A
#define REG_VBAT_MV 0x3C
#define REG_IBAT_CHG_MA 0x3E
#define REG_IBAT_DIS_MA 0x40
#define REG_FG_SOC 0x64

#define NPM1300_PAGE_CHARGER 0x03
#define BCHGENABLESET_REG 0x04
#define BCHG_ENABLE_BIT 0x01
uint16_t vbat, vsys, ichg, idis, soc;

/* Nadi pulse Data */
uint32_t red0, ir0;
uint32_t red1, ir1;
uint32_t red2, ir2;
int32_t sub_red_0 = 0, sub_ir_0 = 0;
int32_t sub_red_1 = 0, sub_ir_1 = 0;
int32_t sub_red_2 = 0, sub_ir_2 = 0;
int32_t prev_red_0 = 0, prev_ir_0 = 0;
int32_t prev_red_1 = 0, prev_ir_1 = 0;
int32_t prev_red_2 = 0, prev_ir_2 = 0;

// Write single byte
static int write_reg0(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_write(i2c_dev0, buf, sizeof(buf), MAX30102_I2C_ADDR);
}

// Read single byte
// static int read_reg0(uint8_t reg, uint8_t *value)
// {
//     return i2c_write_read(i2c_dev0, MAX30102_I2C_ADDR, &reg, 1, value, 1);
// }

// Read 6 bytes from FIFO (3 for RED, 3 for IR)
static int read_fifo0(uint32_t *red, uint32_t *ir)
{
    uint8_t reg = REG_FIFO_DATA;
    uint8_t data[6];

    i2c_write_read(i2c_dev0, MAX30102_I2C_ADDR, &reg, 1, data, 6);
    red0 = ((uint32_t)data[0] << 16) |
           ((uint32_t)data[1] << 8) |
           data[2];
    ir0 = ((uint32_t)data[3] << 16) |
          ((uint32_t)data[4] << 8) | data[5];

    sub_red_0 = (red0 - prev_red_0);
    sub_ir_0 = (ir0 - prev_ir_0);

    if (sub_red_0 > 1000)
    {
        sub_red_0 = 1000;
    }
    else if (sub_red_0 < -1000)
    {
        sub_red_0 = -1000;
    }
    if (sub_ir_0 > 1000)
    {
        sub_ir_0 = 1000;
    }
    else if (sub_ir_0 < -1000)
    {
        sub_ir_0 = -1000;
    }
    prev_red_0 = red0;
    prev_ir_0 = ir0;
    return 0;
}

// Write single byte
static int write_reg1(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_write(i2c_dev1, buf, sizeof(buf), MAX30102_I2C_ADDR);
}

// Read single byte
// static int read_reg1(uint8_t reg, uint8_t *value)
// {
//     return i2c_write_read(i2c_dev1, MAX30102_I2C_ADDR, &reg, 1, value, 1);
// }

// Read 6 bytes from FIFO (3 for RED, 3 for IR)
static int read_fifo1(uint32_t *red, uint32_t *ir)
{
    uint8_t reg = REG_FIFO_DATA;
    uint8_t data[6];

    i2c_write_read(i2c_dev1, MAX30102_I2C_ADDR, &reg, 1, data, 6);
    red1 = ((uint32_t)data[0] << 16) |
           ((uint32_t)data[1] << 8) |
           data[2];
    ir1 = ((uint32_t)data[3] << 16) |
          ((uint32_t)data[4] << 8) |
          data[5];

    sub_red_1 = (red1 - prev_red_1);
    sub_ir_1 = (ir1 - prev_ir_1);

    if (sub_red_1 > 1000)
    {
        sub_red_1 = 1000;
    }
    else if (sub_red_1 < -1000)
    {
        sub_red_1 = -1000;
    }
    if (sub_ir_1 > 1000)
    {
        sub_ir_1 = 1000;
    }
    else if (sub_ir_1 < -1000)
    {
        sub_ir_1 = -1000;
    }
    prev_red_1 = red1;
    prev_ir_1 = ir1;
    return 0;
}

// Write single byte
static int write_reg2(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_write(i2c_dev2, buf, sizeof(buf), MAX30102_I2C_ADDR);
}

// Read single byte
static int read_reg2(uint8_t reg, uint8_t *value)
{
    return i2c_write_read(i2c_dev2, MAX30102_I2C_ADDR, &reg, 1, value, 1);
}

// Read 6 bytes from FIFO (3 for RED, 3 for IR)
static int read_fifo2(uint32_t *red, uint32_t *ir)
{
    uint8_t reg = REG_FIFO_DATA;
    uint8_t data[6];

    i2c_write_read(i2c_dev2, MAX30102_I2C_ADDR, &reg, 1, data, 6);
    red2 = ((uint32_t)data[0] << 16) |
           ((uint32_t)data[1] << 8) |
           data[2];
    ir2 = ((uint32_t)data[3] << 16) |
          ((uint32_t)data[4] << 8) |
          data[5];

    sub_red_2 = (red2 - prev_red_2);
    sub_ir_2 = (ir2 - prev_ir_2);

    if (sub_red_2 > 1000)
    {
        sub_red_2 = 1000;
    }
    else if (sub_red_2 < -1000)
    {
        sub_red_2 = -1000;
    }
    if (sub_ir_2 > 1000)
    {
        sub_ir_2 = 1000;
    }
    else if (sub_ir_2 < -1000)
    {
        sub_ir_2 = -1000;
    }

    prev_red_2 = red2;
    prev_ir_2 = ir2;

    return 0;
}

static void max30102_init0(void)
{
    write_reg0(REG_MODE_CONFIG, 0x40);
    k_msleep(100);

    // FIFO reset
    write_reg0(REG_FIFO_WR_PTR, 0x00);
    write_reg0(REG_OVF_COUNTER, 0x00);
    write_reg0(REG_FIFO_RD_PTR, 0x00);

    // FIFO: sample avg = 4, FIFO_ROLLOVER_ENABLE = 1, FIFO_A_FULL = 0x0F
    // 0x4F = 0b0100_1111
    write_reg0(REG_FIFO_CONFIG, 0x4F);

    // SPO2 mode (RED + IR)
    write_reg0(REG_MODE_CONFIG, 0x03);

    // SPO2 config:
    // [7:5] = 0b100 = 400 Hz
    // [4:2] = 0b01  = 118 us pulse
    // [1:0] = 0b11  = 18-bit ADC
    write_reg0(REG_SPO2_CONFIG, 0x3b);

    // LED amplitudes (approx 7.6 mA)
    // write_reg0(REG_LED1_PA, 0x24);
    // write_reg0(REG_LED2_PA, 0x24);

    // Led amplitute should be 16 ma /* Perfect For finger*/
    // write_reg0(REG_LED2_PA, 0x40);
    // write_reg0(REG_LED1_PA, 0x40);

    write_reg0(REG_LED2_PA, 0x3f);
    write_reg0(REG_LED1_PA, 0x4f);

    /* Led amplitute should be 50 ma */
    // write_reg0(REG_LED2_PA, 0xFF);
    // write_reg0(REG_LED1_PA, 0xFF);
    // Multi-LED mode: Slot1 = RED, Slot2 = IR
    write_reg0(0x11, 0x21);
    write_reg0(0x12, 0x00);
    printk("MAX30102_0 initialized");
}

static void max30102_init1(void)
{
    write_reg1(REG_MODE_CONFIG, 0x40);
    k_msleep(100);

    // FIFO reset
    write_reg1(REG_FIFO_WR_PTR, 0x00);
    write_reg1(REG_OVF_COUNTER, 0x00);
    write_reg1(REG_FIFO_RD_PTR, 0x00);

    // FIFO: sample avg = 4, FIFO_ROLLOVER_ENABLE = 1, FIFO_A_FULL = 0x0F
    // 0x4F = 0b0100_1111
    write_reg1(REG_FIFO_CONFIG, 0x4F);

    // SPO2 mode (RED + IR)
    write_reg1(REG_MODE_CONFIG, 0x03);

    // SPO2 config:
    // [7:5] = 0b100 = 400 Hz
    // [4:2] = 0b01  = 118 us pulse
    // [1:0] = 0b11  = 18-bit ADC
    write_reg1(REG_SPO2_CONFIG, 0x3b);

    // LED amplitudes (approx 7.6 mA)
    // write_reg1(REG_LED1_PA, 0x24);
    // write_reg1(REG_LED2_PA, 0x24);

    // Led amplitute should be 16 ma /* Perfect For finger*/
    // write_reg1(REG_LED2_PA, 0x40);
    // write_reg1(REG_LED1_PA, 0x40);

    write_reg1(REG_LED2_PA, 0x3f);
    write_reg1(REG_LED1_PA, 0x4f);

    /* Led amplitute should be 50 ma */
    // write_reg1(REG_LED2_PA, 0xFF);
    // write_reg1(REG_LED1_PA, 0xFF);
    // Multi-LED mode: Slot1 = RED, Slot2 = IR
    write_reg1(0x11, 0x21);
    write_reg1(0x12, 0x00);
    printk("MAX30102_1 initialized");
}

static void max30102_init2(void)
{
    write_reg2(REG_MODE_CONFIG, 0x40);
    k_msleep(100);

    // FIFO reset
    write_reg2(REG_FIFO_WR_PTR, 0x00);
    write_reg2(REG_OVF_COUNTER, 0x00);
    write_reg2(REG_FIFO_RD_PTR, 0x00);

    // FIFO: sample avg = 4, FIFO_ROLLOVER_ENABLE = 1, FIFO_A_FULL = 0x0F
    // 0x4F = 0b0100_1111
    write_reg2(REG_FIFO_CONFIG, 0x4F);

    // SPO2 mode (RED + IR)
    write_reg2(REG_MODE_CONFIG, 0x03);

    // SPO2 config:
    // [7:5] = 0b100 = 400 Hz
    // [4:2] = 0b01  = 118 us pulse
    // [1:0] = 0b11  = 18-bit ADC
    write_reg2(REG_SPO2_CONFIG, 0x3b);

    // LED amplitudes (approx 7.6 mA)
    // write_reg2(REG_LED1_PA, 0x24);
    // write_reg2(REG_LED2_PA, 0x24);

    // Led amplitute should be 16 ma /* Perfect For finger*/
    // write_reg2(REG_LED2_PA, 0x40);
    // write_reg2(REG_LED1_PA, 0x40);

    write_reg2(REG_LED2_PA, 0x3f);
    write_reg2(REG_LED1_PA, 0x4f);

    /* Led amplitute should be 50 ma */
    // write_reg2(REG_LED2_PA, 0xFF);
    // write_reg2(REG_LED1_PA, 0xFF);
    // Multi-LED mode: Slot1 = RED, Slot2 = IR
    write_reg2(0x11, 0x21);
    write_reg2(0x12, 0x00);
    printk("MAX30102_2 initialized");
}

static void max30102_deinit0(void)
{
    // Put device in shutdown mode (bit[7] = 1)
    write_reg0(REG_MODE_CONFIG, 0x80);

    // Turn off LEDs (set pulse amplitude = 0)
    write_reg0(REG_LED1_PA, 0x00);
    write_reg0(REG_LED2_PA, 0x00);

    // Optionally clear FIFO and interrupts
    write_reg0(REG_FIFO_WR_PTR, 0x00);
    write_reg0(REG_OVF_COUNTER, 0x00);
    write_reg0(REG_FIFO_RD_PTR, 0x00);

    // LOG_INF("MAX30102 deinitialized (shutdown)");
}

static void max30102_deinit1(void)
{
    // Put device in shutdown mode (bit[7] = 1)
    write_reg1(REG_MODE_CONFIG, 0x80);

    // Turn off LEDs (set pulse amplitude = 0)
    write_reg1(REG_LED1_PA, 0x00);
    write_reg1(REG_LED2_PA, 0x00);

    // Optionally clear FIFO and interrupts
    write_reg1(REG_FIFO_WR_PTR, 0x00);
    write_reg1(REG_OVF_COUNTER, 0x00);
    write_reg1(REG_FIFO_RD_PTR, 0x00);

    // LOG_INF("MAX30102 deinitialized (shutdown)");
}

static void max30102_deinit2(void)
{
    // Put device in shutdown mode (bit[7] = 1)
    write_reg2(REG_MODE_CONFIG, 0x80);

    // Turn off LEDs (set pulse amplitude = 0)
    write_reg2(REG_LED1_PA, 0x00);
    write_reg2(REG_LED2_PA, 0x00);

    // Optionally clear FIFO and interrupts
    write_reg2(REG_FIFO_WR_PTR, 0x00);
    write_reg2(REG_OVF_COUNTER, 0x00);
    write_reg2(REG_FIFO_RD_PTR, 0x00);

    // LOG_INF("MAX30102 deinitialized (shutdown)");
}

static int npm1300_read_reg16(uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    int ret = i2c_write_read(i2c_dev0, NPM1300_ADDR, &reg, 1, data, 2);
    if (ret < 0)
        return ret;

    *value = data[0] | (data[1] << 8);
    return 0;
}

void npm1300_enable_charger()
{
    uint8_t tx_buf[3];

    // Step 1: Select Charger Page (0x03)
    tx_buf[0] = NPM1300_PAGE_CHARGER;
    tx_buf[1] = BCHGENABLESET_REG;
    tx_buf[2] = BCHG_ENABLE_BIT;

    if (i2c_write(i2c_dev0, tx_buf, sizeof(tx_buf), NPM1300_ADDR) < 0)
    {
        // LOG_ERR("Failed to enable charger");
        return;
    }
    tx_buf[0] = 0x50;
    tx_buf[1] = 0x01;
    if (i2c_write(i2c_dev0, tx_buf, 2, NPM1300_ADDR) < 0)
    {
        // LOG_ERR("Failed to set charger register");
        return;
    }

    // LOG_INF("Charger enabled successfully");
}
void combine_data()
{
    memset(data, 0, sizeof(data));
    snprintf(data, sizeof(data), "%d,%d,%d,%d,%d,%d,", sub_red_0, sub_ir_0, sub_red_1, sub_ir_1, sub_red_2, sub_ir_2);
    if (ble_connected && notify_enabled)
    {
        notify_data();
    }
}

int main(void)
{

    /* Try common DT label; change if your board uses a different label */
    i2c_dev0 = DEVICE_DT_GET(I2C_DEV_LABEL0);
    if (!i2c_dev0)
    {
        printk("I2C: device not found using alias,\n");
    }
    if (!i2c_dev0)
    {
        printk("I2C: device not found\n");
        // return;
    }

    i2c_dev1 = DEVICE_DT_GET(I2C_DEV_LABEL1);
    if (!i2c_dev1)
    {
        printk("I2C: device not found using alias,\n");
    }
    if (!i2c_dev1)
    {
        printk("I2C: device not found\n");
        // return;
    }
    i2c_dev2 = DEVICE_DT_GET(I2C_DEV_LABEL2);
    if (!i2c_dev2)
    {
        printk("I2C: device not found using alias,\n");
    }
    if (!i2c_dev2)
    {
        printk("I2C: device not found\n");
        // return;
    }

    // printk("Scanning I2C bus on %s...\n", i2c_dev0->name);

    // for (uint8_t addr = 0x03; addr <= 0x77; ++addr) {
    //     /* Try to read 1 byte. If device ACKs, return 0. */
    //     ret = i2c_read(i2c_dev0, &buf, 1, addr);
    //     if (ret == 0) {
    //             x=addr;
    //         printk("  Found device at 0x%02x\n", addr);
    //     }
    //     /* Optional: small delay to be gentle on bus */
    //     k_msleep(1);
    // }

    ble_init();
    start_advertising();

    rx_buf[0] = 'd';
    while (1)
    {
        // max30102_init1();
        // read_fifo1(&red1, &ir1);

        int ret = 0;
        if (sensor_status == 1)
        {
            if (rx_buf[0] == 'c') /* VAT */
            {
                max30102_init0();
                max30102_deinit1();
                max30102_deinit2();
                sensor_status = 0;
            }
            else if (rx_buf[0] == 'b') /* PIT */
            {
                max30102_deinit0();
                max30102_init1();
                max30102_deinit2();
                sensor_status = 0;
            }
            else if (rx_buf[0] == 'a') /* KAF */
            {
                max30102_deinit0();
                max30102_deinit1();
                max30102_init2();
                sensor_status = 0;
            }
            else if (rx_buf[0] == 'd')
            {
                max30102_init0();
                max30102_init1();
                max30102_init2();
                sensor_status = 0;
            }
            else if (rx_buf[0] == 'e')
            {
                max30102_deinit0();
                max30102_deinit1();
                max30102_deinit2();
                sensor_status = 0;
            }
            else
            {
                printk("Invalid Command\n");
            }

            sensor_status = 0;
        }
        if (rx_buf[0] == 'c' || rx_buf[0] == 'd')
            ret = read_fifo0(&red0, &ir0);

        if (rx_buf[0] == 'b' || rx_buf[0] == 'd')
            ret = read_fifo1(&red1, &ir1);

        if (rx_buf[0] == 'a' || rx_buf[0] == 'd')
            ret = read_fifo2(&red2, &ir2);

        combine_data();
        // npm1300_read_reg16(REG_VBAT_MV, &vbat);
        // npm1300_read_reg16(REG_VSYS_MV, &vsys);
        // npm1300_read_reg16(REG_IBAT_CHG_MA, &ichg);
        // npm1300_read_reg16(REG_IBAT_DIS_MA, &idis);
        // npm1300_read_reg16(REG_FG_SOC, &soc);
        k_msleep(10);
    }
    return 0;
}
