#ifndef RECEIVER_H_
#define RECEIVER_H_

#include <mpi.h>

typedef struct receiverData {
    void *buf;
    int count;
    MPI_Datatype datatype;
    int source;
    int tag;
    MPI_Comm comm;
    MPI_Request req;
} ReceiverData;

ReceiverData createReceiver(int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm);
void try_recv(ReceiverData *data, int *recvFlag, MPI_Status *recvStatus);
void try_to_get_message(ReceiverData *data, int *recvFlag, MPI_Status *recvStatus);
void start_new_receive(ReceiverData *data);
void destroyReceiver(ReceiverData *data);

#endif