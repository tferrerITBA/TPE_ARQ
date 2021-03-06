#include <stdint.h>
#include <idtLoader.h>
#include <defs.h>
#include <interrupts.h>
#include "videoMode.h"
#include "RTC.h"

char * read(uint64_t rbx, uint64_t rcx, uint64_t rdx);
char * write(uint64_t rbx, uint64_t rcx, uint64_t rdx);
char * time(uint64_t rbx, uint64_t rcx, uint64_t rdx);
char * pixel(uint64_t rbx, uint64_t rcx, uint64_t rdx);
char * colors(uint64_t rbx, uint64_t rcx, uint64_t rdx);
extern void _int80Handler();
extern void _exception6Handler();

#pragma pack(push)		/* Push de la alineación actual */
#pragma pack (1) 		/* Alinear las siguiente estructuras a 1 byte */

/* Descriptor de interrupcion */
typedef struct {
  uint16_t offset_l, selector;
  uint8_t cero, access;
  uint16_t offset_m;
  uint32_t offset_h, other_cero;
} DESCR_INT;

#pragma pack(pop)		/* Reestablece la alinceación actual */

typedef char * (*sysCalls)(uint64_t, uint64_t, uint64_t);
sysCalls sc[] = {0, 0, 0, &read, &write, &pixel, &colors, 0, 0, 0, 0, 0, 0, &time};

DESCR_INT * idt = (DESCR_INT *) 0;	// IDT de 255 entradas

static void setup_IDT_entry (int index, uint64_t offset);

void load_idt() {

  setup_IDT_entry (0x20, (uint64_t)&_irq00Handler);
  setup_IDT_entry (0x00, (uint64_t)&_exception0Handler);
  setup_IDT_entry (0x06, (uint64_t)&_exception6Handler);
  setup_IDT_entry (0x21, (uint64_t)&_irq01Handler);
  setup_IDT_entry (0x80, (uint64_t)&_int80Handler);

	//Solo interrupcion timer tick  y teclado habilitadas
	picMasterMask(0xFC); // 1111 1100
	picSlaveMask(0xFF);
        
	_sti();
}

static void setup_IDT_entry (int index, uint64_t offset) {
  idt[index].selector = 0x08;
  idt[index].offset_l = offset & 0xFFFF;
  idt[index].offset_m = (offset >> 16) & 0xFFFF;
  idt[index].offset_h = (offset >> 32) & 0xFFFFFFFF;
  idt[index].access = ACS_INT;
  idt[index].cero = 0;
  idt[index].other_cero = (uint64_t) 0;
}

void int80Dispatcher(uint64_t rax, uint64_t rbx, uint64_t rcx, uint64_t rdx) {
  sc[rax](rbx, rcx, rdx);
}

//En rcx se pone un char[] con al menos 9 espacios
char * time(uint64_t rbx, uint64_t rcx, uint64_t rdx) {
  timeToString(rcx);
  return rcx;
}

//rbx es 1 para putnstring, 0 para clear; rcx es un char * y rdx es la longitud
char * write(uint64_t rbx, uint64_t rcx, uint64_t rdx) {
  if(rbx == 1) {
    putnString(rcx, rdx);
    return 0x1;
  }
  if(rbx == 0) {
    clearScreen();
    return 0x2;
  }
  return 0x0;
}

//rbx es 1 para stdin, rcx es el char * destino y rdx la longitud a leer
char * read(uint64_t rbx, uint64_t rcx, uint64_t rdx) {
  if(rbx == 1) {
    return readBuffer(rbx, rcx, rdx);
  }
  return 0x0;
}

//rcx es xCoord, rdx es yCoord
char * pixel(uint64_t rbx, uint64_t rcx, uint64_t rdx) {
  putPixel(rcx, rdx);
  return 0x0;
}

//rbx == 1 --> char; rbx == 0 --> fondo; rcx array con colores B G R
char * colors(uint64_t rbx, uint64_t rcx, uint64_t rdx) {
  uint8_t * cols = rcx;
  if(rbx == 0)
    setBackgroundColors(cols[0], cols[1], cols[2]);
  else if(rbx == 1)
    setCharColors(cols[0], cols[1], cols[2]);
  return 0x0;
}