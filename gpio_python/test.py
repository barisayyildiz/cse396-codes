import RPi.GPIO as GPIO
import time
GPIO.setmode(GPIO.BCM)
control_pins = [1,7,8,25]
for pin in control_pins:
  GPIO.setup(pin, GPIO.OUT)
  GPIO.output(pin, 0)

stepNumber = 0
def oneStep():
	global stepNumber
	if stepNumber == 0:
		GPIO.output(control_pins[0], 0)
		GPIO.output(control_pins[1], 0)
		GPIO.output(control_pins[2], 0)
		GPIO.output(control_pins[3], 1)
	elif stepNumber == 1:
		GPIO.output(control_pins[0], 0)
		GPIO.output(control_pins[1], 0)
		GPIO.output(control_pins[2], 1)
		GPIO.output(control_pins[3], 0)
	elif stepNumber == 2:
		GPIO.output(control_pins[0], 0)
		GPIO.output(control_pins[1], 1)
		GPIO.output(control_pins[2], 0)
		GPIO.output(control_pins[3], 0)
	elif stepNumber == 3:
		GPIO.output(control_pins[0], 1)
		GPIO.output(control_pins[1], 0)
		GPIO.output(control_pins[2], 0)
		GPIO.output(control_pins[3], 0)
	stepNumber += 1
	if stepNumber > 3:
		stepNumber = 0
	return stepNumber

halfstep_seq = [
  [1,0,0,0],
  [1,1,0,0],
  [0,1,0,0],
  [0,1,1,0],
  [0,0,1,0],
  [0,0,1,1],
  [0,0,0,1],
  [1,0,0,1]
]

for i in range(2048):
  oneStep()
  time.sleep(0.01)

# for i in range(512):
#   for halfstep in range(8):
#     for pin in range(4):
#       GPIO.output(control_pins[pin], halfstep_seq[halfstep][pin])
#     time.sleep(0.001)
GPIO.cleanup()