#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>

// numero de coches
#define NUM_CARS 5

// Función para simular la llegada de un auto
void llegada_auto(int car_id, int write_pipe) {
    printf("Auto %d ha llegado al puente.\n", car_id);

    // Simular la espera del auto (opcional)
    sleep(1);

    // Enviar señal al proceso principal de que el auto está listo para cruzar
    write(write_pipe, &car_id, sizeof(car_id));
    printf("Auto %d está esperando para cruzar el puente.\n", car_id);

    // Simular el cruce del puente
    sleep(2);
    printf("Auto %d ha cruzado el puente.\n", car_id);
}

int main() {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Error al crear la tubería");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_CARS; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Error al crear el proceso hijo");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // Proceso hijo
            close(pipe_fd[0]); // Cerrar el extremo de lectura de la tubería
            llegada_auto(i + 1, pipe_fd[1]);
            close(pipe_fd[1]); // Cerrar el extremo de escritura de la tubería
            exit(0);
        }
    }

    // Proceso principal
    close(pipe_fd[1]); // Cerrar el extremo de escritura de la tubería
    int car_id;
    for (int i = 0; i < NUM_CARS; i++) {
        // Leer el id del auto que está listo para cruzar
        read(pipe_fd[0], &car_id, sizeof(car_id));
        printf("El puente está ahora abierto para el auto %d.\n", car_id);

        // Simular que el puente está abierto para este auto
        sleep(3);
        printf("El puente está ahora cerrado para el auto %d.\n", car_id);

        // Esperar a que el proceso hijo termine
        wait(NULL);
    }
    close(pipe_fd[0]); // Cerrar el extremo de lectura de la tubería

    printf("Todos los autos han cruzado el puente.\n");
    return 0;
}
