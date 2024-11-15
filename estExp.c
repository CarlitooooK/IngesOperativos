#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>

#define PROCESS 500  // Máximo de procesos

typedef struct {
    int pid;
    int tiempo_llegada;
    int tiempo_ejecucion;
    int tiempo_espera;
    int tiempo_restante;
    int tiempo_finalizacion;
} Proceso;

Proceso *procesos;  // Puntero a la memoria compartida
Proceso *procesos_locales;  // Estructura local para almacenar los procesos
int id_memoria;
int id_semaforo;
int num_procesos = 0;

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

// Función para transferir procesos desde la memoria compartida a la estructura local
void transferir_procesos() {
    bajar_semaforo();
    for (int i = 0; i < PROCESS; i++) {
        if (procesos[i].tiempo_ejecucion > 0) {  // Solo lee procesos válidos
            procesos_locales[num_procesos] = procesos[i];
            num_procesos++;
        }
    }
    subir_semaforo();
}

// Calcular tiempos para los procesos locales
void calcular_tiempos() {
    int tiempo_actual = 0;

    for (int i = 0; i < num_procesos; i++) {
        if (tiempo_actual < procesos_locales[i].tiempo_llegada) {
            tiempo_actual = procesos_locales[i].tiempo_llegada;  // Espera hasta que llegue el proceso
        }
        procesos_locales[i].tiempo_espera = tiempo_actual - procesos_locales[i].tiempo_llegada;
        tiempo_actual += procesos_locales[i].tiempo_restante;
        procesos_locales[i].tiempo_finalizacion = tiempo_actual;
    }
}

// Imprimir estadísticas de los procesos locales
void imprimir_estadisticas() {
    printf("PID\tLlegada\tEjecución\tEspera\tRestante\tFinalización\n");
    for (int i = 0; i < num_procesos; i++) {
        printf("%d\t%d\t%d\t\t%d\t%d\t\t%d\n", 
               procesos_locales[i].pid, 
               procesos_locales[i].tiempo_llegada, 
               procesos_locales[i].tiempo_ejecucion, 
               procesos_locales[i].tiempo_espera, 
               procesos_locales[i].tiempo_restante, 
               procesos_locales[i].tiempo_finalizacion);
    }
}

int main() {
    // Configuración de memoria compartida
    key_t clave_memoria = ftok("/bin/ls", 33);  // Clave debe coincidir con el emisor
    id_memoria = shmget(clave_memoria, sizeof(Proceso) * PROCESS, 0666);
    if (id_memoria == -1) {
        perror("Error al acceder a la memoria compartida");
        exit(1);
    }
    procesos = (Proceso *)shmat(id_memoria, NULL, 0);
    if (procesos == (void *)-1) {
        perror("Error al asignar memoria compartida");
        exit(1);
    }

    // Configuración del semáforo
    key_t clave_semaforo = ftok("/bin/ls", 44);  // Clave debe coincidir con el emisor
    id_semaforo = semget(clave_semaforo, 1, 0666);
    if (id_semaforo == -1) {
        perror("Error al acceder al semáforo");
        exit(1);
    }

    // Crear un arreglo local para almacenar los procesos transferidos
    procesos_locales = (Proceso *)malloc(sizeof(Proceso) * PROCESS);

    // Transferir procesos desde la memoria compartida
    transferir_procesos();

    // Calcular tiempos y mostrar estadísticas
    calcular_tiempos();
    imprimir_estadisticas();

    // Liberar recursos
    shmdt(procesos);
    shmctl(id_memoria, IPC_RMID, NULL);
    free(procesos_locales);

    return 0;
}
