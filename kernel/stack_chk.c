void __stack_chk_fail(void) {
    while(1);   // kernel panic
}
