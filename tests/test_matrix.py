from microbit import display, Image, Matrix
import random

def close_enough(m1, m2):
    for r in range(4):
        for c in range(4):
            if abs(m1[r,c] - m2[r,c]) > 0.0005:
                return False
    return True

IDENTITY = Matrix([ [1,0,0,0], [0,1,0,0], [0,0,1,0], [0,0,0,1] ])

def test_mult():
    m= Matrix([
       [ 1,  2,  3,  4],
       [ 5,  6,  7,  8],
       [ 6,  7,  8,  9],
       [ 7,  8,  9, 10]
    ])
    # Ground truth from numpy.
    m2 = Matrix([
       [ 57,  67,  77,  87],
       [133, 159, 185, 211],
       [152, 182, 212, 242],
       [171, 205, 239, 273]
    ])
    assert close_enough(m*m, m2)

def test_mult_id():
    #Seed to make repeatable
    random.seed(111348)
    for i in range(100):
        x = Matrix([ [ random.random() for x in range(4) ] for y in range(4) ])
        assert IDENTITY*x == x
        assert x*IDENTITY == x

def test_invert():
    random.seed(24390149)
    for i in range(100):
        x = Matrix([ [ random.random() for x in range(4) ] for y in range(4) ])
        i = x * x.invert()
        assert close_enough(i, IDENTITY)
        i = x.invert() * x
        assert close_enough(i, IDENTITY)

def test_double_transpose():
    random.seed(17)
    for i in range(100):
        x = Matrix([ [ random.random() for x in range(4) ] for y in range(4) ])
        assert x == x.transpose().transpose()

try:
    test_mult()
    test_mult_id()
    test_invert()
    test_double_transpose()
    print("File test: PASS")
    display.show(Image.HAPPY)
except Exception as ae:
    display.show(Image.SAD)
    raise

