int socket (int ,int ,int );
int connect (int, struct sockaddr *, int);
int bind (int, struct sockaddr *, int);
int listen( int, int);
int accept (int, struct sockaddr * , int *);
int send( int, char *, int, int);
int recv( int, char *, int, int);
