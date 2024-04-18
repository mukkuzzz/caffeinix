#include <trap.h>
#include <spinlock.h>
#include <uart.h>
#include <debug.h>
#include <scheduler.h>
#include <printf.h>
#include <plic.h>

extern void kernel_vec(void);

static struct spinlock tick_lock;
/* For test */
volatile uint64 tick_count = 0;

static void tick_intr(void)
{
        spinlock_acquire(&tick_lock);

        tick_count ++;
        if(tick_count % 10 == 0) {
                printf("timer interrupt\n");
        }
        /* TODO: We should wakeup here */
        // wakeup
        spinlock_release(&tick_lock);
}

static int dev_intr(uint64 scause)
{
        int irq = 0;
        /* This is a supervisor external interrupt via PLIC */
        if((scause & 0x8000000000000000L) &&
           (scause & 0xff) == 9) {
                /* Get interrupt number from PLIC */
                irq = plic_claim();
                if(irq == UART0_IRQ) {

                } else if(irq == VIRTIO0_IRQ) {

                } else{
                        printf("Unexpected interrupt irq=%d\n");
                }
                if(irq) {
                        /* Clear the interrupt flag */
                        plic_complete(irq);
                }
                return 1;
        } else if(scause == 0x8000000000000001L) {
                /* Software interrupt */
                if(cpuid() == 0) {
                        tick_intr();
                } 
                 /* Clear the interrupt flag */
                sip_w(sip_r() &~ 2);
                return 2;
        }   
        return 0;
}

void kernel_trap(void)
{
        uint8 which_dev = 0;
        uint64 sepc = sepc_r();
        uint64 sstatus = sstatus_r();
        uint64 cause = scause_r();

        if(!(sstatus & SSTATUS_SPP)) 
                PANIC("[kernel_trap]It is not from supervisor mode");
        if(intr_status())
                PANIC("[kernel_trap]Interrupt enabled");

        /* Unknown device interrupt */
        if((which_dev = dev_intr(cause)) == 0) {
                printf("scause %p\n", cause);
                printf("sepc=%p stval=%p\n", sepc, stval_r());
                PANIC("kerneltrap");
        }

        sepc_w(sepc);
        sstatus_w(sstatus);
}

/* This function for first hart */
void trap_init_lock(void)
{
        spinlock_init(&tick_lock, "trap_tick");
}

/* This function for any hart */
void trap_init(void)
{
        stvec_w((uint64)kernel_vec);
}