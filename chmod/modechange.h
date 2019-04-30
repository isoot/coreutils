#define MODE_X_IF_ANY_X 01

#define MODE_COPY_EXISTING 02

struct mode_change
{
    char op;
    char flags;
    unsigned short affected;
    unsigned short value;
    struct mode_change *next;
};

#define MODE_MASK_EQUALS 1
#define MODE_MASK_PLUS 2
#define MODE_MASK_MINUS 4

#define MODE_INVALID (struct mode_change *)0
#define MODE_MEMORY_EXHAUSTED (struct mode_change *)1

struct mode_change *mode_compile(register char *mode_string, unsigned int masked_ops);
unsigned short mode_adjust(unsigned oldmode, register struct mode_change *changes);
void mode_free(struct mode_change *changes);
