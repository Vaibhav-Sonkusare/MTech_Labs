# Drone Technology Project

## Overview

This project demonstrates **environment perception and pose estimation of a UAV** using the **AirSim simulator**. A multirotor drone is flown manually in a simulated environment, sensor data is captured (RGB images, depth maps, and ground-truth pose), Visual Odometry (VO) is computed, and the estimated trajectory is compared against ground truth.

---

## Requirements

### Software

* **Windows 10 / 11** (Linux supported, but Windows recommended)
* **AirSim (prebuilt binary – Blocks / LandscapeMountains)**
* **Python 3.8 / 3.9 / 3.10**
* **Game Controller or Keyboard** (for manual flight)

---

### Python Libraries

Install the required Python packages:

```bash
pip install numpy msgpack-rpc-python opencv-python matplotlib scipy
pip install airsim
```

---

## Step-by-Step Execution Guide

### Step 1: Download AirSim Binary

1. Open the official AirSim release page: [https://github.com/microsoft/AirSim/releases](https://github.com/microsoft/AirSim/releases)
2. Download **Blocks.zip** (used for this project).

   * Other environments such as `Africa.zip` or `LandscapeMountains.zip` can also be used.
3. Extract the downloaded zip file.
4. Run `Blocks.exe` once to ensure AirSim initializes correctly.

---

### Step 2: Configure AirSim

1. Locate the AirSim settings file:

   ```
   Documents/AirSim/settings.json
   ```

   > If the file does not exist, run `Blocks.exe` once and close it. The file will be created automatically.

2. Replace the contents of `settings.json` with the provided configuration file.

   * RGB and Depth cameras enabled
   * Keyboard / Game Controller enabled for drone control

3. Launch **Blocks.exe** again.

4. When prompted to select a vehicle, choose **Multirotor** (if prompted).

---

### Step 3: Capture Dataset

This step records sensor data while manually flying the drone.

1. Activate your Python virtual environment (if used).

2. Start the AirSim simulator (`Blocks.exe`).

3. Run the capture script:

   ```bash
   python capture.py
   ```

4. Fly the drone using the keyboard or game controller.

5. Press **Ctrl + C** in the terminal to stop recording.

**Generated Output:**

* RGB images (`.png`)
* Depth maps (`.npy`)
* `meta.json` containing:

  * Camera intrinsics
  * Image paths
  * Ground-truth drone pose

---

### Step 4: Compute Visual Odometry

This step estimates the drone trajectory using image sequences and depth information.

```bash
python vo.py
```

**Processing Steps:**

* ORB features detected and matched between frames
* Camera motion (rotation and translation) estimated
* Depth used to recover real-world scale
* Ground-truth scale used as fallback when depth is unreliable

**Output File:**

```
data/session_01/vo_poses.txt
```

---

### Step 5: Analyze and Plot Trajectories

This step aligns the estimated VO trajectory with ground truth and visualizes the result.

```bash
python analyze.py
```

**Results:**

* RMSE values printed in the terminal
* Comparison plot saved as:

```
data/session_01/trajectory_compare_aligned.png
```

**Plot Contents:**

* Top-down trajectory comparison (X–Y)
* Height comparison over time (Z)

---

## Key Concepts Used

* Monocular Visual Odometry
* ORB feature detection and matching
* Essential matrix and pose recovery
* Depth-based scale estimation
* Ground-truth pose from AirSim
* Umeyama alignment for evaluation
* RMSE error computation

---

## Notes and Tips

* Fly the drone **slowly and smoothly** for stable VO estimation.
* Avoid fast rotations and abrupt altitude changes.
* Higher capture FPS improves feature continuity.
* Savitzky–Golay smoothing requires the `scipy` package.
* In AirSim, the **Z-axis is negative upward**.

