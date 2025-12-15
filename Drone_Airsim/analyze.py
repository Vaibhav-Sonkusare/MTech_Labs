
import os
import json
import numpy as np
import matplotlib.pyplot as plt
import math

# --- CONFIGURE --- 
DATA_DIR = "data/session_01"        # update if your session folder is different
META_F = os.path.join(DATA_DIR, "meta.json")
VO_F = os.path.join(DATA_DIR, "vo_poses.txt")
OUT_PNG = os.path.join(DATA_DIR, "trajectory_compare_aligned.png")

SMOOTH = True                       # enable smoothing of VO before alignment
SMOOTH_METHOD = "savgol"            # "savgol" or "moving"
SAVGOL_WINDOW = 7                   # must be odd and >=3
SAVGOL_POLY = 2
MOVING_WIN = 5                      # moving average window if savgol not available or chosen

USE_INITIAL_SEGMENT = False         # set True to compute alignment using a subset of initial frames
ALIGN_FIRST_N = 40
# ------------------

# try to import savgol_filter; fallback to None
try:
    from scipy.signal import savgol_filter  # type: ignore
    _HAS_SAVGOL = True
except Exception:
    _HAS_SAVGOL = False

def umeyama_align(A: np.ndarray, B: np.ndarray, with_scaling: bool = True):
    """
    Umeyama similarity transform: find s, R, t such that s * R * A + t = B
    A, B: (N x 3) arrays of corresponding points
    Returns (s, R, t)
    """
    assert A.shape == B.shape and A.shape[1] == 3
    n = A.shape[0]
    mu_A = A.mean(axis=0)
    mu_B = B.mean(axis=0)
    X = A - mu_A
    Y = B - mu_B
    cov = (Y.T @ X) / n
    U, D, Vt = np.linalg.svd(cov)
    S = np.eye(3)
    if np.linalg.det(U) * np.linalg.det(Vt) < 0:
        S[-1, -1] = -1.0
    R = U @ S @ Vt
    if with_scaling:
        varA = (X**2).sum() / n
        # trace(D * S) where D is vector of singular values
        s = float(np.sum(D * np.diag(S))) / float(varA) if varA != 0 else 1.0
    else:
        s = 1.0
    t = mu_B - s * (R @ mu_A)
    return s, R, t

def smooth_traj(traj: np.ndarray, method="savgol", win=7, poly=2, movwin=5):
    """
    Smooth Nx3 trajectory. If savgol available and chosen, use it.
    Otherwise use simple moving average.
    """
    if traj is None or traj.size == 0:
        return traj
    if method == "savgol" and _HAS_SAVGOL:
        if win % 2 == 0:
            win += 1
        win = max(win, 3)
        if traj.shape[0] < win:
            return traj.copy()
        out = np.zeros_like(traj)
        out[:, 0] = savgol_filter(traj[:, 0], window_length=win, polyorder=poly, mode='interp')
        out[:, 1] = savgol_filter(traj[:, 1], window_length=win, polyorder=poly, mode='interp')
        out[:, 2] = savgol_filter(traj[:, 2], window_length=win, polyorder=poly, mode='interp')
        return out
    else:
        # moving average fallback
        N = traj.shape[0]
        out = np.zeros_like(traj)
        half = max(1, movwin // 2)
        for i in range(N):
            lo = max(0, i - half)
            hi = min(N, i + half + 1)
            out[i] = traj[lo:hi].mean(axis=0)
        return out

def load_data(meta_f: str, vo_f: str):
    if not os.path.exists(meta_f):
        raise FileNotFoundError(f"meta.json not found at: {meta_f}")
    if not os.path.exists(vo_f):
        raise FileNotFoundError(f"vo_poses.txt not found at: {vo_f}")
    with open(meta_f, "r") as fh:
        meta = json.load(fh)
    frames = meta.get("frames", [])
    if len(frames) == 0:
        raise RuntimeError("No frames found inside meta.json")
    gt = np.array([[f["pose"]["position"]["x"],
                    f["pose"]["position"]["y"],
                    f["pose"]["position"]["z"]] for f in frames], dtype=float)
    vo = np.loadtxt(vo_f)
    if vo.ndim == 1:
        vo = vo.reshape((-1, 3))
    return gt, vo

def main():
    gt, vo = load_data(META_F, VO_F)
    minlen = min(len(gt), len(vo))
    gt = gt[:minlen]
    vo = vo[:minlen]

    # smoothing
    if SMOOTH:
        if SMOOTH_METHOD == "savgol" and not _HAS_SAVGOL:
            # fall back to moving average if scipy not installed
            vo = smooth_traj(vo, method="moving", movwin=MOVING_WIN)
        else:
            vo = smooth_traj(vo, method=SMOOTH_METHOD, win=SAVGOL_WINDOW, poly=SAVGOL_POLY, movwin=MOVING_WIN)

    # choose subset for alignment
    if USE_INITIAL_SEGMENT and minlen >= ALIGN_FIRST_N:
        A = vo[:ALIGN_FIRST_N]
        B = gt[:ALIGN_FIRST_N]
    else:
        A = vo
        B = gt

    # compute Umeyama alignment (vo -> gt)
    s, R, t = umeyama_align(A, B, with_scaling=True)
    # apply to full (smoothed) vo
    vo_aligned = (s * (R @ vo.T)).T + t

    # compute RMSE
    err = vo_aligned - gt
    rmse_xyz = np.sqrt(np.mean(err**2, axis=0))
    rmse_total = np.sqrt(np.mean(np.sum(err**2, axis=1)))

    print("Aligned RMSE (X, Y, Z):", np.round(rmse_xyz, 4))
    print("Total RMSE (meters): {:.4f}".format(rmse_total))
    print("Similarity transform scale:", float(s))

    # Plot GT and VO (aligned) only
    plt.figure(figsize=(12, 5))

    plt.subplot(1, 2, 1)
    plt.plot(gt[:, 0], gt[:, 1], label="GT", linewidth=1.8)
    plt.plot(vo_aligned[:, 0], vo_aligned[:, 1], label="VO", linewidth=1.4)
    plt.title("Top-down trajectory (X vs Y)")
    plt.xlabel("X (m)")
    plt.ylabel("Y (m)")
    plt.legend()
    plt.axis('equal')

    plt.subplot(1, 2, 2)
    plt.plot(gt[:, 2], label="GT", linewidth=1.8)
    plt.plot(vo_aligned[:, 2], label="VO", linewidth=1.4)
    plt.title("Height over frames (Z)")
    plt.xlabel("frame index")
    plt.ylabel("Z (m)")
    plt.legend()

    plt.tight_layout()
    plt.savefig(OUT_PNG, dpi=150)
    print("Saved aligned trajectory plot to:", OUT_PNG)
    plt.show()

if __name__ == "__main__":
    main()
