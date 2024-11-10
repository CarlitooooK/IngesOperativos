#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <time.h>

#define VEHICLES 10

// Semáforos
sem_t *sem_mutex;

// Memoria compartida
int *vehicles_on_bridge;
int *bridge_direction;  // 0 = ningún vehículo en el puente, 1 = dirección A->B, -1 = dirección B->A

void vehicle(int id, int direction) {
    sem_wait(sem_mutex);
    while (*vehicles_on_bridge > 0 && *bridge_direction != direction) {
        sem_post(sem_mutex);
        usleep(1000); // Esperar un poco antes de intentar de nuevo
        sem_wait(sem_mutex);
    }

    // Si el puente está vacío, establecemos la dirección del puente
    if (*vehicles_on_bridge == 0) {
        *bridge_direction = direction;
    }

    // Incrementamos el contador de vehículos en el puente
    (*vehicles_on_bridge)++;
    printf("Vehículo %d cruzando en dirección %d\n", id, direction);
    sem_post(sem_mutex);

    // Simula el tiempo de cruce
    usleep(rand() % 1000000);

    // Salir del puente
    sem_wait(sem_mutex);
    (*vehicles_on_bridge)--;
    printf("Vehículo %d ha salido en dirección %d\n", id, direction);

    // Si no quedan vehículos en el puente, liberamos la dirección
    if (*vehicles_on_bridge == 0) {
        *bridge_direction = 0;
    }
    sem_post(sem_mutex);
}

int main() {
    srand(time(NULL));

    // Crear semáforo
    sem_mutex = sem_open("/sem_mutex", O_CREAT, 0666, 1);

    // Configurar memoria compartida
    int shm_fd = shm_open("/shm_bridge", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(int) * 2);
    int *shared_memory = mmap(0, sizeof(int) * 2, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
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

    // Liberar recursos
    sem_close(sem_mutex);
    sem_unlink("/sem_mutex");
    shm_unlink("/shm_bridge");

    return 0;
}
