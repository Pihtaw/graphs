#include <bits/stdc++.h>
#include <chrono>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <direct.h>
#endif

using namespace std;
using namespace std::chrono;

using vi = vector<int>;
using vvi = vector<vi>;

extern tuple<int,int,long long> manual_coarsen(const vvi &adj_orig, int max_pairs);
extern pair<vvi, vector<int>> manual_coarsen_full(const vvi &adj_orig, int max_pairs);
extern pair<vvi, vector<int>> algo_coarsen_full(const vvi &adj_orig,
                                                double ef5, double ef4, double ef3, double ef2,
                                                int max_motifs_per_size, bool merge_leaves_first,
                                                int max_total_motifs_to_apply);

vvi read_edgelist_remap(const string &filename, int &mapped_nodes, int &edges, vector<pair<int,int>> &orig_edges) {
    ifstream in(filename);
    if (!in) { cerr << "Failed to open " << filename << "\n"; return {}; }
    unordered_map<long long,int> idmap;
    orig_edges.clear();
    string line;
    mapped_nodes = 0;
    edges = 0;
    while (getline(in, line)) {
        if (line.empty()) continue;
        if (line[0] == '#') continue;
        stringstream ss(line);
        long long a,b;
        if (!(ss >> a >> b)) continue;
        if (!idmap.count(a)) idmap[a] = mapped_nodes++;
        if (!idmap.count(b)) idmap[b] = mapped_nodes++;
        int u = idmap[a], v = idmap[b];
        orig_edges.emplace_back(u,v);
        edges++;
    }
    vvi adj(mapped_nodes);
    for (auto &e : orig_edges) {
        int u = e.first, v = e.second;
        if (u == v) continue;
        adj[u].push_back(v);
        adj[v].push_back(u);
    }
    for (int i=0;i<mapped_nodes;++i) {
        sort(adj[i].begin(), adj[i].end());
        adj[i].erase(unique(adj[i].begin(), adj[i].end()), adj[i].end());
    }
    return adj;
}

static void write_edgelist(const string &path, const vvi &adj) {
    ofstream out(path);
    if (!out) { cerr << "Failed to open " << path << " for writing\n"; return; }
    int n = adj.size();
    for (int u=0; u<n; ++u) {
        for (int v : adj[u]) if (u < v) out << u << " " << v << "\n";
    }
    out.close();
}

static void write_mapping(const string &path, const vector<int> &old2new) {
    ofstream out(path);
    if (!out) { cerr << "Failed to open " << path << " for writing mapping\n"; return; }
    for (size_t i=0;i<old2new.size();++i) out << i << " " << old2new[i] << "\n";
    out.close();
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <edgelist> [ef_threshold]\n";
        return 1;
    }
    string path = argv[1];
    double ef_threshold = 0.9;
    if (argc >= 3) ef_threshold = atof(argv[2]);

    int mapped_nodes = 0, edges = 0;
    vector<pair<int,int>> orig_edges;
    auto adj = read_edgelist_remap(path, mapped_nodes, edges, orig_edges);
    if (adj.empty()) return 1;
    int orig_n = adj.size();
    cerr << "read_edgelist_remap: lines=" << edges << " mapped_nodes=" << mapped_nodes << " edges=" << edges << "\n";
    cerr << "Loaded graph: n=" << orig_n << ", edges(undirected)=" << edges << "\n";

    string outdir = "results";
    #ifdef _WIN32
    _mkdir(outdir.c_str());
    #else
    mkdir(outdir.c_str(), 0755);
    #endif

    {
        string orig_out = outdir + "/orig_remapped.edgelist";
        ofstream out(orig_out);
        if (out) {
            for (auto &e : orig_edges) {
                int u = e.first, v = e.second;
                if (u == v) continue;
                if (u < v) out << u << " " << v << "\n";
                else out << v << " " << u << "\n";
            }
            out.close();
            cerr << "Wrote remapped original edgelist to " << orig_out << "\n";
        }
    }

    int manual_merges = 0, manual_new_n = orig_n;
    long long manual_time = 0;
    vvi adj_manual_final;
    vector<int> manual_old2new;
    {
        auto t0 = high_resolution_clock::now();
        auto res = manual_coarsen(adj, 10000);
        auto [applied, new_n, ms] = res;
        manual_merges = applied;
        manual_new_n = new_n;
        manual_time = ms;
        cerr << "manual: merges=" << manual_merges << ", new_n=" << manual_new_n << ", time_ms=" << manual_time << "\n";

        auto full = manual_coarsen_full(adj, 10000);
        adj_manual_final = move(full.first);
        manual_old2new = move(full.second);
        write_edgelist(outdir + "/manual_coarsened.edgelist", adj_manual_final);
        write_mapping(outdir + "/manual_old2new.txt", manual_old2new);
        cerr << "Wrote manual coarsened graph and mapping to results/\n";
    }

    int algo_new_n = orig_n;
    long long algo_time = 0;
    vvi adj_algo_final;
    vector<int> algo_old2new;
    {
        auto t0 = high_resolution_clock::now();
        auto res = algo_coarsen_full(adj,
                                     /*ef5=*/ef_threshold,
                                     /*ef4=*/ef_threshold,
                                     /*ef3=*/ef_threshold,
                                     /*ef2=*/ef_threshold,
                                     /*max_motifs_per_size=*/1000,
                                     /*merge_leaves_first=*/true,
                                     /*max_total_motifs_to_apply=*/2000);
        adj_algo_final = move(res.first);
        algo_old2new = move(res.second);
        algo_new_n = (int)adj_algo_final.size();
        auto t1 = high_resolution_clock::now();
        algo_time = duration_cast<milliseconds>(t1 - t0).count();
        cerr << "algo_coarsen_full: final_n=" << algo_new_n << ", time_ms=" << algo_time << "\n";

        write_edgelist(outdir + "/algo_coarsened.edgelist", adj_algo_final);
        write_mapping(outdir + "/algo_old2new.txt", algo_old2new);
        cerr << "Wrote algo coarsened graph and mapping to results/\n";
    }

    cout << "algorithm,orig_nodes,coarsened_nodes,compression_ratio,time_ms\n";
    cout << "manual," << orig_n << "," << manual_new_n << "," << fixed << setprecision(3) << (double)manual_new_n / orig_n << "," << manual_time << "\n";
    cout << "algo," << orig_n << "," << algo_new_n << "," << fixed << setprecision(3) << (double)algo_new_n / orig_n << "," << algo_time << "\n";

    string outpath = outdir + "/compare_results.csv";
    ofstream fout(outpath);
    if (fout) {
        fout << "algorithm,orig_nodes,coarsened_nodes,compression_ratio,time_ms\n";
        fout << "manual," << orig_n << "," << manual_new_n << "," << fixed << setprecision(3) << (double)manual_new_n / orig_n << "," << manual_time << "\n";
        fout << "algo," << orig_n << "," << algo_new_n << "," << fixed << setprecision(3) << (double)algo_new_n / orig_n << "," << algo_time << "\n";
        fout.close();
        cerr << "Wrote results to " << outpath << "\n";
    } else {
        cerr << "Failed to write results to " << outpath << "\n";
    }

    return 0;
}
