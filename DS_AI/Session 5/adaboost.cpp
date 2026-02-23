#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

using namespace std;

struct Sample {
    double glucose;
    double bp;
    int label;   // 0 or 1
};

struct Stump {
    int feature;      // 0 = glucose, 1 = bp
    double threshold;
    int polarity;     // +1 or -1
    double alpha;
};

vector<Sample> dataset;
vector<double> weights;
vector<Stump> model;

// -----------------------------
// Load CSV
// -----------------------------
void loadData(string filename) {
    ifstream file(filename);
    string line;

    getline(file, line); // skip header

    while (getline(file, line)) {
        stringstream ss(line);
        string val;
        Sample s;

        getline(ss, val, ',');
        s.glucose = stod(val);

        getline(ss, val, ',');
        s.bp = stod(val);

        getline(ss, val, ',');
        s.label = stoi(val);

        dataset.push_back(s);
    }
}

// -----------------------------
// Decision stump prediction
// -----------------------------
int stumpPredict(const Stump &stump, const Sample &s) {
    double featureValue = (stump.feature == 0) ? s.glucose : s.bp;

    int pred = (featureValue < stump.threshold) ? 1 : -1;
    return stump.polarity * pred;
}

// -----------------------------
// Train one decision stump
// -----------------------------
Stump trainStump() {
    Stump bestStump;
    double minError = 1e9;

    for (int feature = 0; feature < 2; feature++) {
        for (auto &sample : dataset) {

            double threshold = (feature == 0) ?
                               sample.glucose : sample.bp;

            for (int polarity : {1, -1}) {

                double error = 0;

                for (int i = 0; i < dataset.size(); i++) {
                    Stump temp{feature, threshold, polarity, 0};
                    int pred = stumpPredict(temp, dataset[i]);

                    int actual = (dataset[i].label == 1) ? 1 : -1;

                    if (pred != actual)
                        error += weights[i];
                }

                if (error < minError) {
                    minError = error;
                    bestStump = {feature, threshold, polarity, 0};
                }
            }
        }
    }

    // Compute alpha
    double eps = 1e-10;
    bestStump.alpha = 0.5 * log((1 - minError + eps) / (minError + eps));

    return bestStump;
}

// -----------------------------
// Update weights
// -----------------------------
void updateWeights(const Stump &stump) {
    double sumW = 0;

    for (int i = 0; i < dataset.size(); i++) {
        int pred = stumpPredict(stump, dataset[i]);
        int actual = (dataset[i].label == 1) ? 1 : -1;

        weights[i] *= exp(-stump.alpha * actual * pred);
        sumW += weights[i];
    }

    // normalize
    for (auto &w : weights)
        w /= sumW;
}

// -----------------------------
// Final prediction
// -----------------------------
int predict(const Sample &s) {
    double total = 0;

    for (auto &stump : model) {
        total += stump.alpha * stumpPredict(stump, s);
    }

    return (total >= 0) ? 1 : 0;
}

// -----------------------------
// Main
// -----------------------------
int main() {

    loadData("Naive-Bayes-Classification-Data.csv");

    int N = dataset.size();
    weights.assign(N, 1.0 / N);

    int rounds = 5; // number of weak learners

    for (int i = 0; i < rounds; i++) {
        Stump stump = trainStump();
        model.push_back(stump);
        updateWeights(stump);

        cout << "Round " << i+1
             << " Alpha: " << stump.alpha << endl;
    }

    // Evaluate accuracy
    int correct = 0;
    for (auto &s : dataset) {
        if (predict(s) == s.label)
            correct++;
    }

    cout << "\nAccuracy: "
         << (double)correct / dataset.size() * 100
         << "%" << endl;

    return 0;
}
