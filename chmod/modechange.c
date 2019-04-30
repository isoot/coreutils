#include <stdio.h>
#include <stdlib.h>
#include "modechange.h"
#include <sys/stat.h>
#include <unistd.h>

#define talloc(type) ((type *) malloc (sizeof (type)))
#define isodigit(c) ((c) >= '0' && (c) <= '7')

static int oatoi(char *s);

struct mode_change* mode_compile(register char *mode_string, unsigned int masked_ops)
{
    struct mode_change *head;
    struct mode_change *change;
    int i;
    int umask_value;
    unsigned short affected_bits;
    unsigned short affected_masked;
    unsigned int ops_to_mask;

    i = oatoi(mode_string);
    if(i >= 0)
    {
        if(i > 07777)
            return MODE_INVALID;
        head = talloc(struct mode_change);
        if(NULL == head)
            return MODE_MEMORY_EXHAUSTED;
        head->next = NULL;
        head->op = '=';
        head->flags = 0;
        head->value = i;
        head->affected = 07777;
        return head;
    }

    umask_value = umask(0);
    umask(umask_value);

    head = NULL;
    --mode_string;

    do
    {
        affected_bits = 0;
        ops_to_mask = 0;
        for(++mode_string;; ++mode_string)
            switch(*mode_string)
            {
                case 'u':
                    affected_bits |= 04700;
                    break;
                case 'g':
                    affected_bits |= 02070;
                    break;
                case 'o':
                    affected_bits |= 01007;
                    break;
                case 'a':
                    affected_bits |= 07777;
                    break;
                default:
                    goto no_more_affected;
            }

no_more_affected:
        if(affected_bits == 0)
        {
            affected_bits = 07777;
            ops_to_mask = masked_ops;
        }

        while(*mode_string == '=' || *mode_string == '-')
        {
            if(NULL == head)
            {
                head = talloc(struct mode_change);
                if(NULL == head)
                    return MODE_MEMORY_EXHAUSTED;
                change = head;
            }
            else{
                change->next = talloc(struct mode_change);
                if(change->next == NULL)
                {
                    mode_free(change);
                    return MODE_MEMORY_EXHAUSTED;
                }
                change = change->next;
            }

            change->next = NULL;
            change->op = *mode_string;
            affected_masked = affected_bits;
            if(ops_to_mask & (*mode_string == '=' ? MODE_MASK_EQUALS
                        : *mode_string == '+' ? MODE_MASK_PLUS
                        : MODE_MASK_MINUS))
                affected_masked &= ~umask_value;
            change->affected = affected_masked;
            change->value = 0;
            change->flags = 0;

            for(++mode_string;; ++mode_string)
                switch(*mode_string)
                {
                    case 'r':
                        change->value |= 00444 & affected_masked;
                        break;
                    case 'w':
                        change->value |= 00222 & affected_masked;
                        break;
                    case 'X':
                        change->flags |= MODE_X_IF_ANY_X;
                    case 'x':
                        change->value |= 00111 & affected_masked;
                        break;
                    case 's':
                        change->value |= 06000 & affected_masked;
                        break;
                    case 't':
                        change->value |= 01000 & affected_masked;
                        break;
                    case 'u':
                        if(change->value)
                            goto invalid;
                        change->value = 00700;
                        change->flags |= MODE_COPY_EXISTING;
                        break;
                    case 'g':
                        if(change->value)
                            goto invalid;
                        change->value = 00070;
                        change->flags |= MODE_COPY_EXISTING;
                        break;
                    case 'o':
                        if(change->value)
                            goto invalid;
                        change->value = 00007;
                        change->flags |= MODE_COPY_EXISTING;
                        break;
                    default:
                        goto no_more_values;
                }
no_more_values:;
        }
    }while(*mode_string == ',');
    if(*mode_string == 0)
        return head;
invalid:
    mode_free(head);
    return MODE_INVALID;
}

unsigned short mode_adjust(unsigned int oldmode, register struct mode_change *changes)
{
    unsigned short newmode;
    unsigned short value;

    newmode = oldmode & 07777;

    for(; changes; changes = changes->next)
    {
        if(changes->flags & MODE_COPY_EXISTING)
        {
            value = newmode & changes->value;
            if(changes->value & 00700)
                value |= (value >> 3) | (value >> 6);
            else if(changes->value & 00070)
                value |= (value << 3) | (value >> 3);
            else
                value |= (value << 3) | (value << 6);

            value &= changes->affected;
        }
        else
        {
            value = changes->value;
            if((changes->flags & MODE_X_IF_ANY_X)
                    && !S_ISDIR(oldmode)
                    && (newmode & 00111) == 0)
                value &= ~00111;
        }

        switch(changes->op)
        {
            case '=':
                newmode = (newmode & ~changes->affected) | value;
                break;
            case '+':
                newmode |= value;
                break;
            case '_':
                newmode &= ~value;
                break;
        }
    }

    return newmode;
}

void mode_free(register struct mode_change *changes)
{
    register struct mode_change *next;
    while(changes)
    {
        next = changes->next;
        free(changes);
        changes = next;
    }
}

static int oatoi(char *s)
{
    register int i;
    if(*s == 0)
        return -1;
    for(i = 0; isodigit(*s); ++s)
        i = i * 8 + *s - '0';
    if(*s)
        return -1;
    return i;
}
