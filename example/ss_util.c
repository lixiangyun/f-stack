
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define LOCK_PREFIX "lock;"

static inline long __cmpxchg(volatile void *ptr, long old, long new, int size)
{
    long prev;

    switch (size) {
    case 1:
        asm volatile(LOCK_PREFIX "cmpxchgb %b1,%2"
                 : "=a"(prev)
                 : "q"(new), "m"(*(char *)(ptr)), "0"(old)
                 : "memory");
        return prev;
    case 2:
        asm volatile(LOCK_PREFIX "cmpxchgw %w1,%2"
                 : "=a"(prev)
                 : "r"(new), "m"(*(short *)(ptr)), "0"(old)
                 : "memory");
        return prev;
    case 4:
        asm volatile(LOCK_PREFIX "cmpxchgl %k1,%2"
                 : "=a"(prev)
                 : "r"(new), "m"(*(int *)(ptr)), "0"(old)
                 : "memory");
        return prev;
    case 8:
        asm volatile(LOCK_PREFIX "cmpxchgq %1,%2"
                 : "=a"(prev)
                 : "r"(new), "m"(*(long *)(ptr)), "0"(old)
                 : "memory");
        return prev;
    }

    return old;
}

int ss_atomic_add(int * pAddr, int value)
{
    int oldvalue;
    int ret;

    do {
        oldvalue = *pAddr;
        ret = (int)__cmpxchg(pAddr, oldvalue, oldvalue + value, sizeof(int));
    } while ( ret != oldvalue );

    return 1;
}

int ss_atomic_sub(int * pAddr, int value)
{
    int oldvalue;
    int ret;

    do {
        oldvalue = *pAddr;
        ret = (int)__cmpxchg(pAddr, oldvalue, oldvalue - value, sizeof(int));
    } while ( ret != oldvalue );

    return 1;
}

int ss_atomic_inc(int * pAddr)
{
    int oldvalue;
    int ret;

    do {
        oldvalue = *pAddr;
        ret = (int)__cmpxchg(pAddr, oldvalue, oldvalue + 1, sizeof(int));
    } while ( ret != oldvalue );

    return 1;
}

int ss_atomic_dec(int * pAddr)
{
    int oldvalue;
    int ret;

    do {
        oldvalue = *pAddr;
        ret = (int)__cmpxchg(pAddr, oldvalue, oldvalue - 1, sizeof(int));
    } while ( ret != oldvalue );

    return 1;
}

int ss_atomic_cas(int * pAddr , int valueOld , int valueNew)
{
    int ret = (int)__cmpxchg(pAddr, valueOld, valueNew, sizeof(int));
    return ( ret == valueOld ) ? 1 : 0;
}


int ss_atomic64_add(long * pAddr, long value)
{
    long oldvalue;
    long ret;

    do {
        oldvalue = *pAddr;
        ret = __cmpxchg(pAddr, oldvalue, oldvalue + value, sizeof(long));
    } while ( ret != oldvalue );

    return 1;
}

int ss_atomic64_sub(long * pAddr, long value)
{
    long oldvalue;
    long ret;

    do {
        oldvalue = *pAddr;
        ret = __cmpxchg(pAddr, oldvalue, oldvalue - value, sizeof(long));
    } while ( ret != oldvalue );

    return 1;
}

int ss_atomic64_inc(long * pAddr)
{
    long oldvalue;
    long ret;

    do {
        oldvalue = *pAddr;
        ret = __cmpxchg(pAddr, oldvalue, oldvalue - 1, sizeof(long));
    } while ( ret != oldvalue );

    return 1;
}

int ss_atomic64_dec(long * pAddr)
{
    long oldvalue;
    long ret;

    do {
        oldvalue = *pAddr;
        ret = __cmpxchg(pAddr, oldvalue, oldvalue - 1, sizeof(long));
    } while ( ret != oldvalue );

    return 1;
}


int ss_atomic64_cas(long * pAddr, long valueold, long valuenew)
{
    long ret = __cmpxchg(pAddr, valueold, valuenew, sizeof(long));
    return ( ret == valueold ) ? 1 : 0;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


