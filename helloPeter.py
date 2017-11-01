#from sense_emu import SenseHat
from sense_hat import SenseHat
sense = SenseHat()


# Display "Hello Peter" twice

color = (223, 138, 0)
bgColor = (30, 0, 60)
rounds = 2
while rounds > 0:
    rounds -= 1
    sense.show_message("Hello PETER", 0.2, color, bgColor)

sense.clear((0, 0, 0))

