#ifndef PRINTER_H_
#define PRINTER_H_

// used style codes
#define STYLE_NEW "92"
#define STYLE_TASK "94"
#define STYLE_SLIP "35"
#define STYLE_WASH "33"
#define STYLE_CUSTOM "2;92"

// operation literals
#define P_BROADCASTING "broadcasting"
#define P_REQUESTING "requesting"
#define P_OBTAINING "obtained"
#define P_RELEASING "releasing"
// signal names
#define P_REQ "REQ"
#define P_ACK "ACK"
#define P_REL "REL"
// signal content
#define P_TASK "TASK"
#define P_SLIP "SLIP"
#define P_WASH "WASH"

void printNewTask(char *COLOR, int pid, int lamport, char *OPname, int libID, int taskID, int *targets, int arrSize);
void printTask(char *COLOR, int pid, int lamport, char *OPname, int libID, int taskID);
void printSlip(char *COLOR, int pid, int lamport, char *OPname, int reqLamport, int reqPid);
void printWash(char *COLOR, int pid, int lamport, char *OPname, int reqLamport, int reqPid);
void styledPrintln(char *style, int pid, int lamport, char* message);

void printCommSend(int pid, int lamport, char *TYPE, char *OPname, int target);
void printCommRecv(int pid, int lamport, char *TYPE, char *OPname, int source);
void printCustomDebug(int pid, int lamport, char *message);

#endif