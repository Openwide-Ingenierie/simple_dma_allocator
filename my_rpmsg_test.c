#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define EXAMPLE_DMA_SIZE 4096

int main(int argc, char * argv[]){
    FILE * f_size = fopen("/sys/kernel/my_dma_allocator/dma_size", "r+");
    if (!f_size) {
        perror("device dma open size failed.");
        return -1;
    }

    FILE * f_phys = fopen("/sys/kernel/my_dma_allocator/addr_phys", "r");
    if (!f_phys) {
        perror("device dma open phys failed.");
        return -1;
    }

    FILE * f_rpmsg = fopen("/dev/ttyRPMSG30", "w");
    if (f_rpmsg < 0) {
        perror("device rpmsg open failed.");
        return -1;
    }

    printf("Found dma allocator device\n");

    size_t size = EXAMPLE_DMA_SIZE;

    fprintf(f_size, "%lu\n", size);
    fflush(f_size);

    unsigned long addr_phys = 0;

    do {
        fscanf(f_phys, "%lx", &addr_phys);
        rewind(f_phys);
    } while (addr_phys == 0);

    printf("Allocation done\n");

    printf("Found physical adress : %lx\n", addr_phys);

    fprintf(f_rpmsg, "%lx %lu", addr_phys, size);
    fflush(f_rpmsg);

    printf("Write done ! \n Waiting for 10s ...\n");

    sleep(10);

    fprintf(f_size, "0\n");
    fflush(f_size);

    do {
        fscanf(f_phys, "%lx", &addr_phys);
        rewind(f_phys);
    } while (addr_phys != 0);

    printf("DONE\n");

    fclose(f_size);
    fclose(f_phys);

    return 0;
}