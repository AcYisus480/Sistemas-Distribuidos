#include "../../lib/LindaDriver/LindaDriver.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unordered_set>
#include <vector>

using namespace std;

// --- Cargar diccionario ---
unordered_set<string> loadDictionary(const string &path) {
    unordered_set<string> dict;
    ifstream f(path);
    if (!f.is_open()) {
        cerr << "No se pudo abrir el diccionario: " << path << endl;
        exit(1);
    }
    string word;
    while (getline(f, word)) {
        for (auto &c : word) c = toupper(c);
        dict.insert(word);
    }
    f.close();
    return dict;
}

// --- Puntuación ---
int letterScore(char c) {
    switch (toupper(c)) {
        case 'A': case 'E': case 'I': case 'L': case 'N': case 'O':
        case 'R': case 'S': case 'T': case 'U': return 1;
        case 'D': case 'G': return 2;
        case 'B': case 'C': case 'M': case 'P': return 3;
        case 'F': case 'H': case 'V': case 'Y': return 4;
        case 'Q': case 'J': case 'X': return 8;
        case 'Z': return 10;
        default: 
            if ((unsigned char)c == 0xD1) return 8;
            return 0;  
    }
}
int wordScore(const string &w) {
    int total = 0;
    for (char c : w) total += letterScore(c);
    return total;
}

// --- Permutaciones y búsqueda de mejor palabra ---
void generatePermutations(const string &letters, int t,
                          const unordered_set<string> &dict,
                          string &bestWord, int &bestScore) {
    string temp = letters;
    sort(temp.begin(), temp.end());
    unordered_set<string> checked;

    vector<int> mask(temp.size(), 0);
    fill(mask.begin(), mask.begin() + t, 1);

    do {
        string combo;
        for (size_t i = 0; i < temp.size(); ++i)
            if (mask[i]) combo.push_back(temp[i]);

        sort(combo.begin(), combo.end());
        do {
            if (checked.count(combo)) continue;
            checked.insert(combo);
            if (dict.count(combo)) {
                int s = wordScore(combo);
                if (s > bestScore) {
                    bestScore = s;
                    bestWord = combo;
                }
            }
        } while (next_permutation(combo.begin(), combo.end()));

    } while (prev_permutation(mask.begin(), mask.end()));
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cerr << "Uso: " << argv[0] << " <IP_LS> <Port_LS> <diccionario>" << endl;
        return 1;
    }

    string ip = argv[1];
    string port = argv[2];
    string dictPath = argv[3];

    LD LindaDriver(ip, port);
    auto dict = loadDictionary(dictPath);

    cout << "Worker conectado a " << ip << ":" << port << endl;

    while (true) {
        Tuple pattern("calcular", "?X", "?Y");
        Tuple task(pattern.size());
        task = LindaDriver.IN(pattern);

        string letters = task.get(2);
        int t = stoi(task.get(3));

        if (t == 0) {
            cout << "Worker: tarea de fin recibida -> saliendo." << endl;
            break;
        }

        cout << "Worker: procesando letras=" << letters << " t=" << t << endl;

        string bestWord = "-";
        int bestScore = 0;

        generatePermutations(letters, t, dict, bestWord, bestScore);

        Tuple result("score", bestWord, to_string(bestScore));
        LindaDriver.OUT(result);

        cout << "Worker: mejor palabra -> " << result.to_string() << endl;
    }

    LindaDriver.STOP();
    return 0;
}
