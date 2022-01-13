extern int createServer(char server[50], char port[4], char localPort[5], char name[20], int bootstrapRequested);
extern int closeServer();
extern int sendData();
extern int readData();
extern int waitForTimeout();
extern void handleValueRefresh(char * resourceUri, int resourceUriLength, char * value, int valueLength);
extern void refreshObjects();

#define LWM2M_SNAP_CONTROL_OBJECT_ID      30000
#define LWM2M_SNAP_OBJECT_ID              30001
