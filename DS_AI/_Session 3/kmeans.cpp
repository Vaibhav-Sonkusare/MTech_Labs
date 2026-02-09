#include <bits/stdc++.h>
using namespace std;


double euclidean(const vector<double>& a, const vector<double>& b) {
    double sum = 0.0;
    for (int i = 0; i < a.size(); i++)
        sum += (a[i] - b[i]) * (a[i] - b[i]);
    return sqrt(sum);
}

double manhattan(const vector<double>& a, const vector<double>& b) {
    double sum = 0.0;
    for (int i = 0; i < a.size(); i++)
        sum += abs(a[i] - b[i]);
    return sum;
}


vector<vector<double>> read_csv(const string& filename) {
    vector<vector<double>> data;
    ifstream file(filename);
    string line;

    getline(file, line);

    while (getline(file, line)) {
        stringstream ss(line);
        string value;
        vector<double> row;

        int col = 0;
        while (getline(ss, value, ',')) {
            if (col >= 1 && col <= 4) {
                row.push_back(stod(value));
            }
            col++;
        }
        data.push_back(row);
    }
    return data;
}


vector<int> kmeans(
    const vector<vector<double>>& data,
    int k,
    bool use_manhattan,
    int max_iters = 100
) {
    int n = data.size();
    int d = data[0].size();

    vector<vector<double>> centroids(k);
    for (int i = 0; i < k; i++)
        centroids[i] = data[i];

    vector<int> labels(n);

    for (int iter = 0; iter < max_iters; iter++) {
        for (int i = 0; i < n; i++) {
            double min_dist = 1e18;
            for (int j = 0; j < k; j++) {
                double dist = use_manhattan
                              ? manhattan(data[i], centroids[j])
                              : euclidean(data[i], centroids[j]);
                if (dist < min_dist) {
                    min_dist = dist;
                    labels[i] = j;
                }
            }
        }

        vector<vector<double>> new_centroids(k, vector<double>(d, 0.0));
        vector<int> count(k, 0);

        for (int i = 0; i < n; i++) {
            count[labels[i]]++;
            for (int j = 0; j < d; j++)
                new_centroids[labels[i]][j] += data[i][j];
        }

        for (int i = 0; i < k; i++)
            for (int j = 0; j < d; j++)
                new_centroids[i][j] /= count[i];

        centroids = new_centroids;
    }

    return labels;
}

double silhouette_score(
    const vector<vector<double>>& data,
    const vector<int>& labels,
    bool use_manhattan
) {
    int n = data.size();
    double total = 0.0;

    for (int i = 0; i < n; i++) {
        double a = 0.0;
        int same_count = 0;

        unordered_map<int, pair<double,int>> cluster_dist;

        for (int j = 0; j < n; j++) {
            if (i == j) continue;

            double dist = use_manhattan
                          ? manhattan(data[i], data[j])
                          : euclidean(data[i], data[j]);

            if (labels[i] == labels[j]) {
                a += dist;
                same_count++;
            } else {
                cluster_dist[labels[j]].first += dist;
                cluster_dist[labels[j]].second++;
            }
        }

        a /= same_count;

        double b = 1e18;
        for (auto& kv : cluster_dist) {
            double avg = kv.second.first / kv.second.second;
            b = min(b, avg);
        }

        total += (b - a) / max(a, b);
    }

    return total / n;
}


int main() {
    vector<vector<double>> data = read_csv("Iris.csv");

    int k = 3;

    auto labels_euc = kmeans(data, k, false);
    auto labels_man = kmeans(data, k, true);

    double sil_euc = silhouette_score(data, labels_euc, false);
    double sil_man = silhouette_score(data, labels_man, true);

    cout << "Silhouette Score (Euclidean): " << sil_euc << endl;
    cout << "Silhouette Score (Manhattan): " << sil_man << endl;

    return 0;
}