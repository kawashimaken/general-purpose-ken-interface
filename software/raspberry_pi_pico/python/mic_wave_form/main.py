import machine
import time
import struct

# -------------------------
# OLED: SoftI2C (SDA=GP14, SCL=GP15)
# -------------------------
i2c = machine.SoftI2C(sda=machine.Pin(14), scl=machine.Pin(15), freq=400000)
ADDR = 0x3C
W, H = 128, 32

def cmd(c):
    i2c.writeto(ADDR, bytes([0x80, c]))

# All 6 address-setup commands batched into one I2C transaction
_FLUSH_HDR = bytes([0x80,0x21, 0x80,0x00, 0x80,0x7F,
                    0x80,0x22, 0x80,0x00, 0x80,0x03])

def flush(buf):
    i2c.writeto(ADDR, _FLUSH_HDR)
    i2c.writeto(ADDR, b'\x40' + buf)

# Display init
for c in [0xAE, 0x20, 0x00, 0x40, 0xA1, 0xA8, 0x1F,
          0xC8, 0xD3, 0x00, 0xDA, 0x02, 0xD5, 0x80,
          0xD9, 0xF1, 0xDB, 0x30, 0x81, 0xFF,
          0xA4, 0xA6, 0x8D, 0x14, 0xAF]:
    cmd(c)

# Framebuffer: 4 pages x 128 cols
buf = bytearray(W * 4)

# Precomputed lookup tables for set_pixel speed
_PAGE = bytes([y >> 3  for y in range(H)])   # page index for each y
_BIT  = bytes([1 << (y & 7) for y in range(H)])  # bitmask for each y

# -------------------------
# I2S Microphone: INMP441
# SCK=GP4, WS=GP5, SD=GP6
# -------------------------
SAMPLE_RATE = 16000
NUM_SAMPLES = W          # one sample per display column

audio_in = bytearray(NUM_SAMPLES * 4)   # 32-bit per sample

i2s = machine.I2S(
    0,
    sck=machine.Pin(4),
    ws=machine.Pin(5),
    sd=machine.Pin(6),
    mode=machine.I2S.RX,
    bits=32,                             # INMP441: 24-bit data in 32-bit slot
    format=machine.I2S.MONO,
    rate=SAMPLE_RATE,
    ibuf=NUM_SAMPLES * 4 * 8            # internal DMA buffer (8 frames headroom)
)

# -------------------------
# AGC (Auto Gain Control) settings
# -------------------------
NOISE_GATE   = 800     # Ignore signals below this (cuts AC background hum)
AGC_MAX_AMP  = (H // 2 - 2)  # Max display amplitude in pixels (= 14px)
CENTER_Y     = H // 2        # = 16
agc_peak     = 1000    # Running peak estimate (starts small, adapts quickly)
AGC_ATTACK   = 0.3     # How fast gain reduces when signal is loud  (0~1)
AGC_RELEASE  = 0.02    # How fast gain recovers in silence          (0~1)

# -------------------------
# Draw a vertical line in column x from y0 to y1 (inclusive)
# Operates directly on buf pages — avoids per-pixel overhead
# -------------------------
def draw_vline(x, y0, y1):
    if y0 > y1:
        y0, y1 = y1, y0
    y0 = max(0, y0)
    y1 = min(H - 1, y1)
    p0, p1 = y0 >> 3, y1 >> 3
    if p0 == p1:
        # Both endpoints in the same page: single write
        mask = 0
        for b in range(y0 & 7, (y1 & 7) + 1):
            mask |= 1 << b
        buf[p0 * W + x] |= mask
    else:
        # Bottom of first page
        mask = 0
        for b in range(y0 & 7, 8):
            mask |= 1 << b
        buf[p0 * W + x] |= mask
        # Full middle pages
        for p in range(p0 + 1, p1):
            buf[p * W + x] = 0xFF
        # Top of last page
        mask = 0
        for b in range(0, (y1 & 7) + 1):
            mask |= 1 << b
        buf[p1 * W + x] |= mask

# -------------------------
# Main loop
# -------------------------
print("Waveform display started")
prev_y = CENTER_Y

while True:
    # --- Capture audio (blocking read, fills exactly NUM_SAMPLES) ---
    i2s.readinto(audio_in)

    # --- Clear framebuffer ---
    buf[:] = bytes(W * 4)

    # --- Render waveform ---
    # Draw center line (dashed every 4px for reference)
    for x in range(0, W, 4):
        buf[_PAGE[CENTER_Y] * W + x] |= _BIT[CENTER_Y]

    # --- AGC: find peak amplitude in this frame ---
    frame_peak = 0
    for x in range(W):
        idx = x << 2
        raw = (audio_in[idx+3] << 24 | audio_in[idx+2] << 16 |
               audio_in[idx+1] <<  8 | audio_in[idx])
        if raw >= 0x80000000:
            raw -= 0x100000000
        sample24 = raw >> 8
        amp = sample24 if sample24 >= 0 else -sample24
        if amp > frame_peak:
            frame_peak = amp

    # Noise gate: if frame is silent, show flat line
    if frame_peak < NOISE_GATE:
        buf[:] = bytes(W * 4)
        for x in range(0, W, 4):
            buf[_PAGE[CENTER_Y] * W + x] |= _BIT[CENTER_Y]
        flush(buf)
        prev_y = CENTER_Y
        continue

    # Update running peak with attack/release
    if frame_peak > agc_peak:
        agc_peak = int(agc_peak * (1 - AGC_ATTACK)  + frame_peak * AGC_ATTACK)
    else:
        agc_peak = int(agc_peak * (1 - AGC_RELEASE) + frame_peak * AGC_RELEASE)
    agc_peak = max(agc_peak, NOISE_GATE)

    # --- Render waveform with AGC scale ---
    for x in range(W):
        idx = x << 2
        raw = (audio_in[idx+3] << 24 | audio_in[idx+2] << 16 |
               audio_in[idx+1] <<  8 | audio_in[idx])
        if raw >= 0x80000000:
            raw -= 0x100000000
        sample24 = raw >> 8
        y = CENTER_Y - int(sample24 * AGC_MAX_AMP / agc_peak)
        y = max(0, min(H - 1, y))
        draw_vline(x, prev_y, y)
        prev_y = y

    # --- Push buffer to OLED in 2 I2C transactions ---
    flush(buf)