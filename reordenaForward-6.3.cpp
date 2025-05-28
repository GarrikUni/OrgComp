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

// int hasDataHazard(const Operacao& atual, const Operacao& ant1, const Operacao& ant2) {
//     if (ant1.rd != "00000" && (atual.rs1 == ant1.rd || atual.rs2 == ant1.rd)) {
//         return 2;
//     }
//     if (ant2.rd != "00000" && (atual.rs1 == ant2.rd || atual.rs2 == ant2.rd)) {
//         return 1;
//     }
//     return 0;
// }

bool hasWAWHazard ( const Operacao& atual, const Operacao& anterior ) {
    // a instrução anterior sempre chega ao estágio de escrita (WB) antes da segunda, 
    // a não ser que a instrução atual seja uma instrução mais curta. 
    // Essa função garante marca um possível WAW Hazard para garantir o funcionamento correto das instruções
    if( atual.rd != "00000" && atual.rd == anterior.rd ) {
        return true;
    }

    return false;
}

bool hasLoadUseHazard(const Operacao& atual, const Operacao& anterior) {
    // Se a anterior for um load (opcode 0000011)
    if ( anterior.opcode == "0000011" && anterior.rd != "00000" ) {
        // E a atual usa o registrador carregado (rd == rs1 ou rs2)
        return (atual.rs1 == anterior.rd || atual.rs2 == anterior.rd);
    }
    return false;
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
    Operacao anterior = Operacao();
    vector<tuple<string, Operacao>> fila;

    for (size_t i = 0; i < instrucoes.size(); ++i) {
        string hex = get<0>(instrucoes[i]);
        Operacao op = get<1>(instrucoes[i]);

        // Antes de inserir NOPs, tenta reordenar com a fila
        if ( hasWAWHazard( op, anterior ) || hasLoadUseHazard(op, anterior ) ) {
            bool reordenado = false;
            for (size_t j = 0; j < fila.size(); ++j) {
                Operacao op_candidato = get<1>(fila[j]);
                if ( !hasWAWHazard( op_candidato, anterior ) && !hasLoadUseHazard(op_candidato, anterior ) ) {
                    instrucoesComNOPs.push_back(get<0>(fila[j]));
                    anterior = op_candidato;
                    fila.erase(fila.begin() + j);
                    reordenado = true;
                    break;
                }
            }
            if (!reordenado) {
                instrucoesComNOPs.push_back("00000013");
                anterior = Operacao();
            }
        }

        if ( hasWAWHazard( op, anterior ) || hasLoadUseHazard(op, anterior ) ) {
            fila.push_back(make_tuple(hex, op));
        } else {
            instrucoesComNOPs.push_back(hex);
            anterior = op;
        }
    }

    // Finaliza com o que restou na fila
    for (size_t i = 0; i < fila.size(); ++i) {
        string hex = get<0>(fila[i]);
        Operacao op = get<1>(fila[i]);

        if ( hasWAWHazard( op, anterior )|| hasLoadUseHazard(op, anterior ) ) {
            instrucoesComNOPs.push_back("00000013");
            anterior = Operacao();
        }

        instrucoesComNOPs.push_back(hex);
        anterior = op;
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
