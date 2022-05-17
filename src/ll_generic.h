#ifndef LL_GENERIC_H
#define LL_GENERIC_H

#define LL_APPEND(head, tail, node) {\
    if (head == NULL) {\
        head = node;\
        tail = node;\
    } else {\
        tail->next = node;\
        tail = node;\
    }\
}

#endif