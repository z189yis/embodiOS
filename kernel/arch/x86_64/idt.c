/* x86_64 Interrupt Descriptor Table */
#include <stdint.h>
#include <stddef.h>
#include "../../include/arch/x86_64/idt.h"
#include "../../include/arch/x86_64/interrupt.h"
#include "../../include/embodios/mm.h"
#include "../../include/embodios/console.h"

/* IDT entry structure */
struct idt_entry {
    uint16_t offset_low;    /* Offset bits 0-15 */
    uint16_t selector;      /* Code segment selector */
    uint8_t  ist;          /* Interrupt Stack Table */
    uint8_t  type_attr;    /* Type and attributes */
    uint16_t offset_mid;    /* Offset bits 16-31 */
    uint32_t offset_high;   /* Offset bits 32-63 */
    uint32_t zero;         /* Reserved */
} __attribute__((packed));

/* IDT pointer structure */
struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* IDT with 256 entries */
static struct idt_entry idt[256];
static struct idt_ptr idtp;

/* External interrupt stub table */
extern void* interrupt_stub_table[];

/* Set an IDT gate */
static void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t flags)
{
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].offset_mid = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].selector = selector;
    idt[num].ist = 0;
    idt[num].type_attr = flags;
    idt[num].zero = 0;
}

/* Initialize the IDT */
void idt_init(void)
{
    /* Set up IDT pointer */
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint64_t)&idt;

    /* Clear IDT - use simple loop instead of memset to avoid potential issues */
    for (int i = 0; i < 256; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].ist = 0;
        idt[i].type_attr = 0;
        idt[i].offset_mid = 0;
        idt[i].offset_high = 0;
        idt[i].zero = 0;
    }

    /* Set up exception handlers (0-31) - check if stub table is valid */
    if (interrupt_stub_table != NULL) {
        for (int i = 0; i < 32; i++) {
            if (interrupt_stub_table[i] != NULL) {
                idt_set_gate(i, (uint64_t)interrupt_stub_table[i], 0x08, 0x8E);
            }
        }

        /* Set up IRQ handlers (32-47) */
        for (int i = 32; i < 48; i++) {
            if (interrupt_stub_table[i] != NULL) {
                idt_set_gate(i, (uint64_t)interrupt_stub_table[i], 0x08, 0x8E);
            }
        }
    }

    /* Load IDT */
    __asm__ volatile("lidt %0" : : "m"(idtp));
}

/* Install a custom interrupt handler */
void idt_install_handler(uint8_t num, uint64_t handler)
{
    idt_set_gate(num, handler, 0x08, 0x8E);
}

/* C interrupt handler - called from assembly stub */
void interrupt_handler(struct interrupt_frame* frame)
{
    uint64_t vector = frame->int_no;

    /* CPU exceptions (0-31): fatal - report and halt */
    if (vector < 32) {
        static const char* const names[] = {
            "#DE", "#DB", "NMI", "#BP", "#OF", "#BR", "#UD", "#NM",
            "#DF", "#CSO", "#TS", "#NP", "#SS", "#GP", "#PF", "reserved",
            "#MF", "#AC", "#MC", "#XM", "#VE", "reserved", "reserved",
            "reserved", "reserved", "reserved", "reserved", "reserved",
            "reserved", "reserved", "#SX", "reserved"
        };
        console_printf("\n\n*** KERNEL EXCEPTION %s (vector %llu) ***\n",
                       names[vector], (unsigned long long)vector);
        console_printf("  RAX=%016llx  RBX=%016llx  RCX=%016llx  RDX=%016llx\n",
                       (unsigned long long)frame->rax, (unsigned long long)frame->rbx,
                       (unsigned long long)frame->rcx, (unsigned long long)frame->rdx);
        console_printf("  RIP=%016llx  RSP=%016llx  RFLAGS=%016llx\n",
                       (unsigned long long)frame->rip, (unsigned long long)frame->rsp,
                       (unsigned long long)frame->rflags);
        console_printf("  CS=%016llx  SS=%016llx  ERR=%016llx\n",
                       (unsigned long long)frame->cs, (unsigned long long)frame->ss,
                       (unsigned long long)frame->err_code);
        console_printf("  Kernel halted.\n");

        __asm__ volatile("cli");
        for (;;) { __asm__ volatile("hlt"); }
    }

    /* IRQs (32-47): acknowledge the 8259 and dispatch */
    if (vector >= 32 && vector < 48) {
        int irq = (int)(vector - 32);

        if (irq >= 8) {
            __asm__ volatile("outb %0, %1" :: "a"((uint8_t)0x20), "Nd"((uint16_t)0xA0));
        }
        __asm__ volatile("outb %0, %1" :: "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));

        if (vector == 32) {
            /* Timer interrupt: advances system ticks + scheduler tick */
            extern void timer_interrupt_handler(void);
            timer_interrupt_handler();
        }
    }
}
