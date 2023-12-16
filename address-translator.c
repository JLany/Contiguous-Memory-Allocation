#include <stdlib.h>
#include <stdio.h>

int pow(int a, int b) {
    if (b == 0)
        return 1;

    int temp = pow(a, b / 2);
    if (b % 2 == 0)
        return temp * temp;
    else
        return a * temp * temp;
}

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        printf("Invalid arguments.\nTerminating...\n");
        exit(-1);
    }

    int address = atoi(argv[1]);

    int page = address / pow(2, 12);
    int offset = address - (page * pow(2, 12));

    printf("The address %d contains:\n", address);
    printf("page number = %d\n", page);
    printf("offset = %d\n", offset);

    return 0;
}


