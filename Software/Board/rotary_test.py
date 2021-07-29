from machine import Pin

angle = 0


def print_angle():
    global angle
    angle = (angle - 360) if (angle > 360) else angle
    print("{} deg".format(angle))


def clock(p):
    global angle
    angle += 10
    print_angle()


def anti_clock(p):
    global angle
    angle -= 10
    print_angle()


a = Pin(4, Pin.IN, Pin.PULL_UP)
b = Pin(2, Pin.IN, Pin.PULL_UP)

a.irq(trigger=Pin.IRQ_FALLING, handler=clock)
b.irq(trigger=Pin.IRQ_FALLING, handler=anti_clock)
