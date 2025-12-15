# capture.py (robust version)
import os
import time
import json
import numpy as np
import cv2
import airsim

# --- configurable ---
OUT_ROOT = "data"
SESSION_NAME = "session_01"      # change per run
CAP_FREQ = 5.0                   # frames per second (adjust)
CAM_NAME = "0"                   # camera name used in Blocks
# ----------------------

out_dir = os.path.join(OUT_ROOT, SESSION_NAME)
os.makedirs(out_dir, exist_ok=True)
img_dir = os.path.join(out_dir, "images")
depth_dir = os.path.join(out_dir, "depth")
os.makedirs(img_dir, exist_ok=True)
os.makedirs(depth_dir, exist_ok=True)

meta = {"frames": []}

client = airsim.MultirotorClient()
client.confirmConnection()
print("Connected to AirSim")

# ensure drone is ready (but don't force takeoff if you want manual control)
client.enableApiControl(True)
client.armDisarm(True)

# get camera info (has FOV) — width/height come from first response
cam_info = client.simGetCameraInfo(CAM_NAME)
fov = getattr(cam_info, "fov", None)  # radians, may be None in some builds

interval = 1.0 / CAP_FREQ
frame_idx = 0

def decode_rgb_response(resp, expected_w=None, expected_h=None):
    """
    Accepts an airsim ImageResponse for RGB and returns a HxWx3 uint8 numpy array (RGB)
    Handles both compressed PNG/JPEG bytes and raw image_data_uint8 arrays.
    """
    # If compressed (PNG/JPEG), image_data_uint8 contains encoded bytes -> use imdecode
    try:
        if resp.compress or (resp.image_data_uint8 is not None and resp.width * resp.height * 3 != len(resp.image_data_uint8)):
            # decode encoded image bytes
            arr = np.frombuffer(resp.image_data_uint8, dtype=np.uint8)
            img = cv2.imdecode(arr, cv2.IMREAD_COLOR)  # BGR
            if img is None:
                return None
            img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
            return img
        else:
            # raw uint8 array (RGB interleaved)
            w = resp.width if hasattr(resp, "width") and resp.width > 0 else expected_w
            h = resp.height if hasattr(resp, "height") and resp.height > 0 else expected_h
            if w is None or h is None:
                return None
            arr = np.frombuffer(resp.image_data_uint8, dtype=np.uint8)
            # sometimes data is returned as flat with length w*h*4 (RGBA) -> handle that
            if arr.size == h * w * 4:
                arr = arr.reshape((h, w, 4))
                arr = arr[:, :, :3]  # drop alpha
            else:
                arr = arr.reshape((h, w, 3))
            # arr is RGB already in AirSim responses
            return arr
    except Exception as e:
        print("RGB decode error:", e)
        return None

def decode_depth_response(resp, expected_w=None, expected_h=None):
    """
    Accepts an airsim ImageResponse for depth (image_data_float) and returns HxW float32 array in meters.
    """
    try:
        w = resp.width if hasattr(resp, "width") and resp.width > 0 else expected_w
        h = resp.height if hasattr(resp, "height") and resp.height > 0 else expected_h
        if w is None or h is None:
            return None
        data = np.array(resp.image_data_float, dtype=np.float32)
        if data.size == w * h:
            return data.reshape((h, w))
        # sometimes depth returns h*w*4 or other shape; try to find best reshape
        if data.size == h * w * 4:
            data = data.reshape((h, w, 4))
            return data[:, :, 0]  # take first channel
        # fallback: return zeros
        return np.zeros((h, w), dtype=np.float32)
    except Exception as e:
        print("Depth decode error:", e)
        return None

# We'll set image_width/height after first frame to compute intrinsics properly
image_width = None
image_height = None

print("Starting capture. Fly the drone now (use joystick/keyboard). Press Ctrl+C to stop.")

try:
    while True:
        t0 = time.time()

        responses = client.simGetImages([
            airsim.ImageRequest(CAM_NAME, airsim.ImageType.Scene, False, False),
            airsim.ImageRequest(CAM_NAME, airsim.ImageType.DepthPerspective, True, False)
        ])

        if not responses or len(responses) < 2:
            print("No responses from sim; retrying")
            time.sleep(0.5)
            continue

        rgb_response = responses[0]
        depth_response = responses[1]

        # If width/height not known yet, read from response
        if image_width is None or image_height is None:
            if hasattr(rgb_response, "width") and rgb_response.width > 0 and hasattr(rgb_response, "height") and rgb_response.height > 0:
                image_width = int(rgb_response.width)
                image_height = int(rgb_response.height)
            elif hasattr(depth_response, "width") and depth_response.width > 0:
                image_width = int(depth_response.width)
                image_height = int(depth_response.height)
            else:
                # fallback to small default
                image_width, image_height = 256, 144
            # compute K using cam_info.fov if available
            if fov is not None:
                fx = fy = (image_width / 2.0) / np.tan(fov / 2.0)
            else:
                # assume a focal length if fov not available
                fx = fy = 500.0
            cx = image_width / 2.0
            cy = image_height / 2.0
            meta["camera_info"] = {
                "camera_name": CAM_NAME,
                "image_width": image_width,
                "image_height": image_height,
                "fov_rad": float(fov) if fov is not None else None,
                "K": [[float(fx), 0.0, float(cx)], [0.0, float(fy), float(cy)], [0.0, 0.0, 1.0]]
            }
            print("Detected image size:", image_width, "x", image_height)
            print("Camera intrinsics K:", meta["camera_info"]["K"])

        timestamp = time.time()

        # get pose (ground truth)
        pose = client.simGetVehiclePose()
        pos = pose.position
        ori = pose.orientation  # quaternion

        # decode / save RGB
        rgb_img = decode_rgb_response(rgb_response, expected_w=image_width, expected_h=image_height)
        if rgb_img is None:
            print("Warning: could not decode rgb frame; skipping frame", frame_idx)
            time.sleep(max(0.0, interval - (time.time() - t0)))
            continue

        # ensure shape is (H,W,3)
        if rgb_img.ndim == 2:
            rgb_img = cv2.cvtColor(rgb_img, cv2.COLOR_GRAY2RGB)
        h, w = rgb_img.shape[:2]
        # if resized needed, optionally resize to target (we keep native returned size)
        rgb_path = os.path.join(img_dir, f"img_{frame_idx:06d}.png")
        # convert RGB->BGR for cv2.imwrite
        cv2.imwrite(rgb_path, cv2.cvtColor(rgb_img, cv2.COLOR_RGB2BGR))

        # decode / save depth
        depth_img = decode_depth_response(depth_response, expected_w=image_width, expected_h=image_height)
        if depth_img is None:
            depth_img = np.zeros((image_height, image_width), dtype=np.float32)
        depth_path = os.path.join(depth_dir, f"depth_{frame_idx:06d}.npy")
        np.save(depth_path, depth_img.astype(np.float32))

        # record meta
        meta["frames"].append({
            "index": frame_idx,
            "timestamp": timestamp,
            "rgb": os.path.relpath(rgb_path, out_dir),
            "depth": os.path.relpath(depth_path, out_dir),
            "pose": {
                "position": {"x": pos.x_val, "y": pos.y_val, "z": pos.z_val},
                "orientation": {"w": ori.w_val, "x": ori.x_val, "y": ori.y_val, "z": ori.z_val}
            }
        })

        if frame_idx % 10 == 0:
            print(f"Captured frame {frame_idx} (size {w}x{h})")

        frame_idx += 1

        # save metadata periodically
        if frame_idx % 10 == 0:
            with open(os.path.join(out_dir, "meta.json"), "w") as f:
                json.dump(meta, f, indent=2)

        # sleep to respect capture frequency
        dt = time.time() - t0
        to_sleep = interval - dt
        if to_sleep > 0:
            time.sleep(to_sleep)

except KeyboardInterrupt:
    print("Capture stopped by user (KeyboardInterrupt)")
finally:
    # final save
    with open(os.path.join(out_dir, "meta.json"), "w") as f:
        json.dump(meta, f, indent=2)
    try:
        client.armDisarm(False)
        client.enableApiControl(False)
    except Exception:
        pass
    print("Saved capture to", out_dir)
