import airsim

client = airsim.MultirotorClient()
client.confirmConnection()
client.enableApiControl(True)
client.armDisarm(True)

client.takeoffAsync().join()
client.moveByVelocityZAsync(1, 0, -3, 5).join()

responses = client.simGetImages([
    airsim.ImageRequest("0", airsim.ImageType.Scene, False, False),
    airsim.ImageRequest("0", airsim.ImageType.DepthPerspective, True)
])

pose = client.simGetVehiclePose()
print("Position:", pose.position)
print("Orientation:", pose.orientation)

