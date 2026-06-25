import networkx as nx
import matplotlib.pyplot as plt
import json
from pathlib import Path
import argparse

def read_map(path):
    m = {}
    with open(path) as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 2:
                old = int(parts[0]); new = int(parts[1])
                m[old] = new
    return m

def compute_or_load_layout(G, layout_path):
    if layout_path.exists():
        with open(layout_path) as f:
            pos = json.load(f)
        return {int(k): tuple(v) for k,v in pos.items()}
    pos = nx.spring_layout(G, seed=42, k=0.08, iterations=200)
    with open(layout_path, "w") as f:
        json.dump({str(k): [float(v[0]), float(v[1])] for k,v in pos.items()}, f)
    return pos

def node_colors_from_mapping(G, mapping):
    rep = {}
    for old,new in mapping.items():
        if new not in rep or old < rep[new]:
            rep[new] = old
    rep_set = set(rep.values())
    colors = []
    for n in G.nodes():
        colors.append("#1f77b4" if n in rep_set else "#2ca02c")
    return colors

def plot_three(orig_edgelist, algo_map, manual_map, outpng, layout_json):
    G = nx.read_edgelist(orig_edgelist, nodetype=int)
    pos = compute_or_load_layout(G, Path(layout_json))
    algo_m = read_map(algo_map)
    manual_m = read_map(manual_map)
    algo_colors = node_colors_from_mapping(G, algo_m)
    manual_colors = node_colors_from_mapping(G, manual_m)
    plt.figure(figsize=(15,5))
    plt.subplot(1,3,1)
    nx.draw_networkx_nodes(G, pos, node_size=20, node_color="#1f77b4")
    nx.draw_networkx_edges(G, pos, alpha=0.25, width=0.6)
    plt.title("Original"); plt.axis('off')
    plt.subplot(1,3,2)
    nx.draw_networkx_nodes(G, pos, node_size=20, node_color=algo_colors)
    nx.draw_networkx_edges(G, pos, alpha=0.25, width=0.6)
    plt.title("Algo"); plt.axis('off')
    plt.subplot(1,3,3)
    nx.draw_networkx_nodes(G, pos, node_size=20, node_color=manual_colors)
    nx.draw_networkx_edges(G, pos, alpha=0.25, width=0.6)
    plt.title("Manual"); plt.axis('off')
    plt.tight_layout()
    plt.savefig(outpng, dpi=200)
    plt.close()

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--results_dir", default="experiments_results")
    p.add_argument("--data_dir", default="data_cities")
    p.add_argument("--out_dir", default="results/plots")
    p.add_argument("--examples_per_city", type=int, default=1, help="how many examples per city (use 1 if only one edgelist)")
    args = p.parse_args()

    out = Path(args.out_dir); out.mkdir(parents=True, exist_ok=True)
    for citydir in sorted(Path(args.results_dir).iterdir()):
        if not citydir.is_dir(): continue
        resdir = citydir / "results"
        if not resdir.exists(): 
            print("No results for", citydir.name); continue
        orig = resdir / "orig_remapped.edgelist"
        if not orig.exists():
            candidate = Path(args.data_dir) / (citydir.name + ".edgelist")
            if candidate.exists():
                orig = candidate
            else:
                print("No orig edgelist for", citydir.name); continue
        algo_map = resdir / "algo_old2new.txt"
        manual_map = resdir / "manual_old2new.txt"
        if not algo_map.exists() or not manual_map.exists():
            print("Missing mapping files for", citydir.name); continue
        layout_json = citydir / "layout.json"
        outpng = out / f"{citydir.name}_three_panel.png"
        plot_three(str(orig), str(algo_map), str(manual_map), str(outpng), str(layout_json))
        print("Saved", outpng)
