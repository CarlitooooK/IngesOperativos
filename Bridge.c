#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>

#define VEHICLES 10

// Definición para las operaciones de semáforo
struct sembuf sem_wait = {0, -1, 0};  // P() - Decrementa el semáforo
struct sembuf sem_signal = {0, 1, 0}; // V() - Incrementa el semáforo

// Identificadores para la memoria compartida y el semáforo
int shm_id, sem_id;
int *vehicles_on_bridge, *bridge_direction;

// Función que representa el comportamiento de un vehículo
void vehicle(int id, int direction) {
    // Acceder al semáforo para sincronizar el acceso al puente
    semop(sem_id, &sem_wait, 1);
    while (*vehicles_on_bridge > 0 && *bridge_direction != direction) {
        semop(sem_id, &sem_signal, 1);  // Libera el semáforo mientras espera
        usleep(1000);                    // Espera un poco antes de intentar de nuevo
        semop(sem_id, &sem_wait, 1);     // Intenta adquirir el semáforo nuevamente
    }

    // Si el puente está vacío, establece la dirección del puente
    if (*vehicles_on_bridge == 0) {
        *bridge_direction = direction;
    }

    // Incrementa el contador de vehículos en el puente
    (*vehicles_on_bridge)++;
    printf("Vehículo %d cruzando en dirección %d\n", id, direction);
    semop(sem_id, &sem_signal, 1);  // Libera el semáforo

    // Simula el tiempo de cruce
    usleep(rand() % 1000000);

    // Salir del puente
    semop(sem_id, &sem_wait, 1);
    (*vehicles_on_bridge)--;
    printf("Vehículo %d ha salido en dirección %d\n", id, direction);

    // Si no quedan vehículos en el puente, liberamos la dirección
    if (*vehicles_on_bridge == 0) {
        *bridge_direction = 0;
    }
    semop(sem_id, &sem_signal, 1);  // Libera el semáforo
}

int main() {
    srand(time(NULL));

    // Crear semáforo de System V (1 semáforo en el conjunto)
    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);  // Inicializar el semáforo a 1

    // Configurar memoria compartida para variables de control
    shm_id = shmget(IPC_PRIVATE, sizeof(int) * 2, IPC_CREAT | 0666);
    int *shared_memory = (int *)shmat(shm_id, NULL, 0);
    vehicles_on_bridge = &shared_memory[0];
    bridge_direction = &shared_memory[1];

    *vehicles_on_bridge = 0;
    *bridge_direction = 0;

    // Crear procesos para cada vehículo
    for (int i = 0; i < VEHICLES; i++) {
        int direction = rand() % 2 == 0 ? 1 : -1;
        pid_t pid = fork();

        if (pid == 0) {
            vehicle(i + 1, direction);
            exit(0);
        }
    }

    // Esperar a que todos los procesos hijos terminen
    for (int i = 0; i < VEHICLES; i++) {
        wait(NULL);
    }

    // Liberar recursos compartidos
    semctl(sem_id, 0, IPC_RMID);  // Eliminar el semáforo
    shmdt(shared_memory);         // Desvincular memoria compartida
    shmctl(shm_id, IPC_RMID, 0);  // Eliminar la memoria compartida

    return 0;
}
