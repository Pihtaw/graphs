import pandas as pd
from pathlib import Path
import matplotlib.pyplot as plt
import seaborn as sns
import argparse

def collect_csvs(results_root):
    rows = []
    for citydir in Path(results_root).iterdir():
        if not citydir.is_dir(): continue
        for run in citydir.iterdir():
            if not run.is_dir(): continue
            csvp = run / "results" / "compare_results.csv"
            if csvp.exists():
                df = pd.read_csv(csvp)
                for _, r in df.iterrows():
                    rdict = r.to_dict()
                    rdict['city'] = citydir.name
                    rdict['run'] = run.name
                    rows.append(rdict)
    return pd.DataFrame(rows)

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--results_root", default="experiments_results")
    p.add_argument("--out_dir", default="results/plots")
    args = p.parse_args()
    out = Path(args.out_dir); out.mkdir(parents=True, exist_ok=True)
    df = collect_csvs(args.results_root)
    if df.empty:
        print("No data found"); return
    df['compression_ratio'] = pd.to_numeric(df['compression_ratio'], errors='coerce')
    df['time_ms'] = pd.to_numeric(df['time_ms'], errors='coerce')
    plt.figure(figsize=(12,6))
    sns.boxplot(x='city', y='time_ms', hue='algorithm', data=df)
    plt.xticks(rotation=45, ha='right'); plt.title('Time per city'); plt.tight_layout()
    plt.savefig(out / "time_per_city.png", dpi=200); plt.close()
    plt.figure(figsize=(12,6))
    sns.boxplot(x='city', y='compression_ratio', hue='algorithm', data=df)
    plt.xticks(rotation=45, ha='right'); plt.title('Compression per city'); plt.tight_layout()
    plt.savefig(out / "compression_per_city.png", dpi=200); plt.close()
    # scatter time vs compression
    plt.figure(figsize=(8,6))
    sns.scatterplot(x='compression_ratio', y='time_ms', hue='algorithm', data=df, alpha=0.6)
    plt.title('Time vs Compression'); plt.tight_layout()
    plt.savefig(out / "time_vs_compression.png", dpi=200); plt.close()
    print("Saved plots to", out)

if __name__ == "__main__":
    main()
