# vo.py (depth-based scale estimation + improved matching)
"""
Monocular VO with depth-derived per-frame scale.
- Reads images, depth (.npy) and meta.json saved by capture.py
- Uses ORB + KNN (Lowe ratio) + Essential matrix + recoverPose
- Computes scale per-frame from matched keypoints using depth maps (median)
- Falls back to GT scale if depth-based scale cannot be estimated
- Saves metric poses to DATA_DIR/vo_poses.txt
"""
import os
import json
import numpy as np
import cv2
import math

# --- CONFIG ---
DATA_DIR = "data/session_01"   # change to your session folder
OUT_F = os.path.join(DATA_DIR, "vo_poses.txt")
MIN_MATCHES = 10               # minimum good matches required to accept a pose update
MAX_MATCHES_USED = 2000
ORB_FEATURES = 4000            # increase ORB features for more keypoints
RATIO_TEST = 0.75              # Lowe ratio
MIN_SCALES_FOR_MEDIAN = 5      # minimum per-match depth samples to accept depth-based scale
# ----------------

# helper: load meta
meta_path = os.path.join(DATA_DIR, "meta.json")
if not os.path.exists(meta_path):
    raise FileNotFoundError("meta.json not found. Run capture.py first and check DATA_DIR path.")

with open(meta_path, "r") as f:
    meta = json.load(f)

frames = meta.get("frames", [])
if len(frames) == 0:
    raise RuntimeError("No frames recorded in meta.json")

# camera intrinsics
K_list = meta.get("camera_info", {}).get("K", None)
if K_list is None:
    raise RuntimeError("camera intrinsics (K) not found in meta.json")
K = np.array(K_list, dtype=float)

# ORB detector + BF matcher (for binary descriptors)
orb = cv2.ORB_create(nfeatures=ORB_FEATURES)
bf = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=False)  # use knnMatch + ratio

# utility functions
def load_gray(relpath):
    p = os.path.join(DATA_DIR, relpath)
    img = cv2.imread(p, cv2.IMREAD_GRAYSCALE)
    if img is None:
        img_c = cv2.imread(p, cv2.IMREAD_COLOR)
        if img_c is None:
            return None
        return cv2.cvtColor(img_c, cv2.COLOR_BGR2GRAY)
    return img

def load_depth(relpath):
    p = os.path.join(DATA_DIR, relpath)
    if not os.path.exists(p):
        return None
    try:
        d = np.load(p)
        # ensure float32
        return d.astype(np.float32)
    except Exception:
        return None

def keypoint_to_3d(pt, depth_img, K):
    """
    Convert pixel coordinate pt=(x,y) to 3D point using depth image and intrinsics K.
    Returns np.array([X,Y,Z]) in camera coordinates or None if invalid.
    """
    if depth_img is None:
        return None
    x_f, y_f = pt[0], pt[1]
    x = int(round(x_f)); y = int(round(y_f))
    h, w = depth_img.shape
    if x < 0 or x >= w or y < 0 or y >= h:
        return None
    z = float(depth_img[y, x])
    if z <= 0 or math.isinf(z) or math.isnan(z):
        return None
    fx = K[0, 0]; fy = K[1, 1]; cx = K[0, 2]; cy = K[1, 2]
    X = (x_f - cx) * z / fx
    Y = (y_f - cy) * z / fy
    return np.array([X, Y, z], dtype=float)

# VO containers
poses_vo = []
R_total = np.eye(3)
t_total = np.zeros((3, 1))
poses_vo.append([float(t_total.item(0)), float(t_total.item(1)), float(t_total.item(2))])

prev_img = None
prev_kp = None
prev_des = None
prev_frame = None  # store full previous frame metadata (so we can access its depth path)

for i in range(len(frames)):
    frame = frames[i]
    img_rel = frame.get("rgb")
    if img_rel is None:
        print(f"[{i}] No rgb path in meta; skipping")
        poses_vo.append([float(t_total.item(0)), float(t_total.item(1)), float(t_total.item(2))])
        continue

    img = load_gray(img_rel)
    if img is None:
        print(f"[{i}] Unable to load image {img_rel}; skipping")
        poses_vo.append([float(t_total.item(0)), float(t_total.item(1)), float(t_total.item(2))])
        continue

    kp, des = orb.detectAndCompute(img, None)

    if prev_img is None:
        # initialize
        prev_img = img
        prev_kp = kp
        prev_des = des
        prev_frame = frame
        poses_vo.append([float(t_total.item(0)), float(t_total.item(1)), float(t_total.item(2))])
        continue

    if des is None or prev_des is None or len(prev_des) == 0 or len(des) == 0:
        prev_img = img; prev_kp = kp; prev_des = des; prev_frame = frame
        poses_vo.append([float(t_total.item(0)), float(t_total.item(1)), float(t_total.item(2))])
        continue

    # knn match + ratio test
    knn_matches = bf.knnMatch(prev_des, des, k=2)
    good_matches = []
    for m_n in knn_matches:
        if len(m_n) != 2:
            continue
        m, n = m_n
        if m.distance < RATIO_TEST * n.distance:
            good_matches.append(m)

    if len(good_matches) < MIN_MATCHES:
        
        prev_img = img; prev_kp = kp; prev_des = des; prev_frame = frame
        poses_vo.append([float(t_total.item(0)), float(t_total.item(1)), float(t_total.item(2))])
        continue

    matches = sorted(good_matches, key=lambda x: x.distance)[:MAX_MATCHES_USED]

    pts1 = np.float32([prev_kp[m.queryIdx].pt for m in matches])
    pts2 = np.float32([kp[m.trainIdx].pt for m in matches])

    # compute essential matrix
    E, mask = cv2.findEssentialMat(pts1, pts2, K, method=cv2.RANSAC, prob=0.999, threshold=1.0)
    if E is None:
        print(f"[{i}] Essential matrix estimation failed.")
        prev_img = img; prev_kp = kp; prev_des = des; prev_frame = frame
        poses_vo.append([float(t_total.item(0)), float(t_total.item(1)), float(t_total.item(2))])
        continue

    _, R, t, mask_pose = cv2.recoverPose(E, pts1, pts2, K)

    # --- DEPTH-BASED SCALE ESTIMATION ---
    # load depth maps for prev and current frames (paths are stored relative to DATA_DIR in meta)
    prev_depth = None
    cur_depth = None
    try:
        prev_depth_rel = prev_frame.get("depth")
        cur_depth_rel = frame.get("depth")
        if prev_depth_rel is not None:
            prev_depth = load_depth(prev_depth_rel)
        if cur_depth_rel is not None:
            cur_depth = load_depth(cur_depth_rel)
    except Exception:
        prev_depth = None
        cur_depth = None

    scales = []
    if prev_depth is not None and cur_depth is not None:
        # compute per-match 3D distances where both depths are valid
        for m in matches:
            p1 = prev_kp[m.queryIdx].pt
            p2 = kp[m.trainIdx].pt
            P1 = keypoint_to_3d(p1, prev_depth, K)
            P2 = keypoint_to_3d(p2, cur_depth, K)
            if P1 is None or P2 is None:
                continue
            d = np.linalg.norm(P2 - P1)
            if d > 0 and not math.isinf(d) and not math.isnan(d):
                scales.append(d)
    # choose scale
    scale = None
    if len(scales) >= MIN_SCALES_FOR_MEDIAN:
        scale = float(np.median(scales))
        # optionally clamp scale to reasonable bounds to avoid spikes
        if scale <= 0 or scale > 1e4:
            scale = None

    # fallback to GT-based scale if depth-based not available
    if scale is None:
        try:
            cur_gt = frame["pose"]["position"]
            prev_gt = prev_frame["pose"]["position"]
            prev_vec = np.array([prev_gt["x"], prev_gt["y"], prev_gt["z"]], dtype=float)
            cur_vec = np.array([cur_gt["x"], cur_gt["y"], cur_gt["z"]], dtype=float)
            gt_scale = np.linalg.norm(cur_vec - prev_vec)
            if gt_scale < 1e-6:
                gt_scale = 1.0
            scale = float(gt_scale)
        except Exception:
            scale = 1.0
  

    else:
        print(f"[{i}] Depth-based scale median from {len(scales)} matches = {scale:.4f}")

    # update global pose: t_total = t_total + scale * R_total * t
    t_total = t_total + scale * (R_total.dot(t))
    R_total = R.dot(R_total)

    poses_vo.append([float(t_total.item(0)), float(t_total.item(1)), float(t_total.item(2))])

    # advance previous frame
    prev_img = img
    prev_kp = kp
    prev_des = des
    prev_frame = frame

# save poses
poses_arr = np.array(poses_vo)
np.savetxt(OUT_F, poses_arr, fmt="%.6f")
print("Saved VO poses to", OUT_F)
