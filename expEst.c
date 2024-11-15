#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <time.h>

typedef struct {
    int pid;
    int tiempo_llegada;
    int tiempo_ejecucion;
    int tiempo_espera;
    int tiempo_restante;
    int tiempo_finalizacion;
} Proceso;

Proceso *procesos;
int id_semaforo;

void bajar_semaforo() {
    struct sembuf sb = {0, -1, 0};
    semop(id_semaforo, &sb, 1);
}

void subir_semaforo() {
    struct sembuf sb = {0, 1, 0};
    semop(id_semaforo, &sb, 1);
}

void proceso_lector() {
    FILE *archivo = fopen("procesos.txt", "r");
    if (!archivo) {
        perror("fopen");
        exit(1);
    }

    while (1) {
        bajar_semaforo(); // Lo bajamos para indicar que voy a estar ocupado en este proceso
        for (int i = 0; i < rand() % 20 + 1; i++) { // Tamaño de lote aleatorio
            Proceso proc;
            if (fscanf(archivo, "%d %d", &proc.pid, &proc.tiempo_ejecucion) != 2) break;
            printf("Archivo leído exitosamente, ejemplo: Proceso #%d, ID:%d \n", proc.pid, proc.tiempo_ejecucion); // Mensaje de prueba
            proc.tiempo_llegada = time(NULL); // Asignar tiempo de llegada
            proc.tiempo_restante = proc.tiempo_ejecucion;
            proc.tiempo_espera = 0;
            proc.tiempo_finalizacion = 0;
            procesos[proc.pid] = proc; // Guardar proceso en memoria compartida
            printf("Proceso %d agregado con tiempo de ejecución %d\n", proc.pid, proc.tiempo_ejecucion);
        }
        
        subir_semaforo(); // Lo subimos para que otro proceso pueda acceder
        
        sleep(rand() % 10 + 1); // Espera aleatoria
    }
    fclose(archivo);
}

// Calcula un quantum dinámico en función del tiempo restante promedio de los procesos activos
int calcular_quantum_dinamico() {
    int suma_tiempo = 0;
    int contador = 0;

    for (int i = 0; i < 500; i++) {  // Suponiendo hasta 500 procesos
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

        int quantum_dinamico = calcular_quantum_dinamico();  // Calcula el quantum antes de cada ciclo
        printf("Quantum dinámico calculado: %d segundos\n", quantum_dinamico);

        for (int i = 0; i < 500; i++) {  // Suponiendo hasta 500 procesos
            if (procesos[i].tiempo_restante > 0) {
                printf("Iniciando proceso #%d con tiempo restante %d\n", procesos[i].pid, procesos[i].tiempo_restante);

                if (procesos[i].tiempo_restante <= quantum_dinamico) {
                    sleep(procesos[i].tiempo_restante);
                    procesos[i].tiempo_finalizacion = time(NULL); // Asignar tiempo de finalización
                    procesos[i]. tiempo_restante = 0; // Proceso finalizado
                    printf("Proceso #%d finalizado\n", procesos[i].pid);
                } else {
                    sleep(quantum_dinamico);
                    procesos[i].tiempo_restante -= quantum_dinamico; // Reducir tiempo restante
                    printf("Proceso #%d interrumpido, tiempo restante ahora %d\n", procesos[i].pid, procesos[i].tiempo_restante);
                }
            }
        }

        subir_semaforo(); // Liberar acceso a la cola de procesos
        sleep(1); // Espera entre ciclos de despacho
    }
}

void imprimir_estadisticas() {
    printf("PID\tLlegada\tEjecucion\tEspera\tRestante\tFinalizacion\n");
    for (int i = 0; i < 500; i++) { // Suponiendo hasta 500 procesos
        if (procesos[i].tiempo_restante == 0) { // Solo imprimir procesos finalizados
            printf("%d\t%d\t%d\t\t%d\t%d\t\t%d\n", 
                   procesos[i].pid, 
                   procesos[i].tiempo_llegada, 
                   procesos[i].tiempo_ejecucion, 
                   procesos[i].tiempo_espera, 
                   procesos[i].tiempo_restante, 
                   procesos[i].tiempo_finalizacion);
        }
    }
}

int main() {
    // Crear memoria compartida
    int shmid = shmget(IPC_PRIVATE, sizeof(Proceso) * 500, IPC_CREAT | 0666);
    procesos = (Proceso *)shmat(shmid, NULL, 0);

    // Crear semáforo
    id_semaforo = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    semctl(id_semaforo, 0, SETVAL, 1); // Inicializar semáforo en 1

    // Crear procesos para lector y despachador
    if (fork() == 0) {
        proceso_lector();
        exit(0);
    }

    if (fork() == 0) {
        proceso_despachador();
        exit(0);
    }

    // Esperar un tiempo y luego imprimir estadísticas
    sleep(30); // Tiempo de ejecución de los procesos
    imprimir_estadisticas();

    // Desvincular y eliminar memoria compartida
    shmdt(procesos);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(id_semaforo, 0, IPC_RMID); // Eliminar semáforo

    return 0;
}