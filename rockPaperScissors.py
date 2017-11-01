#from sense_emu import SenseHat
from sense_hat import SenseHat
sense = SenseHat()
from time import sleep
from random import randint
from random import choice

# turn the display so it can stand with the cable coming out at the top
sense.set_rotation(180)

rockPaperScissors = ["R", "P", "S"]

# Generate a random colour
def pick_random_colour():
  random_red = randint(0, 255)
  random_green = randint(0, 255)
  random_blue = randint(0, 255)
  return (random_red, random_green, random_blue)

while True:
    red = (255, 0, 0) 
    count = 1
    while count < 4:
        sense.show_letter(str(count), pick_random_colour())
        sleep(1)
        count += 1

    selection = choice(rockPaperScissors)
    sense.show_letter(selection, red)
    sleep(5)
    sense.clear((0, 0, 0))
    sleep(1)


