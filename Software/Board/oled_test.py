from machine import Pin, SoftI2C
import ssd1306
from time import sleep
from utime import time_ns

# ESP32 Pin assignment
i2c = SoftI2C(scl=Pin(21), sda=Pin(22))

oled_width = 128
oled_height = 64
oled = [
    ssd1306.SSD1306_I2C(oled_width, oled_height, i2c),
    ssd1306.SSD1306_I2C(oled_width, oled_height, i2c),
    ssd1306.SSD1306_I2C(oled_width, oled_height, i2c),
    ssd1306.SSD1306_I2C(oled_width, oled_height, i2c),
    ssd1306.SSD1306_I2C(oled_width, oled_height, i2c),
    ssd1306.SSD1306_I2C(oled_width, oled_height, i2c),
    ssd1306.SSD1306_I2C(oled_width, oled_height, i2c),
    ssd1306.SSD1306_I2C(oled_width, oled_height, i2c),
]

time_cur = time_ns()
time_old = time_cur

oled_index = 0
while True:
    try:
        i2c.writeto(0x70, b"\x00")
    except:
        pass
    if oled_index == 0:
        time_old = time_cur
        time_cur = time_ns()
    oled[oled_index].fill(0)
    oled[oled_index].text("{} us".format(str((time_cur - time_old) / 1000)), 0, 0)
    oled[oled_index].text(
        "{} FPS".format(str(1 / ((time_cur - time_old) / 1000000000))), 0, 10
    )
    oled[oled_index].show()
    oled_index = 0 if (oled_index >= 7) else oled_index + 1
