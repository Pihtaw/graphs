set -euo pipefail

COMPARE_BIN="./compare"
DATA_DIR="data_cities"
EXPERIMENTS_DIR="experiments_results"
PLOTS_DIR="results/plots"
PLOT_SCRIPT="plot_graphs_consistent_layout.py"

mkdir -p "$EXPERIMENTS_DIR"
mkdir -p "$PLOTS_DIR"

for f in "$DATA_DIR"/*.edgelist; do
  city=$(basename "$f" .edgelist)
  echo
  echo "=== City: $city ==="

  city_out="$EXPERIMENTS_DIR/$city"
  mkdir -p "$city_out/results"

  echo "Running compare on $f ..."
  rm -rf results
  mkdir -p results
  if "$COMPARE_BIN" "$f" 0.9 > "$city_out/compare_stdout.txt" 2> "$city_out/compare_stderr.txt"; then
    echo "compare finished for $city"
  else
    echo "compare failed for $city; see $city_out/compare_stderr.txt"
    continue
  fi

  if [ -d results ]; then
    mv results/* "$city_out/results/" 2>/dev/null || true
    rmdir results 2>/dev/null || true
  fi

  work=".plot_tmp/$city"
  rm -rf "$work"
  mkdir -p "$work/results"
  if [ -f "$city_out/results/orig_remapped.edgelist" ]; then
    cp "$city_out/results/orig_remapped.edgelist" "$work/results/orig_remapped.edgelist"
  else
    cp "$f" "$work/results/orig_remapped.edgelist"
  fi
  cp -n "$city_out/results/algo_old2new.txt" "$work/results/" 2>/dev/null || true
  cp -n "$city_out/results/manual_old2new.txt" "$work/results/" 2>/dev/null || true
  cp -n "$city_out/results/algo_coarsened.edgelist" "$work/results/" 2>/dev/null || true
  cp -n "$city_out/results/manual_coarsened.edgelist" "$work/results/" 2>/dev/null || true
  cp -n "$city_out/results/layout.json" "$work/results/" 2>/dev/null || true

  echo "Running plot script for $city ..."
  pushd "$work" > /dev/null
  if python3 "../../$PLOT_SCRIPT"; then
    echo "Plot script OK for $city"
  else
    echo "Plot script failed for $city"
  fi
  popd > /dev/null

  if [ -f "$work/results/algo_colored.png" ]; then
    mv "$work/results/algo_colored.png" "$PLOTS_DIR/${city}_algo_colored.png"
  fi
  if [ -f "$work/results/manual_colored.png" ]; then
    mv "$work/results/manual_colored.png" "$PLOTS_DIR/${city}_manual_colored.png"
  fi

  if [ -f "$work/results/layout.json" ]; then
    mkdir -p "$city_out/results"
    cp "$work/results/layout.json" "$city_out/results/layout.json"
  fi

  rm -rf "$work"
done

echo
echo "All done. Plots are in $PLOTS_DIR"
