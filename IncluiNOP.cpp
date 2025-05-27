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
using namespace std;

struct Operacao { //criação default é nop(addi x0, x0, 0)
    string opcode = "0010011";
    string rd = "00000";
    string rs1 = "00000";
    string rs2;
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
        cout << "Data Hazard: 1 linha acima.\n";
        return 2; // precisa de 2 nops
    }

    if (anterior2.rd != "00000" && (atual.rs1 == anterior2.rd || atual.rs2 == anterior2.rd)) {
        cout << "Data Hazard: 2 linhas acima.\n";
        return 1; // precisa de 1 nop
    }

    return 0;
}

int main() {
    cout << "Comeco\n\n";
    string texto;
    Operacao operacao;

    ifstream arquivo_lido("teste_all_hazards.txt"); // Arquivo Hex dump 

    Operacao historico[3];

    int numNOPS;
    vector<string> incluiNOPS;

    // Use a while loop together with the getline() function to read the file line by line
    while ( getline(arquivo_lido, texto) ) {

        texto.erase( find_if( texto.rbegin(), texto.rend(), []( unsigned char ch ) { // Limpa a linha de texto para retirar inicio e quebra de linha
            return ch != '\n' && ch != '\r';
        } ).base(), texto.end() );

        operacao = decodeInstruction(texto);

        // Atualiza o histórico (desloca e insere a nova operação)
        historico[2] = historico[1];
        historico[1] = historico[0];
        historico[0] = operacao;

        numNOPS = hasDataHazard(operacao, historico[1], historico[2]);

        for ( int i=0; i < numNOPS ; i++ ) {
            incluiNOPS.push_back("00000013"); //inclui NOP em HEX
        }

        incluiNOPS.push_back(texto);
        
    }

    // Close the file
    arquivo_lido.close();

    for ( string linha : incluiNOPS ) {
        cout << linha << "\n";
    }

    cout << "\nFim\n";
}