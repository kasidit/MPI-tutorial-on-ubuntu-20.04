#include <stdio.h>
#include "mpi.h" 

#define RCC_INVOKE_MAX_FNAME_LEN 256  

#define RCC_INIT 	1
#define RCC_CMD 	2
#define RCC_CLIENT_FIN 	3
#define RCC_SERVER_FIN 	4

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

    MPI_Open_port(MPI_INFO_NULL, port_name); 
    printf("server available at %s\n", port_name); 

    while (rcc_state != RCC_SERVER_FIN) { 
        MPI_Comm_accept(port_name, MPI_INFO_NULL, 0, MPI_COMM_WORLD,  
                         &client); 
        rcc_state = RCC_CMD; 

        done = 0; 
        while (!done){

            MPI_Recv(fname, RCC_INVOKE_MAX_FNAME_LEN, MPI_CHAR,  
                      MPI_ANY_SOURCE, MPI_ANY_TAG, client, &status); 
            printf("receive %s\n", fname); 
            switch (status.MPI_TAG) { 
                case 0: MPI_Comm_free(&client); 
                        MPI_Close_port(port_name); 
                        MPI_Finalize(); 
                        return 0; 
                case 1: MPI_Comm_disconnect(&client); 
                        rcc_state = RCC_SERVER_FIN; 
                        printf("disconnect now\n"); 
                        done = 1; 
                        break; 
                case 2: printf("%s do something\n", fname); 
                default: 
                        /* Unexpected message type */ 
                        MPI_Abort(MPI_COMM_WORLD, 1); 
                } 
            } 
        } 
} 
