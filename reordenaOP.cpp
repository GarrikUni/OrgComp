//g++ -o <arquivo saída> <arquivo entrada>; <-- compilar o exe

/*
LISTA DE OPCODES DO RISC-V (ref: https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/notebooks/RISCV/RISCV_CARD.pdf)
*/

#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <algorithm>
#include <vector>
#include <set>
using namespace std;

struct Operacao { //criação default é nop(addi x0, x0, 0)
    string opcode = "0010011";
    string rd = "00000";
    string rs1 = "00000";
    string rs2 = "00000";;
    string imm = "000000000000";
    string funct3 = "000";
    string funct7;
};

string hexToBinary(string hex) {
    string binario;

    for (char c : hex) {

        int value;
        if ( c>='0' && c<='9' ) value = c - '0'; // subtrai o valor ascii de 0 (48) para obter o valor do caracter, pois os caracteres numericos são sequenciais.
        else if ( c>='A' && c<='F' ) value = c - 'A' + 10;
        else if ( c>='a' && c<='f' ) value = c - 'a' + 10;
        else throw invalid_argument("Caractere hexadecimal inválido.");

        binario += bitset<4>(value).to_string(); // Converte o value obtido do hex para binário, e esse binário para string
    }
    
    return binario;
}

int binToInt(const string& bin) {
    return stoi(bin, nullptr, 2);
}

string getOpcode(string binario) {
    return binario.substr(25, 7);
}

Operacao decodeInstruction ( const string& hex ) {
    Operacao op;

    string binario = hexToBinary(hex);

    op.opcode = getOpcode(binario);

    // funct3 e 7, necassários para diferenciar algumas instruções
    op.funct3 = binario.substr(17, 3);  // funct3 (bits 12 - 14)
    op.funct7 = binario.substr(0, 7);   // funct7 (bits 25 - 31)

    if (op.opcode == "0110011") { // Tipo R
        op.rd = binario.substr(20, 5);  // rd (bits 7 - 11)
        op.rs1 = binario.substr(12, 5); // rs1 (bits 15 - 19)
        op.rs2 = binario.substr(7, 5);  // rs2 (bits 20 - 24)
        op.imm = "000000000000";  // Sem imm
    } 
    else if (op.opcode == "0010011") { // Tipo I
        op.rd = binario.substr(20, 5);    // rd (bits 7 - 11)
        op.rs1 = binario.substr(12, 5);   // rs1 (bits 15 - 19)
        op.imm = binario.substr(0, 12);   // imm (bits 0 - 11)
    } 
    else if (op.opcode == "0000011") { // Load instructions (Tipo I)
        op.rd = binario.substr(20, 5);    // rd (bits 7 - 11)
        op.rs1 = binario.substr(12, 5);   // rs1 (bits 15 - 19)
        op.imm = binario.substr(0, 12);   // imm (bits 0 - 11)
    } 
    else if (op.opcode == "0100011") { // Tipo S (Store instructions)
        op.rs1 = binario.substr(12, 5);  // rs1 (bits 15 - 19)
        op.rs2 = binario.substr(7, 5);   // rs2 (bits 20 - 24)
        op.imm = binario.substr(0, 7) + binario.substr(20, 5); // imm (combine imm[11:5] and imm[4:0])
    }
    else if (op.opcode == "1100011") { // Tipo B (Branch instructions)
        op.rs1 = binario.substr(12, 5);  // rs1 (bits 15 - 19)
        op.rs2 = binario.substr(7, 5);   // rs2 (bits 20 - 24)
        op.imm = binario[0] + binario.substr(24, 1) + binario.substr(1, 6) + binario.substr(20, 4) + "0"; // imm[4:1|11] 
    }
    else if (op.opcode == "1101111") { // Tipo J (Jump instructions)
        op.rd = binario.substr(20, 5);  // rd (bits 7 - 11)
        op.imm = binario[0] + binario.substr(12, 8) + binario[11] + binario.substr(1, 10) + "0";  // imm
    }
    else if (op.opcode == "0110111" || op.opcode == "0010111") { // Tipo U (lui, auipc)
        op.rd = binario.substr(20, 5);  // rd (bits 7 - 11)
        op.imm = binario.substr(0, 20); // imm (bits 12 - 31)
    }

    return op;
}

int hasDataHazard(const Operacao& atual, const Operacao& anterior1, const Operacao& anterior2) {
    // Ignora NOPs (rd = "00000")
    if (anterior1.rd != "00000" && (atual.rs1 == anterior1.rd || atual.rs2 == anterior1.rd)) {
        //cout << "Data Hazard: 1 linha acima.\n";
        return 2; // precisa de 2 nops
    }

    if (anterior2.rd != "00000" && (atual.rs1 == anterior2.rd || atual.rs2 == anterior2.rd)) {
        //cout << "Data Hazard: 2 linhas acima.\n";
        return 1; // precisa de 1 nop
    }

    return 0;
}

int main() {
    cout << "Comeco\n\n";
    string hexCode;
    Operacao op;

    ifstream arquivo_lido("ex01_hex_dump.txt"); // Arquivo Hex dump 

    vector<pair<string, Operacao>> instrucoes;

    while ( getline(arquivo_lido, hexCode) ) {

        hexCode.erase( find_if( hexCode.rbegin(), hexCode.rend(), []( unsigned char ch ) { // Limpa a linha de hexCode para retirar inicio e quebra de linha
            return ch != '\n' && ch != '\r';
        } ).base(), hexCode.end() );

        op = decodeInstruction(hexCode);
        instrucoes.push_back({hexCode, op});
        
    }

    vector<string> instrucoes_final;
    Operacao hist[3];
    set<string> registradores_prontos = {"00000"}; // x0 sempre pronto
    vector<pair<string, Operacao>> fila;

    for ( auto &[hex,operacao] : instrucoes ) {
        if( registradores_prontos.find(operacao.rs1) != registradores_prontos.end() 
        && registradores_prontos.find(operacao.rs2) != registradores_prontos.end() 
        && hasDataHazard(operacao, hist[1], hist[2]) == 0 ){
            instrucoes_final.push_back(hex); // Os registradores rs1 e rs2 utilizados na instrução já estão prontos, adiciona a operação para ser executada
            hist[2]=hist[1];
            hist[1]=hist[0];
            hist[0]=operacao;
            registradores_prontos.insert(operacao.rd);
        } else {
            fila.push_back({hex, operacao}); // Os registradores rs1 e rs2 utilizados na instrução NÃO estão prontos, adiciona a operação para ESPERAR
        }
    }


    bool progresso = true;

    while (progresso && !fila.empty()) { // fica tentando reordenar até qua não haja progresso
        progresso = false;
        for (size_t i = 0; i < fila.size(); ) {
            auto [hex, operacao] = fila[i];
            if ( registradores_prontos.find(operacao.rs1) != registradores_prontos.end() 
            && registradores_prontos.find(operacao.rs2) != registradores_prontos.end() 
            && hasDataHazard(operacao, hist[1], hist[2]) == 0 ) {
                instrucoes_final.push_back(hex); // Os registradores rs1 e rs2 utilizados na instrução já estão prontos, adiciona a operação para ser executada
                registradores_prontos.insert(operacao.rd);

                hist[2]=hist[1];
                hist[1]=hist[0];
                hist[0]=operacao;

                fila.erase(fila.begin() + i); // Retira item da fila de espera
                progresso = true;
            } else {
                // Só avança se não removeu
                ++i;
            }
        }
    }

    while (!fila.empty()) {
        for (size_t i = 0; i < fila.size(); ) {
            auto [hex, operacao] = fila[i];
    
            int numNOPS = hasDataHazard(operacao, hist[0], hist[1]);

            for (int j = 0; j < numNOPS; j++) {
                instrucoes_final.push_back("00000013");
                hist[2] = hist[1];
                hist[1] = hist[0];
                hist[0] = Operacao(); // NOP
            }
    
            // Agora sim, insere a instrução
            instrucoes_final.push_back(hex);
            registradores_prontos.insert(operacao.rd);
    
            hist[2] = hist[1];
            hist[1] = hist[0];
            hist[0] = operacao;
    
            fila.erase(fila.begin() + i);
        }
    }

    // Close the file
    arquivo_lido.close();

    for ( string hex : instrucoes_final ) {
        cout << hex << endl;
    }

    cout << "\nFim\n";
}