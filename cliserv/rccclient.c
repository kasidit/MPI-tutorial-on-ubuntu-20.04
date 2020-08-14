#include <stdio.h>
#include <string.h>
#include "mpi.h" 

#define RCC_INVOKE_MAX_FNAME_LEN 256  

int main( int argc, char **argv ) 
{ 
    MPI_Comm server; 
    char port_name[MPI_MAX_PORT_NAME];
    char fname[RCC_INVOKE_MAX_FNAME_LEN];
    int done, tag;

    MPI_Init( &argc, &argv ); 
    strcpy( port_name, argv[1] );/* assume server's name is cmd-line arg */ 
 
    MPI_Comm_connect( port_name, MPI_INFO_NULL, 0, MPI_COMM_WORLD,  
                      &server ); 
 
    done = 0; 
    while (!done) { 
        strcpy(fname, "dummy"); 
        tag = 2; /* Action to perform */ 
        MPI_Send(fname, sizeof(fname), MPI_CHAR, 0, tag, server ); 
        tag = 1;
        MPI_Send(fname, sizeof(fname), MPI_CHAR, 0, tag, server ); 
        done = 1; 
    } 
    MPI_Comm_disconnect( &server ); 


    MPI_Comm_connect( port_name, MPI_INFO_NULL, 0, MPI_COMM_WORLD,  
                      &server ); 
 
    done = 0; 
    while (!done) { 
        strcpy(fname, "dummy"); 
        tag = 2; /* Action to perform */ 
        MPI_Send(fname, sizeof(fname), MPI_CHAR, 0, tag, server ); 
        tag = 0;
        MPI_Send(fname, sizeof(fname), MPI_CHAR, 0, tag, server ); 
        done = 1; 
    } 
    MPI_Comm_disconnect( &server ); 

    MPI_Finalize(); 
    return 0; 
} 
