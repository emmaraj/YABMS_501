# -*- coding: utf-8 -*-
"""
Created on Sat Mar 22 11:30:49 2025

@author: Emma
"""

import numpy as np
import os

# Get the current working directory (should be where you're running the script)
cwd = os.getcwd()

# Dataset configurations
datasets = {
    "testing": (16, 12, 8),
    "small": (121, 180, 115),
    "medium": (550, 620, 480),
    "large": (962, 1012, 1221),
    "native": (2500, 3000, 2100),
}

for name, (M, N, P) in datasets.items():
    A = np.random.rand(M, N).astype(np.float32)
    B = np.random.rand(N, P).astype(np.float32)
    R = A @ B

    print(f"\n=== Dataset: {name} ===")
    print("A shape:", A.shape, "size:", A.size)
    print("B shape:", B.shape, "size:", B.size)
    print("R shape:", R.shape, "size:", R.size)

    # Save in current working directory
    A.tofile(os.path.join(cwd, f"A_{name}.bin"))
    B.tofile(os.path.join(cwd, f"B_{name}.bin"))
    R.tofile(os.path.join(cwd, f"R_{name}_gold.bin"))
