#include <stdlib.h> 

#include <sys/types.h> 
#include <sys/time.h> 

#ifdef HAVE_SYS_SELECT 
#include <sys/select.h> 
#endif 

int usleep (unsigned long usec) 
{ 
    struct timeval timeout; 
    timeout.tv_usec = usec % (unsigned long) 1000000; 
    timeout.tv_sec = usec / (unsigned long) 1000000; 
    select(0, NULL, NULL, NULL, &timeout); 
    
    return 0;
}
