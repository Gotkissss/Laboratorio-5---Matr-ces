#include <bits/stdc++.h>
#include <pthread.h>
using namespace std;

struct ThreadArgs {
    int *data;         // puntero al primer elemento de la matriz (arreglo lineal)
    size_t n;          // tamaño N (matriz N x N)
    size_t row_begin;  // fila inicial (incluida)
    size_t row_end;    // fila final (excluida)
    int tid;           // id del hilo (para impresión)
    bool print_all;    // si debe imprimir todas las filas que procesa
};

static inline int *row_ptr(int *base, size_t n, size_t r) {
    return base + r * n; // fila r, stride n
}

void print_row(const int *row, size_t n) {
    for (size_t j = 0; j < n; ++j) {
        cout << setw(4) << row[j] << (j + 1 == n ? '\n' : ' ');
    }
}

void *sort_rows(void *arg) {
    ThreadArgs *args = static_cast<ThreadArgs *>(arg);
    int *base = args->data;
    size_t n = args->n;

    // Ordenar, fila por fila, el segmento asignado
    for (size_t r = args->row_begin; r < args->row_end; ++r) {
        int *row = row_ptr(base, n, r);
        sort(row, row + n);
    }

    // Impresión: cada hilo imprime SOLO las filas que ordenó
    // Para no saturar la salida en N grandes, por defecto se imprime un resumen.
    if (!args->print_all) {
        cout << "[Hilo " << args->tid << "] filas ordenadas: "
             << args->row_begin << ".." << (args->row_end ? args->row_end - 1 : 0)
             << " (total " << (args->row_end - args->row_begin) << ")\n";
        // Mostrar una vista previa: primeras 1–2 filas del rango
        size_t preview = min<size_t>(2, (args->row_end - args->row_begin));
        for (size_t k = 0; k < preview; ++k) {
            size_t r = args->row_begin + k;
            cout << "[Hilo " << args->tid << "] fila " << r << ": ";
            print_row(row_ptr(base, n, r), n);
        }
    } else {
        for (size_t r = args->row_begin; r < args->row_end; ++r) {
            cout << "[Hilo " << args->tid << "] fila " << r << ": ";
            print_row(row_ptr(base, n, r), n);
        }
    }

    return nullptr;
}

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Parámetros: N (tamaño), T (#hilos), --print-all
    size_t N = 200; // tamaño grande por defecto pero menor a 500
    int T = 4;      // hilos por defecto
    bool print_all = false;

    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "--print-all") print_all = true;
        else if (a.rfind("--n=", 0) == 0) N = stoul(a.substr(5));
        else if (a.rfind("--threads=", 0) == 0) T = stoi(a.substr(10));
        else if (i + 1 < argc && (a == "-n")) N = stoul(argv[++i]);
        else if (i + 1 < argc && (a == "-t")) T = stoi(argv[++i]);
        else {
            cerr << "Uso: " << argv[0] << " [-n N|--n=N] [-t T|--threads=T] [--print-all]\n";
            return 1;
        }
    }

    if (N == 0) { cerr << "N debe ser > 0\n"; return 1; }
    if (T <= 0) T = 1;
    if ((size_t)T > N) T = (int)N; // no más hilos que filas

    // Reservar matriz N x N en un bloque contiguo
    vector<int> mat(N * N);

    // Inicializar con enteros aleatorios
    std::mt19937 rng((uint32_t)chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist(0, 999);
    for (size_t i = 0; i < N * N; ++i) mat[i] = dist(rng);

    cout << "Matriz generada aleatoriamente de " << N << "x" << N
         << ", hilos: " << T << "\n";

    // Crear hilos
    vector<pthread_t> threads(T);
    vector<ThreadArgs> args(T);

    size_t rows_per = (N + T - 1) / (size_t)T; // ceil
    for (int t = 0; t < T; ++t) {
        size_t begin = (size_t)t * rows_per;
        size_t end = min(N, begin + rows_per);
        args[t] = ThreadArgs{mat.data(), N, begin, end, t, print_all};
        int rc = pthread_create(&threads[t], nullptr, sort_rows, &args[t]);
        if (rc != 0) {
            cerr << "Error creando hilo " << t << ": " << strerror(rc) << "\n";
            return 2;
        }
    }

    // Esperar hilos
    for (int t = 0; t < T; ++t) {
        pthread_join(threads[t], nullptr);
    }

    // Validación: comprobar que cada fila esté ordenada no-decreciente
    bool ok = true;
    for (size_t r = 0; r < N && ok; ++r) {
        int *row = row_ptr(mat.data(), N, r);
        if (!std::is_sorted(row, row + N)) ok = false;
    }
    cout << (ok ? "\nTodas las filas quedaron ordenadas.\n"
                 : "\nERROR: Se detectó una fila no ordenada.\n");

    return 0;
}
