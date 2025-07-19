#include "parent_ops.h"
#include <stdlib.h>

int main(int argc, char* argv[])
{
    (void)argc;  
    (void)argv; 

    init_parent();
    parent_main_loop();

    return EXIT_SUCCESS;
}
