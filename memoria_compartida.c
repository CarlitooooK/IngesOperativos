#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <time.h>

#define PROCESS 500  // Define el número máximo de procesos.

typedef struct {
    int pid;
    int tiempo_ejecucion;
    int tiempo_restante;
} Proceso;  // Estructura que representa un proceso en ejecución.

Proceso *procesos;
int *lote_procesos;
int id_semaforo, id_semaforo_lotes;
int shmid_procesos, shmid_lote;  // IDs de memoria compartida y semáforos.

void bajar_semaforo(int sem_id) {
    // Decrementa el valor del semáforo para obtener acceso exclusivo.
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void subir_semaforo(int sem_id) {
    // Incrementa el valor del semáforo para liberar el acceso.
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}

void manejador_sigterm(int sig) {
    // Libera recursos de memoria compartida y semáforos al recibir SIGTERM.
    shmdt(procesos);
    shmctl(shmid_procesos, IPC_RMID, NULL);
    shmdt(lote_procesos);
    shmctl(shmid_lote, IPC_RMID, NULL);
    semctl(id_semaforo, 0, IPC_RMID);
    semctl(id_semaforo_lotes, 0, IPC_RMID);
    printf("Recursos liberados. Terminando proceso.\n");
    exit(0);
}

void proceso_lector() {
    // Lee procesos de un archivo y los carga en la memoria compartida.
    FILE *archivo = fopen("procesos.txt", "r");
    if (!archivo) {
        perror("fopen");
        exit(1);
    }

    while (1) {
        bajar_semaforo(id_semaforo);  // Bloquea el acceso para modificar la lista de procesos.

        int num_procesos_leidos = 0;
        // Lee un número aleatorio de procesos del archivo.
        for (int i = 0; i < rand() % 20 + 1; i++) {
            if (feof(archivo)) break;
            Proceso proc;
            fscanf(archivo, "%d %d", &proc.pid, &proc.tiempo_ejecucion);
            proc.tiempo_restante = proc.tiempo_ejecucion;
            procesos[proc.pid] = proc;
            printf("Proceso %d agregado con tiempo de ejecución %d\n", proc.pid, proc.tiempo_ejecucion);
            num_procesos_leidos++;
        }

        if (num_procesos_leidos > 0) {
            (*lote_procesos)++;  // Incrementa el número de lotes en cola.
            subir_semaforo(id_semaforo_lotes);  // Notifica al despachador que hay nuevos lotes.
        }
        
        subir_semaforo(id_semaforo);
        sleep(rand() % 10 + 1);  // Pausa aleatoria antes de la próxima lectura.
    }
    fclose(archivo);
}

int calcular_quantum_dinamico() {
    // Calcula un quantum dinámico basado en el tiempo restante promedio de los procesos.
    int suma_tiempo = 0;
    int contador = 0;

    for (int i = 0; i < PROCESS; i++) {
        if (procesos[i].tiempo_restante > 0) {
            suma_tiempo += procesos[i].tiempo_restante;
            contador++;
        }
    }
    return (contador > 0) ? (suma_tiempo / contador) : 1;
}

void proceso_despachador() {
    // Despacha procesos según el quantum dinámico calculado.
    while (1) {
        bajar_semaforo(id_semaforo_lotes);  // Espera a que haya un nuevo lote disponible.
        bajar_semaforo(id_semaforo);  // Accede de forma exclusiva a la lista de procesos.

        int quantum_dinamico = calcular_quantum_dinamico();
        printf("Quantum dinámico calculado: %d segundos\n", quantum_dinamico);

        // Itera a través de los procesos y los ejecuta según su tiempo restante.
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

        (*lote_procesos)--;  // Reduce el número de lotes después de procesarlos.
        subir_semaforo(id_semaforo);
    }
}

int main() {
    // Configura memoria compartida y semáforos para sincronizar procesos.
    shmid_procesos = shmget(IPC_PRIVATE, sizeof(Proceso) * PROCESS, IPC_CREAT | 0666);
    procesos = (Proceso*) shmat(shmid_procesos, NULL, 0);

    shmid_lote = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    lote_procesos = (int*) shmat(shmid_lote, NULL, 0);
    *lote_procesos = 0;

    id_semaforo = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    id_semaforo_lotes = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    semctl(id_semaforo, 0, SETVAL, 1);
    semctl(id_semaforo_lotes, 0, SETVAL, 0);

    signal(SIGTERM, manejador_sigterm);  // Configura el manejador de señal para liberar recursos.

    // Crea procesos hijo y padre para leer y despachar los procesos.
    if (fork() == 0) {
        proceso_lector();
    } else {
        proceso_despachador();
    }

    return 0;
}
