#include <stdlib.h>
#include <stdio.h>

#include "receiver.h"

ReceiverData createReceiver(int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm) {
    int dataTypeSize;
    MPI_Type_size(datatype, &dataTypeSize);

    ReceiverData data;
    data.buf = malloc(dataTypeSize*count);
    data.count = count;
    data.datatype = datatype;
    data.source = source;
    data.tag = tag;
    data.comm = comm;
    MPI_Irecv(data.buf, count, datatype, source, tag, comm, &data.req);

    return data;
}

void try_recv(ReceiverData *data, int *recvFlag, MPI_Status *recvStatus) {

    MPI_Test(&data->req, recvFlag, recvStatus);

    if(*recvFlag){
        MPI_Wait(&data->req, MPI_STATUS_IGNORE);
    
        MPI_Irecv(data->buf, data->count, data->datatype, data->source, data->tag, data->comm, &data->req);
    }
}

void try_to_get_message(ReceiverData *data, int *recvFlag, MPI_Status *recvStatus) {

    MPI_Test(&data->req, recvFlag, recvStatus);

    if(*recvFlag){
        MPI_Wait(&data->req, MPI_STATUS_IGNORE);
    }
}

void start_new_receive(ReceiverData *data) {
    MPI_Irecv(data->buf, data->count, data->datatype, data->source, data->tag, data->comm, &data->req);
}

void blocking_receive(ReceiverData *data, MPI_Status *recvStatus) {
    MPI_Wait(&data->req, recvStatus);
}

void destroyReceiver(ReceiverData *data) {
    MPI_Request_free(&data->req);
    free(data->buf);
}