#from sense_emu import SenseHat
from sense_hat import SenseHat
sense = SenseHat()

# turn the display so it can stand with the cable coming out at the top
sense.set_rotation(180)

# Display message 'rounds' times:

color = (223, 138, 0)
bgColor = (30, 0, 60)
rounds = 20
while rounds > 0:
    rounds -= 1
    sense.show_message("Hello BCS family", 0.2, color, bgColor)

sense.clear((0, 0, 0))

