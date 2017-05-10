#define STACK_CHECK_GUARD 0x726973627a4073b1
uint64_t __stack_chk_guard = STACK_CHECK_GUARD;

void __stack_chk_fail(void){
	__assert(__func__, __FILE__, __LINE__, "stack smash detected");
}

