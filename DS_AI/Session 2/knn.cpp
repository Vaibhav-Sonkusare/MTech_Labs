#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

using namespace std;

/* ---------------- Data Structures ---------------- */

struct DataPoint {
    vector<double> features;
    int label;
};

struct Prediction {
    int actual;
    double prob;   // probability of class 1
};

/* ---------------- Utility Functions ---------------- */

vector<DataPoint> loadCSV(const string& filename) {
    vector<DataPoint> data;
    ifstream file(filename);
    string line;

    getline(file, line); // skip header

    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        DataPoint p;

        getline(ss, token, ',');
        p.features.push_back(stod(token));

        getline(ss, token, ',');
        p.features.push_back(stod(token));

        getline(ss, token, ',');
        p.label = stoi(token);

        data.push_back(p);
    }
    return data;
}

double euclideanDistance(const vector<double>& a, const vector<double>& b) {
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); i++)
        sum += pow(a[i] - b[i], 2);
    return sqrt(sum);
}

/* ---------------- KNN Classifier ---------------- */

class KNNClassifier {
private:
    int k;
    vector<DataPoint> trainData;

public:
    KNNClassifier(int k) : k(k) {}

    void fit(const vector<DataPoint>& data) {
        trainData = data;
    }

    double predictProbability(const DataPoint& test) {
        vector<pair<double, int>> distances;

        for (const auto& p : trainData) {
            double d = euclideanDistance(test.features, p.features);
            distances.push_back({d, p.label});
        }

        sort(distances.begin(), distances.end());

        int count1 = 0;
        for (int i = 0; i < k; i++)
            if (distances[i].second == 1)
                count1++;

        return static_cast<double>(count1) / k;
    }

    int predict(const DataPoint& test) {
        return predictProbability(test) >= 0.5 ? 1 : 0;
    }
};

/* ---------------- Metrics ---------------- */

void computeMetrics(const vector<Prediction>& preds) {
    int TP = 0, TN = 0, FP = 0, FN = 0;

    for (auto& p : preds) {
        int pred = p.prob >= 0.5 ? 1 : 0;

        if (pred == 1 && p.actual == 1) TP++;
        if (pred == 0 && p.actual == 0) TN++;
        if (pred == 1 && p.actual == 0) FP++;
        if (pred == 0 && p.actual == 1) FN++;
    }

    double accuracy = (TP + TN) / double(TP + TN + FP + FN);
    double precision = TP / double(TP + FP + 1e-9);
    double recall = TP / double(TP + FN + 1e-9);
    double f1 = 2 * precision * recall / (precision + recall + 1e-9);

    cout << "\nPerformance Metrics\n";
    cout << "-------------------\n";
    cout << "Accuracy  : " << accuracy << endl;
    cout << "Precision : " << precision << endl;
    cout << "Recall    : " << recall << endl;
    cout << "F1 Score  : " << f1 << endl;
}

/* ---------------- Main ---------------- */

int main() {
    vector<DataPoint> data = loadCSV("Naive-Bayes-Classification-Data.csv");

    // Simple 70-30 split
    int split = data.size() * 0.7;
    vector<DataPoint> train(data.begin(), data.begin() + split);
    vector<DataPoint> test(data.begin() + split, data.end());

    KNNClassifier knn(3);
    knn.fit(train);

    vector<Prediction> predictions;

    for (auto& t : test) {
        double prob = knn.predictProbability(t);
        predictions.push_back({t.label, prob});
    }

    computeMetrics(predictions);

    return 0;
}

