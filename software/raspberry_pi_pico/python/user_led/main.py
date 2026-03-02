from machine import Pin
import time

# Pin setup
led = Pin(28, Pin.OUT)       # GP28: LED
button = Pin(22, Pin.IN, Pin.PULL_UP)  # GP22: Button (internal pull-up)

print("Press the button to blink the LED")

while True:
    if button.value() == 0:  # Button pressed (LOW due to PULL_UP)
        led.toggle()
        time.sleep(0.2)      # Blink interval: 200ms
    else:
        led.off()            # Turn off LED when button is released