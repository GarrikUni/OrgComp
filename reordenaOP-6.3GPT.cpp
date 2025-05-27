// g++ -o reordena reordenaOP-6.3.cpp -std=c++14

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <tuple>
#include <bitset>
#include <algorithm>
using namespace std;

struct Operacao {
    string opcode;
    string rd;
    string rs1;
    string rs2;
};

string hexToBinary(const string& hex) {
    string bin;
    for (char c : hex) {
        int val;
        if (c >= '0' && c <= '9') val = c - '0';
        else if (c >= 'a' && c <= 'f') val = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') val = 10 + (c - 'A');
        else continue;
        bin += bitset<4>(val).to_string();
    }
    return bin;
}

Operacao decodeInstruction(const string& hex) {
    string bin = hexToBinary(hex);
    Operacao op;
    op.opcode = bin.substr(25, 7);
    if (op.opcode == "0110011") { // tipo R
        op.rd = bin.substr(20, 5);
        op.rs1 = bin.substr(12, 5);
        op.rs2 = bin.substr(7, 5);
    } else if (op.opcode == "0010011" || op.opcode == "0000011") { // tipo I (inclui addi, lw)
        op.rd = bin.substr(20, 5);
        op.rs1 = bin.substr(12, 5);
        op.rs2 = "00000";
    } else if (op.opcode == "0100011") { // tipo S (sw)
        op.rd = "00000";
        op.rs1 = bin.substr(12, 5);
        op.rs2 = bin.substr(7, 5);
    } else {
        op.rd = op.rs1 = op.rs2 = "00000";
    }
    return op;
}

int hasDataHazard(const Operacao& atual, const Operacao& ant1, const Operacao& ant2) {
    if (ant1.rd != "00000" && (atual.rs1 == ant1.rd || atual.rs2 == ant1.rd)) {
        return 2;
    }
    if (ant2.rd != "00000" && (atual.rs1 == ant2.rd || atual.rs2 == ant2.rd)) {
        return 1;
    }
    return 0;
}

bool podeExecutar(const Operacao& op, const Operacao& ant1, const Operacao& ant2) {
    return hasDataHazard(op, ant1, ant2) == 0;
}

int main() {
    ifstream entrada("./Hexes/hex_teste_reordenar.txt");
    string linha;
    vector<tuple<string, Operacao>> instrucoes;

    while (getline(entrada, linha)) {
        linha.erase(remove(linha.begin(), linha.end(), '\r'), linha.end());
        linha.erase(remove(linha.begin(), linha.end(), '\n'), linha.end());
        if (!linha.empty()) {
            Operacao op = decodeInstruction(linha);
            instrucoes.push_back(make_tuple(linha, op));
        }
    }
    entrada.close();

    vector<string> instrucoesComNOPs;
    Operacao hist[2] = { Operacao(), Operacao() };
    vector<tuple<string, Operacao>> fila;

    for (size_t i = 0; i < instrucoes.size(); ++i) {
        string hex = get<0>(instrucoes[i]);
        Operacao op = get<1>(instrucoes[i]);

        int nops = hasDataHazard(op, hist[0], hist[1]);

        // Antes de inserir NOPs, tenta reordenar com a fila
        while (nops > 0) {
            bool reordenouAlguma = false;
            for (size_t j = 0; j < fila.size(); ) {
                Operacao candidato = get<1>(fila[j]);
                if (podeExecutar(candidato, hist[0], hist[1])) {
                    instrucoesComNOPs.push_back(get<0>(fila[j]));
                    hist[1] = hist[0];
                    hist[0] = candidato;
                    fila.erase(fila.begin() + j);
                    reordenouAlguma = true;
                    nops--; // você está ocupando 1 ciclo
                } else {
                    ++j;
                }
                if (nops == 0) break;
            }
            if (!reordenouAlguma) {
                instrucoesComNOPs.push_back("00000013");
                hist[1] = hist[0];
                hist[0] = Operacao(); // NOP
                nops--;
            }
        }


        if (podeExecutar(op, hist[0], hist[1])) {
            instrucoesComNOPs.push_back(hex);
            hist[1] = hist[0];
            hist[0] = op;
        } else {
            fila.push_back(make_tuple(hex, op));
        }
    }

    // Finaliza com o que restou na fila
    for (size_t i = 0; i < fila.size(); ++i) {
        string hex = get<0>(fila[i]);
        Operacao op = get<1>(fila[i]);

        int nops = hasDataHazard(op, hist[0], hist[1]);
        for (int j = 0; j < nops; ++j) {
            instrucoesComNOPs.push_back("00000013");
            hist[1] = hist[0];
            hist[0] = Operacao();
        }

        instrucoesComNOPs.push_back(hex);
        hist[1] = hist[0];
        hist[0] = op;
    }

    for (size_t i = 0; i < instrucoesComNOPs.size(); ++i) {
        cout << instrucoesComNOPs[i] << endl;
    }

    ofstream saida("./Hexes/saida_reordenada.txt");
    for (size_t i = 0; i < instrucoesComNOPs.size(); ++i) {
        saida << instrucoesComNOPs[i] << endl;
    }
    saida.close();

    return 0;
}
