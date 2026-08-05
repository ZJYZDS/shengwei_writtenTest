#! /home/zjy/miniconda3/envs/yolo3D_py38/bin/python
# coding=utf-8


import cv2
import os
from ultralytics import YOLO
from ultralytics.utils.plotting import colorize_depth

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJ_DIR = os.path.dirname(SCRIPT_DIR)  # scripts/

MODEL_PATH = os.path.join(PROJ_DIR, "yolo26s-depth.pt")
OUTPUT_DIR = os.path.join(PROJ_DIR, "test_images", "coco_example_depth_images")
os.makedirs(OUTPUT_DIR, exist_ok=True)

model = YOLO(MODEL_PATH)
result = model("https://ultralytics.com/images/bus.jpg")[0]

depth = result.depth.data.cpu().numpy()  # (H, W) float32, meters

# Colorize with near = warm and save
cv2.imwrite(os.path.join(OUTPUT_DIR, "example.png"), colorize_depth(depth, cmap="spectral"))  # (H, W, 3) BGR uint8

# Fix the range to 0-20 m so the same color means the same distance across frames
cv2.imwrite(os.path.join(OUTPUT_DIR, "example_metric.png"), colorize_depth(depth, vmin=0.0, vmax=20.0, cmap="inferno", mode="metric"))

# Blended overlay straight from the Results object (uses cmap="jet", mode="disparity")
result.save(os.path.join(OUTPUT_DIR, "example_overlay.png"))
