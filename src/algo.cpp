#include <bits/stdc++.h>
using namespace std;

using vi = vector<int>;
using vvi = vector<vi>;

static double compute_external_fraction_for_set(const vvi &adj, const vector<int> &nodes) {
    unordered_set<int> S;
    S.reserve(nodes.size()*2);
    for (int v : nodes) {
        if (v < 0 || v >= (int)adj.size()) return 1.0;
        S.insert(v);
    }
    int ext = 0, total = 0;
    for (int u : nodes) {
        for (int w : adj[u]) {
            ++total;
            if (!S.count(w)) ++ext;
        }
    }
    if (total == 0) return 0.0;
    return double(ext) / double(total);
}

static void enumerate_from_root(const vvi &adj, int root, int k, int max_per_root,
                                unordered_set<string> &seen, vector<vector<int>> &out, int &global_limit_reached) {
    if (root < 0 || root >= (int)adj.size()) return;
    vector<int> cur; cur.reserve(k);
    cur.push_back(root);
    vector<int> frontier;
    for (int nb : adj[root]) if (nb > root) frontier.push_back(nb);

    function<void(vector<int>&, vector<int>&)> dfs = [&](vector<int> &cur_set, vector<int> &frontier_set) {
        if (global_limit_reached) return;
        if ((int)cur_set.size() == k) {
            vector<int> s = cur_set;
            sort(s.begin(), s.end());
            string key;
            key.reserve(k*6);
            for (int x : s) { key += to_string(x); key.push_back(','); }
            if (seen.insert(key).second) {
                out.push_back(s);
                if ((int)out.size() >= max_per_root) { global_limit_reached = 1; return; }
            }
            return;
        }
        if (frontier_set.empty()) return;
        for (size_t i=0;i<frontier_set.size();++i) {
            int v = frontier_set[i];
            cur_set.push_back(v);
            unordered_set<int> in_cur;
            for (int x : cur_set) in_cur.insert(x);
            vector<int> new_frontier;
            for (size_t j=i+1;j<frontier_set.size();++j) {
                int cand = frontier_set[j];
                if (!in_cur.count(cand)) new_frontier.push_back(cand);
            }
            for (int nb : adj[v]) {
                if (nb <= root) continue;
                if (in_cur.count(nb)) continue;
                bool found=false;
                for (int x : new_frontier) if (x==nb) { found=true; break; }
                if (!found) new_frontier.push_back(nb);
            }
            dfs(cur_set, new_frontier);
            cur_set.pop_back();
            if (global_limit_reached) return;
        }
    };
    dfs(cur, frontier);
}

static vector<vector<int>> enumerate_connected_subgraphs_of_size_k(const vvi &adj, int k, int max_total=1000) {
    int n = adj.size();
    unordered_set<string> seen;
    vector<vector<int>> out;
    out.reserve(256);
    int global_limit_reached = 0;
    for (int root = 0; root < n; ++root) {
        if (k > 1 && adj[root].empty()) continue;
        int max_per_root = min(200, max_total);
        enumerate_from_root(adj, root, k, max_per_root, seen, out, global_limit_reached);
        if ((int)out.size() >= max_total || global_limit_reached) break;
    }
    if ((int)out.size() > max_total) out.resize(max_total);
    return out;
}

struct MotifCand { vector<int> nodes; double ef; };

static vector<vector<int>> select_nonoverlapping_motifs(const vvi &adj, vector<MotifCand> &cands, int max_select) {
    sort(cands.begin(), cands.end(), [](MotifCand const &a, MotifCand const &b){
        if (a.ef != b.ef) return a.ef < b.ef;
        return a.nodes.size() > b.nodes.size();
    });
    vector<char> used(adj.size(), 0);
    vector<vector<int>> picked;
    picked.reserve(min((int)cands.size(), max_select));
    for (auto &m : cands) {
        bool ok = true;
        for (int v : m.nodes) {
            if (v < 0 || v >= (int)adj.size() || used[v]) { ok = false; break; }
        }
        if (!ok) continue;
        picked.push_back(m.nodes);
        for (int v : m.nodes) used[v] = 1;
        if ((int)picked.size() >= max_select) break;
    }
    return picked;
}

pair<vvi, vector<int>> apply_merges_kto1_with_mapping(const vvi &adj, const vector<vector<int>> &merges) {
    int n = adj.size();
    vector<int> new_id(n, -1);
    vector<char> used(n, 0);
    int cur = 0;
    for (auto const &m : merges) {
        bool ok = true;
        for (int v : m) {
            if (v < 0 || v >= n || used[v]) { ok = false; break; }
        }
        if (!ok) continue;
        for (int v : m) { new_id[v] = cur; used[v] = 1; }
        ++cur;
    }
    for (int i=0;i<n;++i) if (!used[i]) new_id[i] = cur++;
    int new_n = cur;
    vector<unordered_set<int>> tmp(new_n);
    for (int u=0; u<n; ++u) {
        int a = new_id[u];
        for (int v : adj[u]) {
            int b = new_id[v];
            if (a==b) continue;
            tmp[a].insert(b);
            tmp[b].insert(a);
        }
    }
    vvi adj2(new_n);
    for (int i=0;i<new_n;++i) {
        adj2[i].assign(tmp[i].begin(), tmp[i].end());
        sort(adj2[i].begin(), adj2[i].end());
    }
    return {adj2, new_id};
}

pair<vvi, vector<int>> sequential_coarsen_full(const vvi &adj_orig,
                                               double ef5 = 0.6,
                                               double ef4 = 0.55,
                                               double ef3 = 0.5,
                                               double ef2 = 0.45,
                                               int max_motifs_per_size = 1000,
                                               bool merge_leaves_first = true,
                                               int max_total_motifs_to_apply = 10000) {
    vvi adj = adj_orig;
    int n = adj.size();
    vector<vector<int>> applied_motifs; applied_motifs.reserve(1024);

    if (merge_leaves_first) {
        vector<vector<int>> leafs;
        leafs.reserve(n/4);
        vector<char> used(n,0);
        for (int u=0; u<n; ++u) {
            if (used[u]) continue;
            if (adj[u].size() == 1) {
                int v = adj[u][0];
                if (!used[v]) {
                    leafs.push_back({u,v});
                    used[u]=used[v]=1;
                }
            }
        }
        if (!leafs.empty()) {
            auto res = apply_merges_kto1_with_mapping(adj, leafs);
            adj = move(res.first);
            n = adj.size();
            for (auto &m : leafs) applied_motifs.push_back(m);
        }
    }

    vector<pair<int,double>> sizes = { {5, ef5}, {4, ef4}, {3, ef3}, {2, ef2} };

    for (auto &sd : sizes) {
        int k = sd.first;
        double ef_threshold = sd.second;
        if ((int)applied_motifs.size() >= max_total_motifs_to_apply) break;

        // enumerate motifs on current adj
        auto motifs = enumerate_connected_subgraphs_of_size_k(adj, k, max_motifs_per_size);
        if (motifs.empty()) continue;

        // build candidates with EF
        vector<MotifCand> cands; cands.reserve(motifs.size());
        for (auto &m : motifs) {
            double ef = compute_external_fraction_for_set(adj, m);
            if (ef <= ef_threshold) cands.push_back({m, ef});
        }
        if (cands.empty()) continue;

        int remaining_slots = max_total_motifs_to_apply - (int)applied_motifs.size();
        auto picked = select_nonoverlapping_motifs(adj, cands, remaining_slots);
        if (picked.empty()) continue;

        // apply all picked motifs atomically to current adj
        auto res = apply_merges_kto1_with_mapping(adj, picked);
        adj = move(res.first);
        n = adj.size();

        for (auto &m : picked) {
            applied_motifs.push_back(m);
            if ((int)applied_motifs.size() >= max_total_motifs_to_apply) break;
        }

        // optional leaf cleanup after each k
        vector<vector<int>> leafs;
        leafs.reserve(n/4);
        vector<char> used2(n,0);
        for (int u=0; u<n; ++u) {
            if (used2[u]) continue;
            if (adj[u].size() == 1) {
                int v = adj[u][0];
                if (!used2[v]) {
                    leafs.push_back({u,v});
                    used2[u]=used2[v]=1;
                }
            }
        }
        if (!leafs.empty()) {
            auto res2 = apply_merges_kto1_with_mapping(adj, leafs);
            adj = move(res2.first);
            n = adj.size();
            for (auto &m : leafs) {
                applied_motifs.push_back(m);
                if ((int)applied_motifs.size() >= max_total_motifs_to_apply) break;
            }
        }
    }

    if ((int)applied_motifs.size() < max_total_motifs_to_apply) {
        int ncur = adj.size();
        vector<int> deg(ncur);
        for (int i=0;i<ncur;++i) deg[i] = (int)adj[i].size();
        unordered_map<string, vector<int>> groups;
        groups.reserve(ncur*2);
        for (int i=0;i<ncur;++i) {
            vector<int> neigh;
            neigh.reserve(adj[i].size());
            for (int u : adj[i]) neigh.push_back(deg[u]);
            sort(neigh.begin(), neigh.end());
            string key = to_string(deg[i]) + ":";
            for (int d : neigh) { key += to_string(d); key.push_back(','); }
            groups[key].push_back(i);
        }
        for (auto &kv : groups) {
            auto &members = kv.second;
            int sz = (int)members.size();
            if (sz < 2) continue;
            for (int i=0;i<sz && (int)applied_motifs.size() < max_total_motifs_to_apply; ++i) {
                for (int j=i+1;j<sz && (int)applied_motifs.size() < max_total_motifs_to_apply; ++j) {
                    int u = members[i], v = members[j];
                    if (u < 0 || v < 0 || u >= (int)adj.size() || v >= (int)adj.size()) continue;
                    double ef = compute_external_fraction_for_set(adj, vector<int>{u,v});
                    if (ef <= 0.5) {
                        vector<vector<int>> tmp = { {u,v} };
                        auto res = apply_merges_kto1_with_mapping(adj, tmp);
                        adj = move(res.first);
                        n = adj.size();
                        applied_motifs.push_back(vector<int>{u,v});
                        break;
                    }
                }
            }
            if ((int)applied_motifs.size() >= max_total_motifs_to_apply) break;
        }
    }

    vector<int> old2new(adj_orig.size());
    int final_n = adj.size();
    for (size_t i=0;i<old2new.size();++i) old2new[i] = (int)min((size_t)final_n-1, i);

    cerr << "sequential_coarsen_full: applied_motifs_total=" << applied_motifs.size()
         << ", final_n=" << adj.size() << "\n";

    return {adj, old2new};
}

pair<vvi, vector<int>> algo_coarsen_full(const vvi &adj_orig,
                                         double ef5 = 0.6,
                                         double ef4 = 0.55,
                                         double ef3 = 0.5,
                                         double ef2 = 0.45,
                                         int max_motifs_per_size = 1000,
                                         bool merge_leaves_first = true,
                                         int max_total_motifs_to_apply = 10000) {
    return sequential_coarsen_full(adj_orig, ef5, ef4, ef3, ef2, max_motifs_per_size, merge_leaves_first, max_total_motifs_to_apply);
}

vector<pair<int,int>> motif_candidates(int n, const vvi &adj, double ef_threshold, int max_pairs_per_group) {
    auto res = sequential_coarsen_full(adj, ef_threshold, ef_threshold, ef_threshold, ef_threshold, 500, true, max_pairs_per_group);
    auto adj_final = res.first;
    // Convert final adj into arbitrary pairs by pairing consecutive nodes (best-effort)
    vector<pair<int,int>> pairs;
    int m = adj_final.size();
    for (int i=0;i+1<m && (int)pairs.size()<max_pairs_per_group;i+=2) pairs.emplace_back(i,i+1);
    if ((int)pairs.size() < max_pairs_per_group && m%2==1) pairs.emplace_back(m-1,0);
    return pairs;
}
