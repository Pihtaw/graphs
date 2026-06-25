import argparse
import os
import random
import time
from pathlib import Path

import networkx as nx
try:
    import osmnx as ox
except Exception as e:
    raise SystemExit("Установите osmnx: python3 -m pip install osmnx") from e

ox.settings.log_console = False
ox.settings.use_cache = True

# ---------------- defaults ----------------
DEFAULT_CITIES_FILE = "experiments/cities.txt"
OUT_DIR = "data_cities"
DEFAULT_TARGET = 1000
DOWNLOAD_SLEEP = 1.0
# ------------------------------------------

def to_undirected_simple(G):
    """Конвертирует MultiDiGraph/MultiGraph в простой undirected Graph."""
    try:
        if hasattr(ox, "utils_graph") and hasattr(ox.utils_graph, "get_undirected"):
            G_u = ox.utils_graph.get_undirected(G)
        elif hasattr(ox, "get_undirected"):
            G_u = ox.get_undirected(G)
        else:
            G_u = G.to_undirected()
    except Exception:
        G_u = G.to_undirected()

    H = nx.Graph()
    for n, data in G_u.nodes(data=True):
        attrs = data.copy() if isinstance(data, dict) else {}
        H.add_node(n, **attrs)
    for u, v, data in G_u.edges(data=True):
        if u == v:
            continue
        if not H.has_edge(u, v):
            H.add_edge(u, v)
    H.remove_edges_from(list(nx.selfloop_edges(H)))
    return H

def download_graph(place=None, bbox=None, network_type="drive"):
    """Скачивает граф по place или bbox."""
    if place:
        print(f"Downloading graph for place: {place}")
        G = ox.graph_from_place(place, network_type=network_type)
    elif bbox:
        north, south, east, west = bbox
        print(f"Downloading graph for bbox: N={north} S={south} E={east} W={west}")
        G = ox.graph_from_bbox(north, south, east, west, network_type=network_type)
    else:
        raise ValueError("Укажите place или bbox")
    return G

def sample_connected_subgraph(G, target_n):
    """BFS от случайной вершины до target_n, затем взять крупнейшую компоненту."""
    n = G.number_of_nodes()
    if n <= target_n:
        return G.copy()
    nodes_with_deg = [v for v, d in G.degree() if d > 0]
    if not nodes_with_deg:
        chosen = set(random.sample(list(G.nodes()), min(target_n, n)))
        return G.subgraph(chosen).copy()
    seed = random.choice(nodes_with_deg)
    visited = {seed}
    queue = [seed]
    while queue and len(visited) < target_n:
        v = queue.pop(0)
        for w in G.neighbors(v):
            if w not in visited:
                visited.add(w)
                queue.append(w)
                if len(visited) >= target_n:
                    break
    sub = G.subgraph(visited).copy()
    if not nx.is_connected(sub):
        comps = sorted(nx.connected_components(sub), key=len, reverse=True)
        sub = sub.subgraph(comps[0]).copy()
    return sub

def safe_write_edgelist(G, out_path):
    out_path.parent.mkdir(parents=True, exist_ok=True)
    nodes = list(G.nodes())
    idmap = {n: i for i, n in enumerate(nodes)}
    with open(out_path, "w") as f:
        for u, v in G.edges():
            a = idmap[u]; b = idmap[v]
            if a == b: continue
            if a < b:
                f.write(f"{a} {b}\n")
            else:
                f.write(f"{b} {a}\n")

def parse_cities_file(path):
    lines = []
    with open(path, "r", encoding="utf-8") as f:
        for raw in f:
            s = raw.strip()
            if not s or s.startswith("#"):
                continue
            lines.append(s)
    return lines

def is_bbox_line(s):
    parts = [p.strip() for p in s.split(",")]
    if len(parts) != 4:
        return False
    try:
        [float(p) for p in parts]
        return True
    except ValueError:
        return False

def safe_name(s):
    return s.replace(",", "").replace(" ", "_")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cities_file", default=DEFAULT_CITIES_FILE)
    parser.add_argument("--outdir", default=OUT_DIR)
    parser.add_argument("--target_nodes", type=int, default=DEFAULT_TARGET)
    parser.add_argument("--network_type", default="drive")
    parser.add_argument("--sleep", type=float, default=DOWNLOAD_SLEEP)
    args = parser.parse_args()

    random.seed(0)
    ox.settings.use_cache = True

    cities_file = Path(args.cities_file)
    if not cities_file.exists():
        raise SystemExit(f"Cities file not found: {cities_file}")

    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)

    entries = parse_cities_file(cities_file)
    if not entries:
        raise SystemExit("No cities found in file")

    for entry in entries:
        print("------------------------------------------------------------")
        print("Entry:", entry)
        try:
            if is_bbox_line(entry):
                parts = [float(p.strip()) for p in entry.split(",")]
                north, south, east, west = parts
                G_raw = download_graph(bbox=(north, south, east, west), network_type=args.network_type)
                safe = f"bbox_{north}_{south}_{east}_{west}"
            else:
                place = entry
                G_raw = download_graph(place=place, network_type=args.network_type)
                safe = safe_name(place)
        except Exception as e:
            print("Download failed for", entry, ":", e)
            print("Skipping.")
            time.sleep(args.sleep)
            continue

        print("Raw graph nodes:", G_raw.number_of_nodes(), "edges:", G_raw.number_of_edges())
        try:
            G = to_undirected_simple(G_raw)
        except Exception as e:
            print("Conversion to simple graph failed:", e)
            print("Skipping.")
            time.sleep(args.sleep)
            continue

        print("Simple graph nodes:", G.number_of_nodes(), "edges:", G.number_of_edges())
        if G.number_of_nodes() == 0:
            print("Empty graph after conversion, skipping.")
            time.sleep(args.sleep)
            continue

        sub = sample_connected_subgraph(G, args.target_nodes)
        print("Sampled subgraph nodes:", sub.number_of_nodes(), "edges:", sub.number_of_edges())

        out_path = outdir / f"{safe}.edgelist"
        safe_write_edgelist(sub, out_path)
        print("Saved edgelist to", out_path)

        time.sleep(args.sleep)

    print("All done.")

if __name__ == "__main__":
    main()
