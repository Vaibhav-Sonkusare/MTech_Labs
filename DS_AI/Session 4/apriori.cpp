#include <bits/stdc++.h>
using namespace std;

typedef set<string> Itemset;

vector<Itemset> transactions;
map<Itemset, int> support_count;



bool is_subset(const Itemset &a, const Itemset &b)
{
    return includes(b.begin(), b.end(), a.begin(), a.end());
}

int count_support(const Itemset &item)
{
    int count = 0;
    for (auto &t : transactions)
        if (is_subset(item, t))
            count++;
    return count;
}


vector<Itemset> join_sets(const vector<Itemset> &Lk)
{
    vector<Itemset> Ck;
    for (int i = 0; i < Lk.size(); i++)
    {
        for (int j = i + 1; j < Lk.size(); j++)
        {
            Itemset temp = Lk[i];
            temp.insert(Lk[j].begin(), Lk[j].end());
            if (temp.size() == Lk[i].size() + 1)
                Ck.push_back(temp);
        }
    }
    return Ck;
}


vector<vector<Itemset>> apriori(double min_support)
{
    vector<vector<Itemset>> all_L;

    map<string, int> item_count;

    // C1 generation
    for (auto &t : transactions)
        for (auto &item : t)
            item_count[item]++;

    vector<Itemset> L1;

    for (auto &p : item_count)
    {
        double support = (double)p.second / transactions.size();
        if (support >= min_support)
        {
            Itemset temp = {p.first};
            L1.push_back(temp);
            support_count[temp] = p.second;
        }
    }

    all_L.push_back(L1);

    vector<Itemset> Lk = L1;

    while (!Lk.empty())
    {
        vector<Itemset> Ck = join_sets(Lk);
        vector<Itemset> Lk_next;

        for (auto &c : Ck)
        {
            int sup = count_support(c);
            double support = (double)sup / transactions.size();

            if (support >= min_support)
            {
                Lk_next.push_back(c);
                support_count[c] = sup;
            }
        }

        if (!Lk_next.empty())
            all_L.push_back(Lk_next);

        Lk = Lk_next;
    }

    return all_L;
}


void generate_rules(const vector<vector<Itemset>> &all_L, double min_confidence) {
    cout << "\nStrong Association Rules:\n";

    for (auto &level : all_L)
    {
        for (auto &itemset : level)
        {
            if (itemset.size() < 2)
                continue;

            vector<string> items(itemset.begin(), itemset.end());
            int n = items.size();

            for (int mask = 1; mask < (1 << n) - 1; mask++)
            {
                Itemset A, B;

                for (int i = 0; i < n; i++)
                {
                    if (mask & (1 << i))
                        A.insert(items[i]);
                    else
                        B.insert(items[i]);
                }

                double conf =
                    (double)support_count[itemset] /
                    support_count[A];

                if (conf >= min_confidence)
                {
                    cout << "{ ";
                    for (auto &x : A) cout << x << " ";
                    cout << "} -> { ";
                    for (auto &x : B) cout << x << " ";
                    cout << "}  confidence=" << conf << endl;
                }
            }
        }
    }
}



void load_dataset(string filename)
{
    ifstream file(filename);
    string line;

    while (getline(file, line))
    {
        stringstream ss(line);
        string item;
        Itemset transaction;

        while (getline(ss, item, ','))
            if (!item.empty())
                transaction.insert(item);

        if (!transaction.empty())
            transactions.push_back(transaction);
    }
}

int main()
{
    string filename = "Market_Basket_Optimisation.csv";

    load_dataset(filename);

    // cout << "Transactions loaded: " << transactions.size() << endl;

    double min_support, min_confidence;

    // cout << "Enter minimum support (0-1): ";
    // cin >> min_support;

    // cout << "Enter minimum confidence (0-1): ";
    // cin >> min_confidence;
    
    min_support = 0.05;
    min_confidence = 0.08;

    auto frequent_itemsets = apriori(min_support);

    cout << "\nFrequent Itemsets:\n";

    for (auto &level : frequent_itemsets)
    {
        for (auto &itemset : level)
        {
            cout << "{ ";
            for (auto &x : itemset)
                cout << x << ", ";
            cout << "} Support=" << (double)support_count[itemset] / transactions.size() << endl;
        }
    }

    generate_rules(frequent_itemsets, min_confidence);
}
