
import audio
import radio

MIKE = Image("00900:90909:90909:09990:00900")
PASSIVE = Image.DIAMOND_SMALL*0.3
ACTIVE = Image.DIAMOND_*0.7

EMPTY_FRAME = audio.AudioFrame()
received_frame = EMPTY_FRAME

def configure(channel=80):
    radio.config(channel, queue=4, length=32)
    radio.on()

def receive(frame):
    active = False
    display.show(PASSIVE)
    while True:
        recvd = radio.receive_into(frame)
        if recvd == 32:
            #Ignore any messages not exactly 32 bytes long.
            received_frame = frame
            if not active:
                display.show(ACTIVE)
                active = True
        elif active:
            display.show(PASSIVE)
            active = False
        if button_a.is_pressed():
            return

def transmit(audio_in):
    display.show(MIKE)
    for frame in audio_in:
        radio.send_bytes(frame)
        if not button_a.is_pressed():
            return


def output_stream():
    global received_frame
    yield received_frame
    received_frame = EMPTY_FRAME

def main():
    audio_out = output_stream()
    audio_in = audio.record(pin0)
    frame = audio.AudioFrame()
    configure()
    audio.play(audio_out,pin=pin1,return_pin=pin2)
    while True:
        if button_a.is_pressed():
            transmit(audio_in)
        else:
            receive(frame)

main()
