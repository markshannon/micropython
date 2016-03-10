from microbit import display, sleep
import audio
import math

def repeated_frame(frame, count):
    for i in range(count):
        yield frame

def show_wave(name, frame):
    display.scroll(name + " wave", wait=False,delay=100)
    for i in range(32):
        print(frame[i], end=", ")
    print()
    audio.play(repeated_frame(frame, 1500))

frame = audio.AudioFrame()

for i in range(32):
    frame[i] = int(math.sin(math.pi*i/16)*124+0.5)
show_wave("Sine", frame)
sleep(1000)

for i in range(8):
    frame[i] = i*15
    frame[i+8] = 120-i*15
    frame[i+16] = -i*15
    frame[i+24] = i*15-120
show_wave("Triangle", frame)
sleep(1000)

for i in range(16):
    frame[i] = -120
    frame[i+16] = 120
show_wave("Square", frame)
sleep(1000)

for i in range(32):
    frame[i] = 124-i*8
show_wave("Sawtooth", frame)
sleep(1000)
