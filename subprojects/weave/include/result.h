#pragma once
#include "types.h"

#define RESULT_TYPE(OK, ERR)                                                                   \
    struct {                                                                                   \
        bool is_ok;                                                                            \
        union {                                                                                \
            OK ok;                                                                             \
            ERR err;                                                                           \
        };                                                                                     \
    }
