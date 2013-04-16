/* exception support */
struct Exception {
	jmp_buf jmpbuf;
	int val;
	struct Exception *next;
} *__exception;

#define TRY do { struct Exception e; e.next = __exception; __exception = &e; \
	e.val = setjmp(e.jmpbuf); switch (e.val) { case 0:
#define RESCUE(x) break; case x:
#define ELSE break; default:
#define ENSURE } {
#define ENDTRY } __exception = e.next; } while(0)
#define THROW(x) longjmp(__exception->jmpbuf, x)
#define RAISE do { int x = __exception->val; __exception = __exception->next; \
	THROW(x); } while (0)

