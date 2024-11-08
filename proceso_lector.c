#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <string.h>
#include <time.h>

typedef struct {
    int pid;               // ID del proceso
    int tiempo_ejecucion;  // Tiempo total de ejecución
    int tiempo_restante;   // Tiempo restante
} Proceso;

Proceso *procesos;          // Creamos un puntero de procesos de tipo Proceso

int id_semaforo;            

// Es lo que quivalente a down, la función vista en clase
void bajar_semaforo() {
    struct sembuf sb = {0, -1, 0}; 
    semop(id_semaforo, &sb, 1);
}

// Es lo equivalente a up, la función vista en clase 
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

int main() {
    if (fork() == 0) {
        proceso_lector();
    } else {
        /* Implementar la siguiente función
        proceso_despachador();
        */ 
    }
    return 0;
}
