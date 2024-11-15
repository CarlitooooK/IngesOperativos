#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <time.h>

#define PROCESS 500  // Número máximo de procesos

typedef struct {
    int pid;
    int tiempo_llegada;
    int tiempo_ejecucion;
    int tiempo_restante;
} Proceso;

Proceso *procesos;   // Puntero a la memoria compartida
int id_semaforo;     // Identificador del semáforo
int id_memoria;      // Identificador de la memoria compartida

// Función para bajar el semáforo
void bajar_semaforo() {
    struct sembuf sb = {0, -1, 0};
    semop(id_semaforo, &sb, 1);
}

// Función para subir el semáforo
void subir_semaforo() {
    struct sembuf sb = {0, 1, 0};
    semop(id_semaforo, &sb, 1);
}

// Proceso lector que lee procesos del archivo y los guarda en memoria compartida
void proceso_lector() {
    FILE *archivo = fopen("procesos.txt", "r");
    if (!archivo) {
        perror("Error al abrir el archivo");
        exit(1);
    }

    int tiempo_llegada = 0;  // Tiempo de llegada inicial

    while (1) {
        bajar_semaforo(); // Bajamos el semáforo para acceso exclusivo
        for (int i = 0; i < rand() % 20 + 1; i++) { // Tamaño de lote aleatorio
            if (feof(archivo)) break;
            Proceso proc;
            fscanf(archivo, "%d %d", &proc.pid, &proc.tiempo_ejecucion);
            proc.tiempo_llegada = tiempo_llegada++;  // Asignamos tiempo de llegada y lo incrementamos
            proc.tiempo_restante = proc.tiempo_ejecucion;
            procesos[proc.pid] = proc; // Guardar el proceso en memoria compartida
            printf("Proceso %d agregado con tiempo de llegada %d y ejecución %d\n", 
                   proc.pid, proc.tiempo_llegada, proc.tiempo_ejecucion);
        }
        subir_semaforo(); // Subimos el semáforo para que otro proceso pueda acceder
        sleep(rand() % 10 + 1); // Espera aleatoria
    }
    fclose(archivo);
}

// Calcula el quantum dinámico en función del tiempo restante promedio de los procesos activos
int calcular_quantum_dinamico() {
    int suma_tiempo = 0;
    int contador = 0;

    for (int i = 0; i < PROCESS; i++) {
        if (procesos[i].tiempo_restante > 0) {
            suma_tiempo += procesos[i].tiempo_restante;
            contador++;
        }
    }
    return (contador > 0) ? (suma_tiempo / contador) : 1;  // Quantum dinámico o mínimo de 1 segundo
}

// Proceso despachador que simula la ejecución de los procesos usando Round Robin con quantum dinámico
void proceso_despachador() {
    while (1) {
        bajar_semaforo();  // Acceso exclusivo a la cola de procesos
        int quantum_dinamico = calcular_quantum_dinamico();
        printf("Quantum dinámico calculado: %d segundos\n", quantum_dinamico);

        for (int i = 0; i < PROCESS; i++) {
            if (procesos[i].tiempo_restante > 0) {
                printf("Iniciando proceso #%d con tiempo restante %d\n", procesos[i].pid, procesos[i].tiempo_restante);
                if (procesos[i].tiempo_restante <= quantum_dinamico) {
                    sleep(procesos[i].tiempo_restante);
                    procesos[i].tiempo_restante = 0;
                    printf("Proceso #%d ha terminado.\n", procesos[i].pid);
                } else {
                    sleep(quantum_dinamico);
                    procesos[i].tiempo_restante -= quantum_dinamico;
                    printf("Proceso #%d suspendido, tiempo restante %d\n", procesos[i].pid, procesos[i].tiempo_restante);
                }
            }
        }
        subir_semaforo();  // Permitir acceso al lector
        sleep(1);  // Pausa para evitar un bucle ocupado
    }
}

int main() {
    // Configuración de memoria compartida
    key_t clave_memoria = ftok("/bin/ls", 33);
    id_memoria = shmget(clave_memoria, sizeof(Proceso) * PROCESS, 0666 | IPC_CREAT);
    if (id_memoria == -1) {
        perror("Error al crear memoria compartida");
        exit(1);
    }
    procesos = (Proceso *)shmat(id_memoria, NULL, 0);
    if (procesos == (void *) -1) {
        perror("Error al asignar memoria compartida");
        exit(1);
    }

    // Configuración del semáforo
    key_t clave_semaforo = ftok("/bin/ls", 44);
    id_semaforo = semget(clave_semaforo, 1, 0666 | IPC_CREAT);
    if (id_semaforo == -1) {
        perror("Error al crear semáforo");
        exit(1);
    }
    semctl(id_semaforo, 0, SETVAL, 1);  // Inicializa el semáforo en 1

    // Creación de procesos
    if (fork() == 0) {
        proceso_lector();
    } else {
        proceso_despachador();
    }

    // Liberar recursos
    shmdt(procesos);
    shmctl(id_memoria, IPC_RMID, NULL);
    semctl(id_semaforo, 0, IPC_RMID);

    return 0;
}
