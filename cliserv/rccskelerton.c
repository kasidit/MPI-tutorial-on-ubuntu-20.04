#include <stdio.h>
#include "mpi.h" 

#define RCC_INVOKE_MAX_FNAME_LEN 256  

// RCC state 
#define RCC_INIT 	1
#define RCC_WAIT_CMD 	2
#define RCC_RETURN_CMD 	3
#define RCC_CLIENT_FIN 	4
#define RCC_SERVER_FIN 	5

// MPI tag state
#define EMPI_SERV_TERM_TAG		0
#define EMPI_CONNECTION_TERM_TAG	1

// User-defined function state
#define EMPI_FUNC1_TAG			2

int main(int argc, char *argv[]) 
{ 
    MPI_Comm client; 
    MPI_Status status; 
    char port_name[MPI_MAX_PORT_NAME]; 
    char fname[RCC_INVOKE_MAX_FNAME_LEN]; 
    int    done, size, rcc_state; 
 
    rcc_state = RCC_INIT; 

    MPI_Init(&argc, &argv); 
    MPI_Comm_size(MPI_COMM_WORLD, &size); 
    if (size != 1)  
       printf("server too big\n"); 

    printf("max portname size = %d\n", MPI_MAX_PORT_NAME); 

    MPI_Open_port(MPI_INFO_NULL, port_name); 
    printf("server available at %s\n", port_name); 

    while (rcc_state != RCC_SERVER_FIN) { 
        MPI_Comm_accept(port_name, MPI_INFO_NULL, 0, MPI_COMM_WORLD,  
                         &client); 
        rcc_state = RCC_WAIT_CMD; 

        done = 0; 
        while (!done){

            MPI_Recv(fname, RCC_INVOKE_MAX_FNAME_LEN, MPI_CHAR,  
                      MPI_ANY_SOURCE, MPI_ANY_TAG, client, &status); 
            printf("receive %s\n", fname); 

            switch (status.MPI_TAG) { 
                case EMPI_SERV_TERM_TAG: 
			MPI_Comm_free(&client); 
                        MPI_Close_port(port_name); 
                        MPI_Finalize(); 
                        return 0; 
                case EMPI_CONNECTION_TERM_TAG:
			MPI_Comm_disconnect(&client); 
                        printf("disconnect now\n"); 
                        done = 1; 
                        break; 
                case EMPI_FUNC1_TAG:
			printf("%s do something\n", fname); 
			break;
                default: 
                        /* Unexpected message type */ 
                        MPI_Abort(MPI_COMM_WORLD, 1); 
			break;
                } 
            } 
        } 
} 
