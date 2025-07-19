#include "child_ops.h"
#include <stdlib.h>

int main(int argc, char* argv[])
{
    (void)argc; 
    (void)argv;   

    init_child();     
    run_child_process(); 

    return EXIT_SUCCESS; 
}
