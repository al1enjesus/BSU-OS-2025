#include <stdatomic.h>

int atomic_increment(int *ptr) {
    return atomic_fetch_add_explicit(ptr, 1, memory_order_seq_cst);
}
