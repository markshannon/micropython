import synth
import audio
from microbit import sleep

class Bell:

    def __init__(self):
        self.osc1 = synth.Oscillator("sine")
        self.osc2 = synth.Oscillator("sine")
        mix = synth.Adder(self.osc1, 0.8, self.osc2, 0.2)
        self.out = synth.Envelope(mix, 100, 0.8, 0, 0.8)

    def set_frequency(self, f):
        self.osc1.set_frequency(f)
        self.osc2.set_frequency(int(1.6*f))

    def press(self):
        self.out.press()

    def release(self):
        self.out.release()

    def __iter__(self):
        return self.out

if __name__ == "__main__":
    play()