#include <bits/stdc++.h>
using namespace std;

using vi = vector<int>;
using vvi = vector<vi>;

static double pair_external_fraction(const vvi &adj, int u, int v) {
    if (u < 0 || v < 0 || u >= (int)adj.size() || v >= (int)adj.size()) return 1.0;
    unordered_set<int> S;
    S.insert(u); S.insert(v);
    int ext = 0, total = 0;
    for (int x : adj[u]) { total++; if (!S.count(x)) ext++; }
    for (int x : adj[v]) { total++; if (!S.count(x)) ext++; }
    if (total == 0) return 0.0;
    return double(ext) / double(total);
}

static pair<vvi, vector<int>> apply_pairs_and_rebuild(const vvi &adj, const vector<pair<int,int>> &pairs_selected) {
    int n = adj.size();
    vector<int> new_id(n, -1);
    vector<char> used(n, 0);
    int cur = 0;
    for (auto p : pairs_selected) {
        int a = p.first, b = p.second;
        if (a<0 || b<0 || a>=n || b>=n) continue;
        if (used[a] || used[b]) continue;
        new_id[a] = new_id[b] = cur++;
        used[a] = used[b] = 1;
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

tuple<int,int,long long> manual_coarsen(const vvi &adj_orig, int max_pairs) {
    using namespace std::chrono;
    auto t0 = high_resolution_clock::now();

    vvi adj = adj_orig;
    int n = adj.size();
    int applied = 0;

    // Step 1: merge leaves greedily
    {
        vector<char> used(n, 0);
        vector<pair<int,int>> pairs;
        for (int u=0; u<n; ++u) {
            if (used[u]) continue;
            if (adj[u].size() == 1) {
                int v = adj[u][0];
                if (!used[v]) {
                    pairs.emplace_back(u, v);
                    used[u] = used[v] = 1;
                    if ((int)pairs.size() >= max_pairs) break;
                }
            }
        }
        if (!pairs.empty()) {
            auto res = apply_pairs_and_rebuild(adj, pairs);
            adj = move(res.first);
            n = adj.size();
            applied += (int)pairs.size();
        }
    }

    // Step 2: greedy common-neighbor merges
    int attempts = 0;
    while (applied < max_pairs && attempts < 1000) {
        ++attempts;
        int ncur = adj.size();
        vector<int> best_pair = {-1,-1};
        int best_common = -1;
        for (int u=0; u<ncur; ++u) {
            for (int v : adj[u]) {
                if (v <= u) continue;
                int common = 0;
                auto it1 = adj[u].begin();
                auto it2 = adj[v].begin();
                while (it1 != adj[u].end() && it2 != adj[v].end()) {
                    if (*it1 == *it2) { ++common; ++it1; ++it2; }
                    else if (*it1 < *it2) ++it1;
                    else ++it2;
                }
                if (common > best_common) {
                    best_common = common;
                    best_pair = {u, v};
                }
            }
        }
        if (best_common < 0) break;
        double ef = pair_external_fraction(adj, best_pair[0], best_pair[1]);
        if (ef <= 0.6) {
            vector<pair<int,int>> tmp = { {best_pair[0], best_pair[1]} };
            auto res = apply_pairs_and_rebuild(adj, tmp);
            adj = move(res.first);
            n = adj.size();
            applied += 1;
        } else {
            break;
        }
    }

    auto t1 = high_resolution_clock::now();
    long long ms = duration_cast<milliseconds>(t1 - t0).count();
    return {applied, (int)adj.size(), ms};
}

// manual_coarsen_full: returns final adj and a best-effort old->new mapping
pair<vvi, vector<int>> manual_coarsen_full(const vvi &adj_orig, int max_pairs) {
    vvi adj = adj_orig;
    int n = adj.size();

    // Step 1: merge leaves greedily
    vector<char> used(n, 0);
    vector<pair<int,int>> pairs;
    for (int u=0; u<n; ++u) {
        if (used[u]) continue;
        if (adj[u].size() == 1) {
            int v = adj[u][0];
            if (!used[v]) {
                pairs.emplace_back(u, v);
                used[u] = used[v] = 1;
                if ((int)pairs.size() >= max_pairs) break;
            }
        }
    }
    if (!pairs.empty()) {
        auto res = apply_pairs_and_rebuild(adj, pairs);
        adj = move(res.first);
        n = adj.size();
    }

    // Step 2: greedy common-neighbor merges
    int applied = 0;
    int attempts = 0;
    while (applied < max_pairs && attempts < 1000) {
        ++attempts;
        int ncur = adj.size();
        vector<int> best_pair = {-1,-1};
        int best_common = -1;
        for (int u=0; u<ncur; ++u) {
            for (int v : adj[u]) {
                if (v <= u) continue;
                int common = 0;
                auto it1 = adj[u].begin();
                auto it2 = adj[v].begin();
                while (it1 != adj[u].end() && it2 != adj[v].end()) {
                    if (*it1 == *it2) { ++common; ++it1; ++it2; }
                    else if (*it1 < *it2) ++it1;
                    else ++it2;
                }
                if (common > best_common) {
                    best_common = common;
                    best_pair = {u, v};
                }
            }
        }
        if (best_common < 0) break;
        double ef = pair_external_fraction(adj, best_pair[0], best_pair[1]);
        if (ef <= 0.6) {
            vector<pair<int,int>> tmp = { {best_pair[0], best_pair[1]} };
            auto res = apply_pairs_and_rebuild(adj, tmp);
            adj = move(res.first);
            applied += 1;
        } else {
            break;
        }
    }

    // Build best-effort old2new mapping (identity-like placeholder)
    vector<int> old2new(adj_orig.size());
    for (size_t i=0;i<old2new.size();++i) old2new[i] = (int)min((size_t)adj.size()-1, i);

    return {adj, old2new};
}
