#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

typedef struct {
    float f1; // 4 bytes
    char c1;  // 1 byte
    float f2;
    char c2;
} my_s_padd /*__attribute__((packed))*/;

typedef struct {
    float f1; // 4 bytes
    float f2;
    char c2;
    char c1; // 1 byte
} my_s_padd_des /*__attribute__((packed))*/;

typedef struct {
    char c2;
    char c1;  // 1 byte
    float f1; // 4 bytes
    float f2;
} my_s_padd_asc /*__attribute__((packed))*/;

typedef struct __attribute__((packed)) {
    float f1; // 4 bytes
    char c1;  // 1 byte
    float f2;
    char c2;
} my_s_pack;

#pragma pack(push, 1)
typedef struct {
    float f1; // 4 bytes
    float f2;
    char c2;
    char c1; // 1 byte
} my_s_pack_pragma;
#pragma pack(pop)

my_s_padd s_padd;
my_s_padd_des s_padd_des;
my_s_padd_asc *s_padd_asc;
my_s_pack s_pack;
my_s_pack *s_pack_ptr;
my_s_pack_pragma s_pack_pragma;


void struct_as_pointer_1(my_s_pack *p);
void struct_as_pointer_2(my_s_pack_pragma p);

int main(void) {

    puts("Starting RTOS");
    printf("\nSize of padded structure with element order not changed: %d bytes\n"
           "floats 0x%p 0x%p\n"
           "chars  0x%p 0x%p\n",
           sizeof(s_padd), &s_padd.f1, &s_padd.f2, &s_padd.c1, &s_padd.c2);

    printf("\nSize of padded structure with elements in ascending order; "
           "accessing structure elements using pointer: %d bytes\n"
           "floats 0x%p 0x%p\n"
           "chars  0x%p 0x%p\n",
           sizeof(*s_padd_asc), &s_padd_asc->f1, &s_padd_asc->f2, &s_padd_asc->c1, &s_padd_asc->c2);

    printf("\nSize of padded structure with elements in descending order"
           ": %d bytes\n"
           "floats 0x%p 0x%p\n"
           "chars  0x%p 0x%p\n",
           sizeof(s_padd_des), &s_padd_des.f1, &s_padd_des.f2, &s_padd_des.c1, &s_padd_des.c2);

    printf("\nSize of packed structure with element order not changed; "
           "using __attribute__((packed)): %d bytes\n"
           "floats 0x%p 0x%p\n"
           "chars  0x%p 0x%p\n",
           sizeof(s_pack), &s_pack.f1, &s_pack.f2, &s_pack.c1, &s_pack.c2);

    printf("\nSize of packed structure with element order not changed; "
           "using __attribute__((packed))and pointer: %d bytes\n"
           "floats 0x%p 0x%p\n"
           "chars  0x%p 0x%p\n",
           sizeof(*s_pack_ptr), &s_pack_ptr->f1, &s_pack_ptr->f2, &s_pack_ptr->c1, &s_pack_ptr->c2);

    printf("\nSize of packed structure with elements in descending order;"
           " using #pragma pack(push,1) and #pragma pack(pop): %d bytes\n"
           "floats 0x%p 0x%p\n"
           "chars  0x%p 0x%p\n",
           sizeof(s_pack_pragma), &s_pack_pragma.f1, &s_pack_pragma.f2, &s_pack_pragma.c1, &s_pack_pragma.c2);


    struct_as_pointer_1(&s_pack); // passing address of the structure.
    // struct_as_pointer_1(s_pack_ptr);//cannot pass pointer to the structure.
    struct_as_pointer_2(s_pack_pragma); // passing structure.

    vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

    return 0;
}

// Passing the structure and catching it using pointer.
void struct_as_pointer_1(my_s_pack *p) {
    p->f1 = 1.4;
    p->f2 = 5.4;
    p->c1 = 'c';
    p->c2 = 's';

    printf("\nValue of structure elements is\n"
           "floats f1 %f and f2 %f\n"
           "chars c1 %c and c2 %c\n",
           p->f1, p->f2, p->c1, p->c2);
}

// Zero out the struct
void struct_as_pointer_2(my_s_pack_pragma p) {
    memset(&p, 0, sizeof(p));//arguments: address or the structure/pointer(&p+2--> start from 3), data to be written, size of structure

    printf("\nValue of structure elements is\n"
           "floats f1 %.2f and f2 %.2f\n"
           "chars c1 %c and c2 %c\n",
           p.f1, p.f2, p.c1, p.c2);
}