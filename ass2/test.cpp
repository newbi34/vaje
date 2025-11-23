#include <stdio.h> 
#include <signal.h> 
#include <stdlib.h>
#include <time.h>
  
// Handler for SIGINT, triggered by 
// Ctrl-C at the keyboard 
void singalHandler(int sig)  { 
    printf("Caught signal %d\n", sig);
    return;
} 
  
int main()  { 
    signal(SIGINT, singalHandler); 
    int counter = 0;
    while (1){
        sleep(1);
        printf("%d: Hello World!\n", counter);
        counter++;
    }
    return 0; 
}