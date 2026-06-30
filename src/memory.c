#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "config.h"
#include "page_table.h"
#include "tlb.h"

static signed char physical_memory[NUM_FRAMES][FRAME_SIZE];

/*
 * Indica qual página está carregada em cada quadro.
 * Valor -1 indica quadro livre.
 */
static int frame_to_page[NUM_FRAMES];

static FILE *backing = NULL;

void memory_init(FILE *backing_store)
{
    backing = backing_store;

    for (int i = 0; i < NUM_FRAMES; i++) {
        frame_to_page[i] = -1;

        for (int j = 0; j < FRAME_SIZE; j++) {
            physical_memory[i][j] = 0;
        }
    }
}

static int find_free_frame(void)
{
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frame_to_page[i] == -1) {
            return i;
        }
    }

    return -1;
}

int handle_page_fault(int page)
{
    /*
     * 1. Procurar quadro livre.
     * 2. Se não houver quadro livre, selecionar página vítima (LRU
     *    aproximado), invalidá-la na tabela de páginas e removê-la
     *    do TLB, liberando seu quadro para reutilização.
     * 3. Ler a página correta do BACKING_STORE.bin para o quadro.
     * 4. Atualizar frame_to_page e a tabela de páginas.
     * 5. Retornar o número do quadro.
     */

    int frame = find_free_frame();

    if (frame == -1) {
        int victim_page = select_victim_page();

        frame = page_table_get_frame(victim_page);

        page_table_invalidate(victim_page);
        tlb_remove(victim_page);
    }

    if (backing == NULL) {
        fprintf(stderr, "Erro interno: BACKING_STORE nao inicializado.\n");
        exit(1);
    }

    if (fseek(backing, (long) page * PAGE_SIZE, SEEK_SET) != 0) {
        fprintf(stderr, "Erro interno: falha ao buscar pagina %d no BACKING_STORE.\n", page);
        exit(1);
    }

    size_t bytes_read = fread(physical_memory[frame], sizeof(signed char), PAGE_SIZE, backing);

    if (bytes_read != PAGE_SIZE) {
        fprintf(stderr, "Erro interno: falha ao ler pagina %d do BACKING_STORE.\n", page);
        exit(1);
    }

    frame_to_page[frame] = page;
    page_table_update(page, frame);

    return frame;
}

int select_victim_page(void)
{
    int victim = -1;
    int min_counter = 256; /* maior que qualquer valor possivel de 8 bits (0-255) */

    for (int page = 0; page < PAGE_TABLE_SIZE; page++) {
        if (!page_table_is_valid(page)) {
            continue;
        }

        int counter = page_table_get_aging_counter(page);

        if (counter < min_counter) {
            min_counter = counter;
            victim = page;
        }
    }

    return victim;
}

signed char read_memory(int frame, int offset)
{
    if (frame < 0 || frame >= NUM_FRAMES || offset < 0 || offset >= FRAME_SIZE) {
        fprintf(stderr, "Erro interno: acesso invalido a memoria fisica (frame=%d, offset=%d).\n", frame, offset);
        exit(1);
    }

    return physical_memory[frame][offset];
}

int get_page_loaded_in_frame(int frame)
{
    if (frame < 0 || frame >= NUM_FRAMES) {
        return -1;
    }

    return frame_to_page[frame];
}
