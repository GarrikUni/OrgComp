//g++ -o <arquivo saída> <arquivo entrada>; <-- compilar o exe

/*
LISTA DE OPCODES DO RISC-V (ref: https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/notebooks/RISCV/RISCV_CARD.pdf)
0110011 - Tipo R
0010011 - Tipo I
0000011 - Tipo I (para load commands)
0100011 - Tipo S
1100011 - Tipo B
1110011 - Tipo I (ecall/ebreak)

todos os commandos default terminam com o hex=3(b=0011), com exceção de:
jal   J 110 1111 (hex=f)
jalr  I 110 0111 (hex=7)
lui   U 011 0111 (hex=7)
auipc U 001 0111 (hex=7)
*/

#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <algorithm>
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
    if (anterior.opcode == "0000011") {
        // E a atual usa o registrador carregado (rd == rs1 ou rs2)
        return (atual.rs1 == anterior.rd || atual.rs2 == anterior.rd);
    }
    return false;
}


int main() {
    cout << "Comeco\n\n";
    string hexLinha;
    Operacao operacaoAtual;

    ifstream arquivo_lido("ex01_hex_dump.txt"); // Arquivo Hex dump 

    Operacao opAnterior; // Forwading preveni conflito RAW, e WAW após 1 linha, então só checaremos a instrução anterior, para WAW

    // Use a while loop together with the getline() function to read the file line by line
    while ( getline(arquivo_lido, hexLinha) ) {

        hexLinha.erase( find_if( hexLinha.rbegin(), hexLinha.rend(), []( unsigned char ch ) { // Limpa a linha de hexLinha para retirar inicio e quebra de linha
            return ch != '\n' && ch != '\r';
        } ).base(), hexLinha.end() );

        operacaoAtual = decodeInstruction(hexLinha);

        if ( hasWAWHazard( operacaoAtual, opAnterior ) )  {
            cout << "CONFLITO WAW" << endl;
        }
        if (hasLoadUseHazard(operacaoAtual, opAnterior )) {
            cout << "CONFLITO RAW (load-use)" << endl;
        }
        
        cout << operacaoAtual.rd << endl;


        opAnterior = operacaoAtual;
    }

    // Close the file
    arquivo_lido.close();

    cout << "\nFim\n";
}