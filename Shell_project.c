/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell
	(then type ^D to exit program)

**/
#include <unistd.h>

#include "job_control.h"   // remember to compile with module job_control.c 

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

void cambio_dir(char *dir){
	if(dir==NULL){
		chdir(getenv("HOME"));
		return;
		
		prinft("Error al cambiar de directorio: falta directorio");
		return;
	}
	 
	if(chdir(dir)){
		perror("Error al cambiar de directorio");
	}
	
	 
}

job * lista;

void manejador(int sig){
	/* int i;
	 * job*tarea;
	 * int pid_wait;
	 * for(i=list_size;i>0;i--) {
	 * 		tarea=get_item_bypos(lista_tareas,i);
	 * 		pid_wait = waitpid(tarea->pgid,&status,WHNOHANG || WUNTRACED);
	 * 		if(pid_wait==tarea->pgid) {
	 * 			a la tarea le paso algo...
	 * }
	 * }
	 * 
	 */ 
	job * aux;
	int i=list_size(lista);
	int pid_wait;
	int status;
    enum status status_res;
    int info;
	block_SIGCHLD();
	while(i>0){
		aux=get_item_bypos(lista,i);
		pid_wait = waitpid(aux->pgid,&status,WNOHANG | WUNTRACED);
		if(pid_wait==aux->pgid) {
				status_res=analyze_status(status, &info);
				printf("Proceso %d, estado de terminacion %s, info: %d\n", pid_wait, status_strings[status_res], info);
				if ((status_res==SUSPENDED) | (status_res==EXITED)) {
					printf("Detenemos el proceso %d\n",pid_wait);
					delete_job(lista,aux);
				}
		}
		i--;
	}
	unblock_SIGCHLD();
}

// -----------------------------------------------------------------------
//                            MAIN
// -----------------------------------------------------------------------

int main(void)
{
    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background;             /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
    // probably useful variables:
    int pid_fork, pid_wait; /* pid for created and waited process */
    int status;             /* status returned by wait */
    enum status status_res; /* status processed by analyze_status() */
    int info;				/* info processed by analyze_status() */

    ignore_terminal_signals();  // TAREA1 el shell ignora las señales de terminal: ^C, ^Z, etc.
    lista = new_list("tareas");
	signal(SIGCHLD, manejador);
    while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
    {
        printf("migue@laboratorio $ ");
        fflush(stdout);
        get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */

		if(args[0]==NULL) continue;   // if empty command
		
		if(strcmp(args[0], "cd") == 0) {
			cambio_dir(args[1]);
			continue;
		}
		if(strcmp(args[0], "jobs") == 0) {
			if (empty_list(lista)) {
				printf("La lista está vacía\n");
				continue;
			} else {
				print_job_list(lista);
				continue;
			}
			continue;
		}
		if(strcmp(args[0],"fg")==0) {
			int posicion;
			if (empty_list(lista)) {
				printf("La lista está vacía\n");
				continue;
			}
			if (args[1]==NULL) {
				posicion=list_size(lista);
			} else {
				posicion=atoi(args[1]);
			}
			if (posicion<1 || posicion>list_size(lista)) {
				printf("El elemento debe ser un proceso en bg existente");
				continue;
			}
			job * tarea=get_item_bypos(lista, posicion);
			if (tarea->state==STOPPED) {
				killpg(tarea->pgid,SIGCONT);
			}
			tarea->state=FOREGROUND;
			printf("Proceso %d ejecutándose en primer plano\n", tarea->pgid);
			set_terminal(tarea->pgid);
			waitpid(tarea->pgid,&status,WUNTRACED);
			set_terminal(getpid());
			status_res=analyze_status(status,&info);
			if (status_res==SUSPENDED) {
				tarea->state=STOPPED;
			} else {
				block_SIGCHLD();
				delete_job(lista, tarea);
				unblock_SIGCHLD();
			}
			continue;
		}
		
		if(strcmp(args[0],"bg")==0) {
			int posicion;
			if (empty_list(lista)) {
				printf("La lista está vacía\n");
				continue;
			}
			if (args[1]==NULL) {
				posicion=list_size(lista);
			} else {
				posicion=atoi(args[1]);
			}
			if (posicion<1 || posicion>list_size(lista)) {
				printf("El elemento debe ser un proceso en bg existente");
				continue;
			}
			job * tarea=get_item_bypos(lista, posicion);
			if (tarea->state==STOPPED) {
				killpg(tarea->pgid,SIGCONT);
				tarea->state=BACKGROUND;
				printf("Proceso %d ejecutándose en segundo plano\n", tarea->pgid);
				continue;
			}
		}

        pid_fork= fork();

        if(pid_fork==0)
        {   //hijo
            restore_terminal_signals(); // TAREA1 el hijo restaura sus señales de terminal: ^C, ^Z, etc.
            pid_fork=getpid();  // ojo que pid_fork era 0
            new_process_group(pid_fork); //TAREA1 crea nuevo grupo de procesos independiente
            if(background==0) set_terminal(pid_fork);  //TAREA1 asigna el terminal si es fg
            execvp(args[0], args);
            perror("Error al ejecutar el comando");
            exit(-1);
        }
        else
        {
            // padre
            new_process_group(pid_fork);
            if(background==0)
            {   //fg
                set_terminal(pid_fork); //TAREA1 asigna el terminal si es fg
                pid_wait=waitpid(pid_fork, &status, WUNTRACED); //TAREA1 esperamos a que termine o suspenda
                status_res=analyze_status(status, &info); // TAREA1 analizamos terminación
                printf("Proceso %d, estado de terminacion %s, info: %d\n",
                       pid_wait, status_strings[status_res], info); //TAREA 1 imprimimos informe
                set_terminal(getpid()); //TAREA1 el shell recupera el terminal
                
                if(status_res==SUSPENDED) {
					block_SIGCHLD();
					add_job(lista, new_job(pid_fork, args[0], STOPPED));
					unblock_SIGCHLD();
				}
            }
            else
            {   //bg
                printf("Proceso %d, ejecutando %s en background\n", pid_fork, args[0]);
                
                add_job(lista, new_job(pid_fork, args[0], BACKGROUND));
                
                // TAREA1 no esperamos a que termine el proceso bg
            }

        } // end padre

    } // end while
}
