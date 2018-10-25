
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

struct ss_list
{
    struct ss_list * next;
    struct ss_list * prev;
};

#define SS_LIST_INIT(head) \
    (head)->next = (head); \
    (head)->prev = (head);

#define SS_LIST_EMPTY(head)\
    ((head)->next == (head))

#define SS_LIST_AFTER(item,where) \
    (item)->next = (where)->next; \
    (item)->prev = (where); \
    (where)->next->prev = (item); \
    (where)->next = (item);

#define SS_LIST_BEFORE(item,where) \
    (item)->prev = (where)->prev; \
    (item)->next = (where); \
    (where)->prev->next = (item); \
    (where)->prev = (item);

#define SS_LIST_REMOVE(item) \
    (item)->next->prev = (item)->prev; \
    (item)->prev->next = (item)->next;

#define SS_LIST_ENTRY(item,struct,stNode) \
    (struct *)((char *)item - (char *)(&((struct *)0)->stNode))

#define SS_LIST_FOR(pstItem,pstHead) \
    for( (pstItem) = (pstHead)->next; (pstItem) != (pstHead) ; (pstItem) = (pstItem)->next )



int ss_atomic_add(int * pAddr, int value);
int ss_atomic_sub(int * pAddr, int value);
int ss_atomic_inc(int * pAddr);
int ss_atomic_dec(int * pAddr);
int ss_atomic_cas(int * pAddr , int valueOld , int valueNew);

int ss_atomic64_add(long * pAddr, long value);
int ss_atomic64_sub(long * pAddr, long value);
int ss_atomic64_inc(long * pAddr);
int ss_atomic64_dec(long * pAddr);
int ss_atomic64_cas(long * pAddr, long valueold, long valuenew);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


