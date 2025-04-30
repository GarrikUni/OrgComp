#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <algorithm>
using namespace std;

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

struct Contador {
    int tipoR = 0;
    int tipoI = 0;
    int tipoS = 0;
    int tipoB = 0;
    int tipoU = 0;
    int tipoJ = 0;
};

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

int main() {

    Contador cont;

    cout << "Comeco\n\n";
    string texto;

    ifstream arquivo_lido("ex01_hex_dump.txt"); // Arquivo Hex dump 

    int rdHAZ[3] = {0,0,0};

    // Use a while loop together with the getline() function to read the file line by line
    while ( getline(arquivo_lido, texto) ) {
        // Verificar os caracteres 7(texto[6]) e 8(texto[7]), pois estes são os caracteres que informam o OPCODE

        rdHAZ[2] = rdHAZ[1]; // Avança um 
        rdHAZ[1] = rdHAZ[0];
        
        cout << hexToBinary(texto) << endl;
        
        string binario = hexToBinary(texto);

        if ( texto[7] == '3' ) { // Verifica se se é um dos tipos que termina com valor 3
            
            if ( texto[6] == '3' || texto[6] == 'b' ) { //verificar tipo R (0011 ou 1011)(3 ou b)

                int rs1 = binToInt(binario.substr(12, 5));
                int rs2 = binToInt(binario.substr(7, 5));

                if ( rdHAZ[1] != 0 && (rdHAZ[1] == rs1  || rdHAZ[1] == rs2) )
                    cout << "HAZARD!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 1Linha\n";
                if ( rdHAZ[2] != 0 && (rdHAZ[2] == rs1  || rdHAZ[2] == rs2) )
                    cout << "HAZARD!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 2Linhas\n";
                
                int rd = binToInt(binario.substr(20, 5));

                rdHAZ[0] = rd;
                
                cout << "Tipo R: rd=" << rd << ", rs1=" << rs1 << ", rs2=" << rs2 << endl;
                
                cont.tipoR++;
            } else if ( texto[6] == '1' || texto[6] == '9' ) { // verificar tipo I
                
                int rs1 = binToInt(binario.substr(12, 5));

                if ( rdHAZ[1] != 0 && rdHAZ[1] == rs1 )
                    cout << "HAZARD!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 1Linha\n";
                if ( rdHAZ[2] != 0 && rdHAZ[2] == rs1 )
                    cout << "HAZARD!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 2Linhas\n";

                int rd = binToInt(binario.substr(20, 5));
                rdHAZ[0] = rd;

                int imm = binToInt(binario.substr(0, 12));
                cout << "Tipo I: rd=" << rd << ", rs1=" << rs1 << ", imm=" << imm << endl;

                cont.tipoI++;
            } else if ( texto[6] == '0' || texto[6] == '8' ) { // Verifica tipo I para commands LOAD
                
                int rd = binToInt(binario.substr(20, 5));
                int rs1 = binToInt(binario.substr(12, 5));
                int imm = binToInt(binario.substr(0, 12));
                cout << "Tipo I: rd=" << rd << ", rs1=" << rs1 << ", imm=" << imm << endl;

                cont.tipoI++;
            } else if ( texto[6] == '7' || texto[6] == 'f' ) { // verificar tipo I de ecall
                
                int rs1 = binToInt(binario.substr(12, 5));

                int rd = binToInt(binario.substr(20, 5));
                int imm = binToInt(binario.substr(0, 12));
                cout << "Tipo I: rd=" << rd << ", rs1=" << rs1 << ", imm=" << imm << endl;

                cont.tipoI++;
            } else if ( texto[6] == '2' || texto[6] == 'a' ) {  // verificar tipo S (0010, 1010)(2, a(10))
     
                int rs1 = binToInt(binario.substr(12, 5));
                int rs2 = binToInt(binario.substr(7, 5));

                if ( rdHAZ[1] != 0 && (rdHAZ[1] == rs1  || rdHAZ[1] == rs2) )
                    cout << "HAZARD!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 1Linha\n";
                if ( rdHAZ[2] != 0 && (rdHAZ[2] == rs1  || rdHAZ[2] == rs2) )
                    cout << "HAZARD!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 2Linhas\n";

                cout << "Tipo S: rs1=" << rs1 << ", rs2=" << rs2 << endl;
                
                cont.tipoS++;
            } else if ( texto[6] == '6' || texto[6] == 'e' ) {  //verificar tipo B (0110, 1110)(6, e(14))
                int rs1 = binToInt(binario.substr(12, 5));
                int rs2 = binToInt(binario.substr(7, 5));
                cout << "Tipo B: rs1=" << rs1 << ", rs2=" << rs2 << endl;

                cont.tipoB++;
            }
        } else if ( texto[7] == '7' ) {
            if ( texto[6] == '6' || texto[6] == 'e' ) { //verificar tipo I (0110 ou 1110)(6 ou e)
                int rd = binToInt(binario.substr(20, 5));
                int rs1 = binToInt(binario.substr(12, 5));
                int imm = binToInt(binario.substr(0, 12));
                cout << "Tipo I: rd=" << rd << ", rs1=" << rs1 << ", imm=" << imm << endl;

                cont.tipoI++;
            } else if ( texto[6] == '1' || texto[6] == '3' || texto[6] == '9' || texto[6] == 'b' ) { //verificar tipo U (0001, 0011, 1001, 1011)(1, 3, 9, b(11))
                int rd = binToInt(binario.substr(20, 5));
                int imm = binToInt(binario.substr(0, 20));
                cout << "Tipo U: rd=" << rd << ", imm=" << imm << endl;

                cont.tipoU++;
            } 
        } else if ( texto[7] == 'f') { //verificar tipo J, o único comando com final f(1111)
            int rd = binToInt(binario.substr(20, 5));
            cout << "Tipo J: rd=" << rd << endl;

            cont.tipoJ++;
        }
        
    }

    // Close the file
    arquivo_lido.close();
    
    cout << "\nTipo R: " << cont.tipoR << endl;
    cout << "Tipo I: " << cont.tipoI << endl;
    cout << "Tipo S: " << cont.tipoS << endl;
    cout << "Tipo B: " << cont.tipoB << endl;
    cout << "Tipo U: " << cont.tipoU << endl;
    cout << "Tipo J: " << cont.tipoJ << endl;

    cout << "\nFim\n";
}