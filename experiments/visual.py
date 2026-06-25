#!/usr/bin/env python3
# plot_graphs_consistent_layout.py
# Usage: python3 plot_graphs_consistent_layout.py
# Requirements: networkx, matplotlib, numpy
# Installs: pip install networkx matplotlib numpy

import json
from pathlib import Path
import networkx as nx
import matplotlib.pyplot as plt
import numpy as np

RESULTS = Path("results")
ORIG = RESULTS / "orig_remapped.edgelist"
ALGO_EDGELIST = RESULTS / "algo_coarsened.edgelist"
MANUAL_EDGELIST = RESULTS / "manual_coarsened.edgelist"
ALGO_MAP = RESULTS / "algo_old2new.txt"
MANUAL_MAP = RESULTS / "manual_old2new.txt"
LAYOUT_JSON = RESULTS / "layout.json"

def read_mapping(path):
    mapping = {}
    if not path.exists():
        print("Mapping not found:", path)
        return mapping
    with open(path, "r") as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 2:
                old = int(parts[0]); new = int(parts[1])
                mapping[old] = new
    return mapping

def compute_or_load_layout(G, path):
    if path.exists():
        with open(path, "r") as f:
            data = json.load(f)
        pos = {int(k): tuple(v) for k,v in data.items()}
        return pos
    # compute spring layout with fixed seed and parameters for stability
    pos = nx.spring_layout(G, seed=42, k=0.08, iterations=200)
    # save
    with open(path, "w") as f:
        json.dump({str(k): [float(v[0]), float(v[1])] for k,v in pos.items()}, f)
    return pos

def draw_colored(origG, pos, mapping, title, outpng):
    # mapping: old->new. We consider a vertex 'representative' if it is the first occurrence of its new id.
    # Build representative set: for each new id, pick the smallest old index that maps to it.
    rep = {}
    for old, new in mapping.items():
        if new not in rep or old < rep[new]:
            rep[new] = old
    rep_set = set(rep.values())
    # Color: blue for representatives (nodes that "survived" as representative), green for merged nodes (others)
    node_colors = []
    for n in origG.nodes():
        if n in rep_set:
            node_colors.append("#1f77b4")  # blue
        else:
            node_colors.append("#2ca02c")  # green
    plt.figure(figsize=(10,10))
    nx.draw_networkx_nodes(origG, pos, node_size=20, node_color=node_colors)
    nx.draw_networkx_edges(origG, pos, alpha=0.25, width=0.6)
    plt.title(title)
    plt.axis('off')
    plt.tight_layout()
    plt.savefig(outpng, dpi=200)
    plt.close()
    print("Saved", outpng)

def main():
    if not ORIG.exists():
        print("Original remapped edgelist not found:", ORIG)
        return
    G = nx.read_edgelist(ORIG, nodetype=int)
    pos = compute_or_load_layout(G, LAYOUT_JSON)

    algo_map = read_mapping(ALGO_MAP)
    manual_map = read_mapping(MANUAL_MAP)

    if algo_map:
        draw_colored(G, pos, algo_map, f"Algo coarsened (n={len(set(algo_map.values()))})", RESULTS / "algo_colored.png")
    else:
        print("Algo mapping missing; skipping algo plot.")

    if manual_map:
        draw_colored(G, pos, manual_map, f"Manual coarsened (n={len(set(manual_map.values()))})", RESULTS / "manual_colored.png")
    else:
        print("Manual mapping missing; skipping manual plot.")

if __name__ == "__main__":
    main()
