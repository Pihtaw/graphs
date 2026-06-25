import argparse
from pathlib import Path
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from collections import Counter, defaultdict

sns.set(style="whitegrid")

def collect_compare_results(results_root: Path):
    rows = []
    for citydir in sorted(results_root.iterdir()):
        if not citydir.is_dir(): continue
        csvp = citydir / "results" / "compare_results.csv"
        if not csvp.exists(): 
            print("Missing compare_results.csv for", citydir.name)
            continue
        try:
            df = pd.read_csv(csvp)
        except Exception as e:
            print("Failed to read", csvp, ":", e)
            continue
        df.columns = [c.strip() for c in df.columns]
        if 'algorithm' not in df.columns:
            print("No 'algorithm' column in", csvp, "- skipping")
            continue
        df['city'] = citydir.name
        rows.append(df)
    if not rows:
        return pd.DataFrame()
    return pd.concat(rows, ignore_index=True)

def parse_old2new(path: Path):
    """
    Возвращает dict old->new (ints). Игнорирует некорректные строки.
    """
    mapping = {}
    if not path.exists():
        return mapping
    with open(path, "r") as f:
        for line in f:
            s = line.strip()
            if not s: continue
            parts = s.split()
            if len(parts) < 2: continue
            try:
                old = int(parts[0]); new = int(parts[1])
                mapping[old] = new
            except:
                continue
    return mapping

def cluster_size_counts_from_mapping(mapping: dict):
    """
    mapping: old->new
    Возвращает Counter размеров кластеров: {size: count_of_clusters_with_that_size}
    и список размеров (для детальной статистики).
    """
    if not mapping:
        return Counter(), []
    inv = defaultdict(list)
    for old, new in mapping.items():
        inv[new].append(old)
    sizes = [len(members) for members in inv.values()]
    cnt = Counter(sizes)
    return cnt, sizes

def build_algo_cluster_table(results_root: Path):
    """
    Для каждого города парсит algo_old2new.txt и возвращает DataFrame:
    city, cluster_size, clusters_count
    """
    rows = []
    sizes_by_city = {}
    for citydir in sorted(results_root.iterdir()):
        if not citydir.is_dir(): continue
        algo_map = citydir / "results" / "algo_old2new.txt"
        mapping = parse_old2new(algo_map)
        cnt, sizes = cluster_size_counts_from_mapping(mapping)
        sizes_by_city[citydir.name] = sizes
        for s in range(1,6):
            rows.append({"city": citydir.name, "cluster_size": s, "clusters_count": int(cnt.get(s,0))})
        gt5 = sum(v for k,v in cnt.items() if k>5)
        rows.append({"city": citydir.name, "cluster_size": ">5", "clusters_count": int(gt5)})
    return pd.DataFrame(rows), sizes_by_city

def plot_compression_by_city(df_compare: pd.DataFrame, out_dir: Path):
    out_dir.mkdir(parents=True, exist_ok=True)
    df_compare['compression_ratio'] = pd.to_numeric(df_compare['compression_ratio'], errors='coerce')
    summary = df_compare.groupby(['city','algorithm'])['compression_ratio'].agg(['mean','median','count']).reset_index()
    summary.to_csv(out_dir / "compression_summary.csv", index=False)
    plt.figure(figsize=(12,6))
    sns.barplot(data=summary, x='city', y='mean', hue='algorithm')
    plt.xticks(rotation=45, ha='right')
    plt.ylabel("Mean compression_ratio")
    plt.title("Mean compression ratio by city and algorithm")
    plt.tight_layout()
    plt.savefig(out_dir / "compression_by_city.png", dpi=200)
    plt.close()
    plt.figure(figsize=(14,6))
    sns.boxplot(data=df_compare, x='city', y='compression_ratio', hue='algorithm')
    plt.xticks(rotation=45, ha='right')
    plt.ylabel("compression_ratio")
    plt.title("Compression ratio distribution by city and algorithm")
    plt.tight_layout()
    plt.savefig(out_dir / "compression_box_by_city.png", dpi=200)
    plt.close()
    print("Saved compression plots and summary to", out_dir)

def plot_algo_cluster_counts(df_clusters: pd.DataFrame, out_dir: Path):
    out_dir.mkdir(parents=True, exist_ok=True)
    pivot = df_clusters.pivot(index='city', columns='cluster_size', values='clusters_count').fillna(0)
    pivot = pivot[['1','2','3','4','5','>5']] if set(['1','2','3','4','5','>5']).issubset(pivot.columns) else pivot
    pivot.plot(kind='bar', figsize=(14,6))
    plt.ylabel("Number of clusters")
    plt.title("Cluster size counts (algo) per city")
    plt.xticks(rotation=45, ha='right')
    plt.tight_layout()
    plt.savefig(out_dir / "algo_cluster_counts_by_city.png", dpi=200)
    plt.close()
    df_clusters.to_csv(out_dir / "algo_cluster_counts.csv", index=False)
    print("Saved algo cluster counts plot and CSV to", out_dir)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--results_root", default="experiments_results")
    parser.add_argument("--out_dir", default="results/plots")
    args = parser.parse_args()

    results_root = Path(args.results_root)
    out_plots = Path(args.out_dir)
    out_summary = out_plots.parent / "summary"
    out_summary.mkdir(parents=True, exist_ok=True)

    print("Collecting compare results...")
    df_compare = collect_compare_results(results_root)
    if df_compare.empty:
        print("No compare results found. Exiting.")
        return

    print("Building algo cluster table...")
    df_clusters, sizes_by_city = build_algo_cluster_table(results_root)

    print("Plotting compression by city...")
    plot_compression_by_city(df_compare, out_plots)

    print("Plotting algo cluster counts...")
    plot_algo_cluster_counts(df_clusters, out_plots)

    summary_rows = []
    for city, sizes in sizes_by_city.items():
        if not sizes:
            summary_rows.append({"city": city, "clusters_total": 0, "mean_size": None, "median_size": None})
            continue
        arr = np.array(sizes)
        summary_rows.append({
            "city": city,
            "clusters_total": int(len(arr)),
            "mean_size": float(arr.mean()),
            "median_size": float(np.median(arr)),
            "min_size": int(arr.min()),
            "max_size": int(arr.max())
        })
    pd.DataFrame(summary_rows).to_csv(out_summary / "algo_cluster_size_stats.csv", index=False)
    print("Saved cluster size stats to", out_summary / "algo_cluster_size_stats.csv")

    print("All done. Plots in", out_plots, "summaries in", out_summary)

if __name__ == "__main__":
    main()
