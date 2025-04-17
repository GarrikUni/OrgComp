#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <algorithm>
using namespace std;

//g++ -o <arquivo saída> <arquivo entrada>; <-- compilar o exe

// Observem que em um momento futuro (M2) será solicitado que vocês separem as partes
// de uma instrução lida, por exemplo:
// A instrução 0x00500413 formato = i, rd = 8, f3 = 0, rs1 = 0, imed = 5 

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
    return binario.substr(25, 7); //
}

int main() {

    Contador cont;

    cout << "Comeco\n\n";
    string texto;

    ifstream arquivo_lido("ex01_hex_dump.txt"); // Arquivo Hex dump 

    // Use a while loop together with the getline() function to read the file line by line
    while ( getline(arquivo_lido, texto) ) {
        // Verificar os caracteres 7(texto[6]) e 8(texto[7]), pois estes são os caracteres que informam o OPCODE
        // Converter o hexadecimal pra binário, e analisar qual o OPCODE
        // edit: é possível analisar o OPCODE sem converter pra binário
        
        cout << hexToBinary(texto) << endl;
        
        string binario = hexToBinary(texto);

        if ( texto[7] == '3' ) { // Verifica se se é um dos tipos que termina com valor 3
            
            if ( texto[6] == '3' || texto[6] == 'b' ) { //verificar tipo R (0011 ou 1011)(3 ou b)
                
                int rd = binToInt(binario.substr(20, 5));
                int rs1 = binToInt(binario.substr(12, 5));
                int rs2 = binToInt(binario.substr(7, 5));
                cout << "Tipo R: rd=" << rd << ", rs1=" << rs1 << ", rs2=" << rs2 << endl;
                
                cout << texto << " => Tipo R\n";
                cont.tipoR++;
            } else if ( texto[6] == '0' || texto[6] == '1' || texto[6] == '7'  || texto[6] == '8'  || texto[6] == '9'  || texto[6] == 'f' ) { //verificar tipo I (0000, 1000, 0001, 1001, 1111, 0111)(0, 8, 1, 9, f, 7)
                
                int rd = binToInt(binario.substr(20, 5));
                int rs1 = binToInt(binario.substr(12, 5));
                cout << "Tipo I: rd=" << rd << ", rs1=" << rs1 << endl;

                cout << texto << " => Tipo I\n";
                cont.tipoI++;
            } else if ( texto[6] == '2' || texto[6] == 'a' ) {  //verificar tipo S (0010, 1010)(2, a(10))
                
                int rs1 = binToInt(binario.substr(12, 5));
                int rs2 = binToInt(binario.substr(7, 5));
                cout << "Tipo S: rs1=" << rs1 << ", rs2=" << rs2 << endl;
                
                cout << texto << " => Tipo S\n";
                cont.tipoS++;
            } else if ( texto[6] == '6' || texto[6] == 'e' ) {  //verificar tipo B (0110, 1110)(6, e(14))
                cout << texto << " => Tipo B\n";
                cont.tipoB++;
            }
        } else if ( texto[7] == '7' ) {
            if ( texto[6] == '6' || texto[6] == 'e' ) { //verificar tipo I (0110 ou 1110)(6 ou e)
                cout << texto << " => Tipo I\n";
                cont.tipoI++;
            } else if ( texto[6] == '1' || texto[6] == '3' || texto[6] == '9' || texto[6] == 'b' ) { //verificar tipo U (0001, 0011, 1001, 1011)(1, 3, 9, b(11))
                cout << texto << " => Tipo U\n";
                cont.tipoU++;
            } 
        } else if ( texto[7] == 'f') { //verificar tipo J, o único comando com final f(1111)
            cout << texto << " => Tipo J\n";
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