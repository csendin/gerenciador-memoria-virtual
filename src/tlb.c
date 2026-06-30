#include "tlb.h"
#include "config.h"

static tlb_entry_t tlb[TLB_SIZE];

/*
 * Índice da próxima posição a ser substituída.
 * Essa variável implementa FIFO no TLB.
 */
static int fifo_next = 0;

void tlb_init(void)
{
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].page = -1;
        tlb[i].frame = -1;
        tlb[i].valid = 0;
    }

    fifo_next = 0;
}

int tlb_lookup(int page)
{
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].page == page) {
            return tlb[i].frame;
        }
    }

    return -1;
}

void tlb_insert(int page, int frame)
{
    /*
     * 1. Se a pagina ja estiver no TLB, apenas atualiza o frame.
     */
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].page == page) {
            tlb[i].frame = frame;
            return;
        }
    }

    /*
     * 2. Se existir uma entrada invalida (slot livre), usa essa entrada.
     */
    for (int i = 0; i < TLB_SIZE; i++) {
        if (!tlb[i].valid) {
            tlb[i].page = page;
            tlb[i].frame = frame;
            tlb[i].valid = 1;
            return;
        }
    }

    /*
     * 3. TLB cheio: substitui a entrada mais antiga (FIFO).
     */
    tlb[fifo_next].page = page;
    tlb[fifo_next].frame = frame;
    tlb[fifo_next].valid = 1;

    fifo_next = (fifo_next + 1) % TLB_SIZE;
}

void tlb_remove(int page)
{
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].page == page) {
            tlb[i].page = -1;
            tlb[i].frame = -1;
            tlb[i].valid = 0;
            return;
        }
    }
}

void tlb_clear(void)
{
    tlb_init();
}
