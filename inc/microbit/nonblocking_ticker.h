
#include <stdint.h>

typedef void (*callback_funcptr)(void);


extern "C" {

void non_blocking_timer_take_over(callback_funcptr callback);

void non_blocking_timer_revert(callback_funcptr callback);

}