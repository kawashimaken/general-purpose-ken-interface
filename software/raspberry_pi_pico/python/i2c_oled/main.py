import machine
import time
import math

# -------------------------
# I2C setup (SoftI2C)
# -------------------------
i2c = machine.SoftI2C(sda=machine.Pin(14), scl=machine.Pin(15), freq=400000)
ADDR = 0x3C
W, H = 128, 32

# -------------------------
# Low-level I2C functions
# -------------------------
def cmd(c):
    i2c.writeto(ADDR, bytes([0x80, c]))

_FLUSH_HDR = bytes([0x80,0x21, 0x80,0x00, 0x80,0x7F,
                    0x80,0x22, 0x80,0x00, 0x80,0x03])

def flush(buf):
    i2c.writeto(ADDR, _FLUSH_HDR)
    i2c.writeto(ADDR, b'\x40' + buf)

# -------------------------
# Display init
# -------------------------
for c in [0xAE, 0x20, 0x00, 0x40, 0xA1, 0xA8, 0x1F,
          0xC8, 0xD3, 0x00, 0xDA, 0x02, 0xD5, 0x80,
          0xD9, 0xF1, 0xDB, 0x30, 0x81, 0xFF,
          0xA4, 0xA6, 0x8D, 0x14, 0xAF]:
    cmd(c)

# -------------------------
# Framebuffer
# -------------------------
buf = bytearray(W * 4)  # 128 cols x 4 pages

def clear():
    buf[:] = bytes(W * 4)

def set_pixel(x, y, on=1):
    if 0 <= x < W and 0 <= y < H:
        idx = (y // 8) * W + x
        if on:
            buf[idx] |= (1 << (y % 8))
        else:
            buf[idx] &= ~(1 << (y % 8))

def draw_rect(x, y, w, h):
    for i in range(w):
        set_pixel(x + i, y)
        set_pixel(x + i, y + h - 1)
    for i in range(h):
        set_pixel(x, y + i)
        set_pixel(x + w - 1, y + i)

def fill_rect(x, y, w, h):
    # Compute page masks and write directly to buf
    for j in range(y, y + h):
        if 0 <= j < H:
            page = j // 8
            bit  = 1 << (j % 8)
            base = page * W
            for i in range(x, x + w):
                if 0 <= i < W:
                    buf[base + i] |= bit

# -------------------------
# 5x8 Font (A-Z, 0-9, symbols)
# -------------------------
FONT = {
    ' ': [0x00,0x00,0x00,0x00,0x00],
    '!': [0x00,0x00,0x5F,0x00,0x00],
    ',': [0x00,0x05,0x06,0x00,0x00],
    '.': [0x00,0x03,0x03,0x00,0x00],
    ':': [0x00,0x36,0x36,0x00,0x00],
    '-': [0x08,0x08,0x08,0x08,0x08],
    '0': [0x3E,0x51,0x49,0x45,0x3E],
    '1': [0x00,0x42,0x7F,0x40,0x00],
    '2': [0x42,0x61,0x51,0x49,0x46],
    '3': [0x21,0x41,0x45,0x4B,0x31],
    '4': [0x18,0x14,0x12,0x7F,0x10],
    '5': [0x27,0x45,0x45,0x45,0x39],
    '6': [0x3C,0x4A,0x49,0x49,0x30],
    '7': [0x01,0x71,0x09,0x05,0x03],
    '8': [0x36,0x49,0x49,0x49,0x36],
    '9': [0x06,0x49,0x49,0x29,0x1E],
    'A': [0x7E,0x11,0x11,0x11,0x7E],
    'B': [0x7F,0x49,0x49,0x49,0x36],
    'C': [0x3E,0x41,0x41,0x41,0x22],
    'D': [0x7F,0x41,0x41,0x22,0x1C],
    'E': [0x7F,0x49,0x49,0x49,0x41],
    'F': [0x7F,0x09,0x09,0x09,0x01],
    'G': [0x3E,0x41,0x49,0x49,0x7A],
    'H': [0x7F,0x08,0x08,0x08,0x7F],
    'I': [0x00,0x41,0x7F,0x41,0x00],
    'J': [0x20,0x40,0x41,0x3F,0x01],
    'K': [0x7F,0x08,0x14,0x22,0x41],
    'L': [0x7F,0x40,0x40,0x40,0x40],
    'M': [0x7F,0x02,0x0C,0x02,0x7F],
    'N': [0x7F,0x04,0x08,0x10,0x7F],
    'O': [0x3E,0x41,0x41,0x41,0x3E],
    'P': [0x7F,0x09,0x09,0x09,0x06],
    'Q': [0x3E,0x41,0x51,0x21,0x5E],
    'R': [0x7F,0x09,0x19,0x29,0x46],
    'S': [0x46,0x49,0x49,0x49,0x31],
    'T': [0x01,0x01,0x7F,0x01,0x01],
    'U': [0x3F,0x40,0x40,0x40,0x3F],
    'V': [0x1F,0x20,0x40,0x20,0x1F],
    'W': [0x3F,0x40,0x38,0x40,0x3F],
    'X': [0x63,0x14,0x08,0x14,0x63],
    'Y': [0x07,0x08,0x70,0x08,0x07],
    'Z': [0x61,0x51,0x49,0x45,0x43],
}

def draw_char(x, y, ch):
    glyph = FONT.get(ch.upper(), FONT[' '])
    page = y // 8
    shift = y % 8
    for col, byte in enumerate(glyph):
        cx = x + col
        if 0 <= cx < W:
            if shift == 0:
                buf[page * W + cx] |= byte
            else:
                buf[page * W + cx]       |= (byte << shift) & 0xFF
                if page + 1 < 4:
                    buf[(page+1) * W + cx] |= byte >> (8 - shift)

def draw_text(x, y, text):
    cx = x
    for ch in text:
        draw_char(cx, y, ch)
        cx += 6

def center_text(text, y):
    x = (W - len(text) * 6) // 2
    draw_text(max(x, 0), y, text)

# Precompute sine LUT (256 steps, one full cycle)
_SIN_LUT = [int((H/2) + (H/2 - 2) * math.sin(i * 2 * math.pi / 256)) for i in range(256)]

# -------------------------
# Demo 1: Splash screen
# -------------------------
def demo_splash():
    clear()
    center_text("RASPBERRY PI", 2)
    center_text("PICO  DEMO", 18)
    flush(buf)
    time.sleep(2)

# -------------------------
# Demo 2: Scrolling text
# -------------------------
def demo_scroll():
    message = "  HELLO  MICROPYTHON  "
    msg_w = len(message) * 6
    for offset in range(msg_w):
        clear()
        draw_text(-offset, 12, message)
        draw_text(-offset + msg_w, 12, message)
        flush(buf)

# -------------------------
# Demo 3: Counter
# -------------------------
def demo_counter():
    for i in range(20):
        clear()
        center_text("COUNTER", 2)
        center_text("{:04d}".format(i), 18)
        flush(buf)
        time.sleep(0.12)

# -------------------------
# Demo 4: Progress bar
# -------------------------
def demo_progress():
    for i in range(W + 1):
        clear()
        center_text("LOADING", 2)
        draw_rect(0, 20, W, 11)
        fill_rect(2, 22, i * (W - 4) // W, 7)
        flush(buf)
        time.sleep(0.005)
    time.sleep(0.5)

# -------------------------
# Demo 5: Sine wave
# -------------------------
def demo_sine():
    for frame in range(100):
        clear()
        for x in range(W):
            y = _SIN_LUT[(x * 2 + frame * 8) & 0xFF]
            set_pixel(x, max(0, min(H - 1, y)))
        flush(buf)
        time.sleep(0.03)

# -------------------------
# Demo 6: Bouncing ball
# -------------------------
def demo_bounce():
    bx, by = W // 2, H // 2
    vx, vy = 3, 2
    r = 4
    for _ in range(100):
        clear()
        x, y, p = r, 0, 1 - r
        while x >= y:
            for dx, dy in [(x,y),(-x,y),(x,-y),(-x,-y),(y,x),(-y,x),(y,-x),(-y,-x)]:
                set_pixel(bx + dx, by + dy)
            y += 1
            p = p + 2*y+1 if p <= 0 else p + 2*(y-x)+1
            if p > 0: x -= 1
        bx += vx; by += vy
        if bx - r <= 0 or bx + r >= W - 1: vx = -vx
        if by - r <= 0 or by + r >= H - 1: vy = -vy
        flush(buf)
        time.sleep(0.03)

# -------------------------
# Main loop
# -------------------------
demos = [
    ("Splash",   demo_splash),
    ("Scroll",   demo_scroll),
    ("Counter",  demo_counter),
    ("Progress", demo_progress),
    ("Sine",     demo_sine),
    ("Bounce",   demo_bounce),
]

print("OLED demo started")
while True:
    for name, demo in demos:
        print("Running:", name)
        demo()
        time.sleep(0.5)