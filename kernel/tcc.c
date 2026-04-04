#include "libtcc.h"
#include "kernel.h"

// Compile a C source string and return a function pointer to main()
void *tcc_compile(const char *source) {
    TCCState *s;
    void (*func)();

    s = tcc_new();
    if (!s) {
        print("tcc_new failed\n");
        return 0;
    }

    // Set output to memory (compile directly into RAM)
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

    // Compile the source string
    if (tcc_compile_string(s, source) == -1) {
        print("Compilation failed\n");
        tcc_delete(s);
        return 0;
    }

    // Relocate the code (resolve addresses, prepare for execution)
    if (tcc_relocate(s, TCC_RELOCATE_AUTO) < 0) {
        print("Relocation failed\n");
        tcc_delete(s);
        return 0;
    }

    // Get the entry point (the main function)
    func = tcc_get_symbol(s, "main");
    if (!func) {
        print("No main() function found\n");
        tcc_delete(s);
        return 0;
    }

    // We cannot delete the TCC state because it holds the compiled code.
    // For now, we leak it (memory leak). In a more advanced design, we'd keep
    // the state to free later, but for simplicity we'll accept the leak.
    // This is okay for a learning OS.

    return (void*)func;
}
