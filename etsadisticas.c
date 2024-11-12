//Esta madre va a ser el puto código que va a analizar los perros quantums y todo ese dervergue
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

typedef struct {
    int pid;
    int tiempo_llegada;
    int tiempo_ejecucion;
    int tiempo_espera;
    int tiempo_restante;
    int tiempo_finalizacion;
} Proceso;

Proceso *procesos;
int num_procesos = 0;

void agregar_proceso(int pid, int tiempo_llegada, int tiempo_ejecucion) {
    procesos[num_procesos].pid = pid;
    procesos[num_procesos].tiempo_llegada = tiempo_llegada;
    procesos[num_procesos].tiempo_ejecucion = tiempo_ejecucion;
    procesos[num_procesos].tiempo_restante = tiempo_ejecucion;
    procesos[num_procesos].tiempo_espera = 0;
    procesos[num_procesos].tiempo_finalizacion = 0;
    num_procesos++;
}

void calcular_tiempos() {
    int tiempo_actual = 0;

    for (int i = 0; i < num_procesos; i++) {
        if (tiempo_actual < procesos[i].tiempo_llegada) {
            tiempo_actual = procesos[i].tiempo_llegada; // Esperar hasta que llegue el proceso
        }
        procesos[i].tiempo_espera = tiempo_actual - procesos[i].tiempo_llegada;
        tiempo_actual += procesos[i].tiempo_restante;
        procesos[i].tiempo_finalizacion = tiempo_actual;
    }
}

void imprimir_estadisticas() {
    printf("PID\tLlegada\tEjecucion\tEspera\tRestante\tFinalizacion\n");
    for (int i = 0; i < num_procesos; i++) {
        printf("%d\t%d\t%d\t\t%d\t%d\t\t%d\n", 
               procesos[i].pid, 
               procesos[i].tiempo_llegada, 
               procesos[i].tiempo_ejecucion, 
               procesos[i].tiempo_espera, 
               procesos[i].tiempo_restante, 
               procesos[i].tiempo_finalizacion);
    }
}

int main() {
    // Crear memoria compartida
    int shmid = shmget(IPC_PRIVATE, sizeof(Proceso) * 100, IPC_CREAT | 0666);
    procesos = (Proceso *)shmat(shmid, NULL, 0);

    // Simulación de agregar procesos
    agregar_proceso(1, 0, 5);
    agregar_proceso(2, 1, 3);
    agregar_proceso(3, 2, 8);

    // Calcular tiempos
    calcular_tiempos();

    // Imprimir estadísticas
    imprimir_estadisticas();

    // Desvincular y eliminar memoria compartida
    shmdt(procesos);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}