import airsim
import keyboard
import time

client = airsim.MultirotorClient()
client.confirmConnection()
client.enableApiControl(True)
client.armDisarm(True)

speed = 2

while True:

    if keyboard.is_pressed("w"):
        client.moveByVelocityAsync(speed, 0, 0, 0.1)

    if keyboard.is_pressed("s"):
        client.moveByVelocityAsync(-speed, 0, 0, 0.1)

    if keyboard.is_pressed("a"):
        client.moveByVelocityAsync(0, -speed, 0, 0.1)

    if keyboard.is_pressed("d"):
        client.moveByVelocityAsync(0, speed, 0, 0.1)

    if keyboard.is_pressed("space"):
        client.hoverAsync()

    time.sleep(0.03)

