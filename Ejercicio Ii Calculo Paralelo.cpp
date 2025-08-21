#include <pthread.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <climits>

struct ThreadArg {
    int start;         // índice inicial (inclusivo)
    int end;           // índice final (inclusivo)
    int *arr;          // puntero al arreglo
    long long *partials; // puntero al arreglo de sumas parciales
    int idx;           // índice del hilo (posición en partials)
};

void *thread_sum(void *arg_void) {
    ThreadArg *arg = static_cast<ThreadArg*>(arg_void);
    long long sum = 0;
    for (int i = arg->start; i <= arg->end; ++i) {
        sum += arg->arr[i];
    }
    arg->partials[arg->idx] = sum;
    // Imprime la suma parcial desde el hilo
    std::cout << "[Hilo " << arg->idx << "] rango (" << arg->start << ", " << arg->end << ") -> suma parcial = " << sum << std::endl;
    return nullptr;
}

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    int n; // tamaño del arreglo
    int num_threads;

    // Leer entradas y validar: el tamaño del arreglo debe ser mayor que la cantidad de hilos
    while (true) {
        std::cout << "Ingrese el tamaño del arreglo (entero positivo): ";
        if (!(std::cin >> n) || n <= 0) {
            std::cin.clear();
            std::cin.ignore(INT_MAX, '\n');
            std::cout << "Entrada inválida. Intente de nuevo.\n";
            continue;
        }
        std::cout << "Ingrese la cantidad de hilos a crear (entero positivo): ";
        if (!(std::cin >> num_threads) || num_threads <= 0) {
            std::cin.clear();
            std::cin.ignore(INT_MAX, '\n');
            std::cout << "Entrada inválida. Intente de nuevo.\n";
            continue;
        }
        if (n <= num_threads) {
            std::cout << "Error: el tamaño del arreglo (" << n << ") debe ser mayor que la cantidad de hilos (" << num_threads << ").\n";
            std::cout << "Por favor reingrese la información.\n";
            continue;
        }
        break;
    }

    // Crear y llenar el arreglo con valores aleatorios (por ejemplo -100..100)
    std::vector<int> arr(n);
    for (int i = 0; i < n; ++i) arr[i] = (std::rand() % 201) - 100; // [-100,100]

    // Mostrar una porción del arreglo (opcional)
    std::cout << "Arreglo (primeros 20 elementos o menos): ";
    for (int i = 0; i < std::min(n, 20); ++i) std::cout << arr[i] << (i+1< std::min(n,20)? ", ": "\n");

    // Preparar la partición equitativa del trabajo entre hilos
    std::vector<int> partials(num_threads, 0);
    std::vector<pthread_t> threads(num_threads);
    std::vector<ThreadArg> args(num_threads);

    int base = n / num_threads;
    int rem = n % num_threads; // primeros 'rem' hilos obtienen 1 elemento extra
    int current = 0;

    for (int i = 0; i < num_threads; ++i) {
        int count = base + (i < rem ? 1 : 0);
        int start = current;
        int end = current + count - 1;
        args[i].start = start;
        args[i].end = end;
        args[i].arr = arr.data();
        args[i].partials = partials.data();
        args[i].idx = i;
        current = end + 1;

        if (pthread_create(&threads[i], nullptr, thread_sum, &args[i]) != 0) {
            std::cerr << "Error al crear el hilo " << i << std::endl;
            return 1;
        }
    }

    // Esperar a que todos los hilos finalicen
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    // Reunir resultados parciales
    long long total = 0;
    for (int i = 0; i < num_threads; ++i) total += partials[i];

    std::cout << "Suma total calculada por los hilos = " << total << std::endl;

    // (Opcional) calcular la suma secuencial para verificar
    long long seq_sum = 0;
    for (int i = 0; i < n; ++i) seq_sum += arr[i];
    std::cout << "Suma total secuencial (verificación) = " << seq_sum << std::endl;

    if (total == seq_sum) std::cout << "Verificación OK: las sumas coinciden." << std::endl;
    else std::cout << "Verificación ERROR: las sumas no coinciden." << std::endl;

    return 0;
}
