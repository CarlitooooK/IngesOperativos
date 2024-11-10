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
    int tiempo_ejecucion;
    int tiempo_restante;
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
            if (feof(archivo)) break;
            Proceso proc;
            fscanf(archivo, "%d %d", &proc.pid, &proc.tiempo_ejecucion);
            printf("Archivo leído exitosamente, ejemplo: Proceso #%d, ID:%d \n", proc.pid, proc.tiempo_ejecucion); // Mensaje de prueba
            proc.tiempo_restante = proc.tiempo_ejecucion;
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
    if (fork() == 0) {
        proceso_lector();
    } else {
        proceso_despachador();
    }
    return 0;
}
