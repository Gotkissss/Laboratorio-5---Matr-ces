#include <pthread.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

/*
Formato de instrucción de 8 bits (según Lab 3):
[7]    -> bit de paridad (0=paridad par, 1=paridad impar)
[6-5]  -> código de operación (2 bits)
          00 -> Suma (A + B)
          01 -> AND (A & B)
          10 -> OR (A | B)
          11 -> XOR (A ^ B)
[4-3]  -> MOD (modificador de resultado, 2 bits)
          00 -> Directo (sin modificación)
          01 -> Negar resultado (~result)
          10 -> Doble operando A (operandA * 2)
          11 -> Intercambiar operandos
[2]    -> Bandera (1 bit): 1=invertir operandos, 0=mantener
[1-0]  -> Operandos (2 bits): código que define tanto A como B
          00 -> A=1, B=1
          01 -> A=2, B=2
          10 -> A=3, B=3
          11 -> A=4, B=4
*/

struct ThreadArg {
    std::string instruction;
    int idx;
};

// Función para validar paridad (0=par, 1=impar)
bool validateParity(int instruction) {
    int parityBit = (instruction >> 7) & 1;
    int bitCount = 0;
    
    // Contar bits del 0 al 6 (excluyendo bit de paridad)
    for (int i = 0; i < 7; i++) {
        if ((instruction >> i) & 1) {
            bitCount++;
        }
    }
    
    if (parityBit == 0) { // Paridad par
        return (bitCount % 2 == 0);
    } else { // Paridad impar
        return (bitCount % 2 == 1);
    }
}

// Funciones de operaciones
int performSum(int a, int b) { return a + b; }
int performAnd(int a, int b) { return a & b; }
int performOr(int a, int b) { return a | b; }
int performXor(int a, int b) { return a ^ b; }

// Función para invertir bits
int invertBits(int value) {
    return ~value & 0xFF; // Mantener solo 8 bits
}

// Función para obtener valores de operandos según código
void getOperandValues(int operandCode, int &operandA, int &operandB) {
    switch (operandCode) {
        case 0: operandA = 1; operandB = 1; break; // 00 = 1,1
        case 1: operandA = 2; operandB = 2; break; // 01 = 2,2
        case 2: operandA = 3; operandB = 3; break; // 10 = 3,3
        case 3: operandA = 4; operandB = 4; break; // 11 = 4,4
        default: operandA = 1; operandB = 1; break;
    }
}

// Función para aplicar modificadores MOD
int applyMod(int result, int mod, int operandA, int operandB, int opcode) {
    switch (mod) {
        case 0: // 00 - Directo
            return result;
        case 1: // 01 - Negar resultado
            return ~result & 0xFF;
        case 2: // 10 - Doble operando A
            return (operandA * 2) & 0xFF;
        case 3: // 11 - Intercambiar operandos y recalcular
            switch (opcode) {
                case 0: return performSum(operandB, operandA);
                case 1: return performAnd(operandB, operandA);
                case 2: return performOr(operandB, operandA);
                case 3: return performXor(operandB, operandA);
                default: return result;
            }
        default:
            return result;
    }
}

// Convertir string binario a entero
int binaryStringToInt(const std::string& binaryStr) {
    int result = 0;
    for (char c : binaryStr) {
        if (c == '1') {
            result = (result << 1) | 1;
        } else if (c == '0') {
            result = result << 1;
        } else {
            return -1; // Carácter inválido
        }
    }
    return result;
}

void *processInstruction(void *arg_void) {
    ThreadArg *arg = static_cast<ThreadArg*>(arg_void);
    std::string instr = arg->instruction;
    
    std::cout << "\n[Hilo " << arg->idx << "] --- Procesando Instrucción " << (arg->idx + 1) << " ---\n";
    std::cout << "[Hilo " << arg->idx << "] Instrucción: " << instr << std::endl;
    
    // Validación básica
    if (instr.size() != 8) {
        std::cout << "[Hilo " << arg->idx << "] ERROR: Instrucción inválida (longitud distinta de 8).\n";
        return nullptr;
    }
    
    int instructionValue = binaryStringToInt(instr);
    if (instructionValue == -1) {
        std::cout << "[Hilo " << arg->idx << "] ERROR: Caracteres inválidos en la instrucción.\n";
        return nullptr;
    }
    
    // Extraer campos según formato Lab 3
    int parity = (instructionValue >> 7) & 1;
    int opcode = (instructionValue >> 5) & 3;
    int mod = (instructionValue >> 3) & 3;
    int flag = (instructionValue >> 2) & 1;
    int operands = instructionValue & 3;
    
    std::cout << "[Hilo " << arg->idx << "] Paridad: " << parity 
              << ", Opcode: " << opcode << ", MOD: " << mod 
              << ", Bandera: " << flag << ", Operandos: " << operands << std::endl;
    
    // Validar paridad
    if (!validateParity(instructionValue)) {
        std::cout << "[Hilo " << arg->idx << "] ERROR: Paridad incorrecta.\n";
        return nullptr;
    }
    
    std::cout << "[Hilo " << arg->idx << "] Paridad correcta.\n";
    
    // Obtener valores de operandos
    int operandA, operandB;
    getOperandValues(operands, operandA, operandB);
    
    std::cout << "[Hilo " << arg->idx << "] Operandos originales: A=" << operandA << ", B=" << operandB << std::endl;
    
    // Aplicar bandera (invertir operandos)
    if (flag == 1) {
        operandA = invertBits(operandA);
        operandB = invertBits(operandB);
        std::cout << "[Hilo " << arg->idx << "] Bandera activada - Operandos invertidos: A=" << operandA << ", B=" << operandB << std::endl;
    }
    
    // Intercambio de operandos si MOD = 11 (se maneja en applyMod)
    if (mod == 3) {
        std::cout << "[Hilo " << arg->idx << "] MOD=11: Intercambio de operandos aplicado.\n";
    }
    
    // Realizar operación según opcode
    int result = 0;
    std::string operation = "";
    
    switch (opcode) {
        case 0: // 00 - Suma
            result = performSum(operandA, operandB);
            operation = "SUMA";
            break;
        case 1: // 01 - AND
            result = performAnd(operandA, operandB);
            operation = "AND";
            break;
        case 2: // 10 - OR
            result = performOr(operandA, operandB);
            operation = "OR";
            break;
        case 3: // 11 - XOR
            result = performXor(operandA, operandB);
            operation = "XOR";
            break;
    }
    
    std::cout << "[Hilo " << arg->idx << "] Operación " << operation << ": " 
              << operandA << " " << operation << " " << operandB << " = " << result << std::endl;
    
    // Aplicar MOD
    int originalResult = result;
    result = applyMod(result, mod, operandA, operandB, opcode);
    
    switch (mod) {
        case 0:
            std::cout << "[Hilo " << arg->idx << "] MOD: Directo (sin modificación)\n";
            break;
        case 1:
            std::cout << "[Hilo " << arg->idx << "] MOD: Negar resultado (" << originalResult << " -> " << result << ")\n";
            break;
        case 2:
            std::cout << "[Hilo " << arg->idx << "] MOD: Doble operando A (" << (operandA/2) << " * 2 = " << result << ")\n";
            break;
        case 3:
            std::cout << "[Hilo " << arg->idx << "] MOD: Intercambiar operandos y recalcular (" << result << ")\n";
            break;
    }
    
    std::cout << "[Hilo " << arg->idx << "] Resultado final: " << result << std::endl;
    std::cout << "[Hilo " << arg->idx << "] Resultado en binario: ";
    for (int i = 7; i >= 0; i--) {
        std::cout << ((result >> i) & 1);
    }
    std::cout << std::endl;
    
    return nullptr;
}

int main() {
    std::cout << "--- DECODIFICADOR DE INSTRUCCIONES BINARIAS PARALELO ---\n";
    std::cout << "Formato: [Paridad][Opcode][MOD][Flag][Operandos]\n";
    std::cout << "Ingrese instrucciones binarias de 8 bits separadas por espacios:\n";
    
    std::string line;
    std::getline(std::cin, line);
    
    // Parsear instrucciones
    std::vector<std::string> instructions;
    std::stringstream ss(line);
    std::string temp;
    
    while (ss >> temp) {
        if (!temp.empty()) {
            instructions.push_back(temp);
        }
    }
    
    int n = instructions.size();
    if (n == 0) {
        std::cout << "No se ingresaron instrucciones.\n";
        return 0;
    }
    
    std::cout << "\nProcesando " << n << " instrucciones con hilos paralelos...\n";
    
    // Crear hilos
    std::vector<pthread_t> threads(n);
    std::vector<ThreadArg> args(n);
    
    for (int i = 0; i < n; ++i) {
        args[i].instruction = instructions[i];
        args[i].idx = i;
        
        if (pthread_create(&threads[i], nullptr, processInstruction, &args[i]) != 0) {
            std::cerr << "Error al crear el hilo " << i << std::endl;
            return 1;
        }
    }
    
    // Esperar a que todos los hilos terminen
    for (int i = 0; i < n; ++i) {
        pthread_join(threads[i], nullptr);
    }
    
    std::cout << "\n--- Procesamiento completado ---\n";
    return 0;
}