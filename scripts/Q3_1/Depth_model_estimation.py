#! /home/zjy/miniconda3/envs/yolo3D_py38/bin/python
# coding=utf-8

import os
import cv2

from ultralytics import YOLO
from ultralytics.utils.plotting import colorize_depth

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJ_DIR = os.path.dirname(SCRIPT_DIR)  # scripts/

MODEL_PATH = os.path.join(PROJ_DIR, "yolo26s-depth.pt")
INPUT_DIR = os.path.join(PROJ_DIR, "test_images", "src_nuScene_images")
OUTPUT_DIR = os.path.join(PROJ_DIR, "test_images", "real_nuScene_depth_images")
os.makedirs(OUTPUT_DIR, exist_ok=True)

model = YOLO(MODEL_PATH)
result = model(os.path.join(INPUT_DIR, "n008-2018-08-01-15-16-36-0400__CAM_FRONT__1533151603512404.jpg"))[0]

depth = result.depth.data.cpu().numpy()  # (H, W) float32, meters

# Colorize with near = warm and save
cv2.imwrite(os.path.join(OUTPUT_DIR, "depth_colored.png"), colorize_depth(depth, cmap="spectral"))  # (H, W, 3) BGR uint8

# Fix the range to 0-20 m so the same color means the same distance across frames
cv2.imwrite(os.path.join(OUTPUT_DIR, "depth_metric.png"), colorize_depth(depth, vmin=0.0, vmax=20.0, cmap="inferno", mode="metric"))

# Blended overlay straight from the Results object (uses cmap="jet", mode="disparity")
result.save(os.path.join(OUTPUT_DIR, "depth_overlay.png"))
