
#include "MicroBit.h"

extern "C" {

#include "lib/utils/async.h"
#include "py/nlr.h"

// Forward declarations
static void async_attach(async_t *async);
static void async_detach(async_t *async);

void async_start(async_t *async, mp_obj_t iterator, void *state, async_callback_func_ptr callback, async_callback_func_ptr exit, int initial_delay_ms, bool wait) {
    async->iterator = iterator;
    async->state = state;
    async->callback = callback;
    async->exit = exit;
    async->async_exception = NULL;
    async->ticks = 0;
    async->wakeup = initial_delay_ms;
    async->done = false;
    async->async = !wait;
    async_attach(async);
    if (wait) {
        while (!async->done) {
            __WFI();
        }
        if (async->async_exception) {
            mp_obj_t ex = async->async_exception;
            async->async_exception = NULL;
            nlr_raise(ex);
        }
    }
}

mp_obj_t async_get_next(async_t *async) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t obj = mp_iternext(async->iterator);
        if (obj != MP_OBJ_STOP_ITERATION) {
            return obj;
        }
    } else {
        async->async_exception = (mp_obj_t)nlr.ret_val;
    }
    return NULL;
}

void async_stop(async_t *async) {
    async->exit(async, async->state);
    async->iterator = NULL;
    async->state = NULL;
    async->callback = NULL;
    async->done = true;
    if (async->async) {
        async->async_exception = NULL;
    }
}

bool async_is_running(async_t *async) {
    return !async->done;
}

static void do_tick(async_t *async) {
    if (async->done) {
        async_detach(async);
        return;
    }
    async->ticks += FIBER_TICK_PERIOD_MS;
    if (async->ticks < async->wakeup) {
        return;
    }
    async->ticks = 0;
    int32_t delay_ms;
    do {
        delay_ms = async->callback(async, async->state);
    } while (delay_ms <= 0);
    async->wakeup = delay_ms;
    if (MP_STATE_VM(mp_pending_exception) != MP_OBJ_NULL) {
        async->async_exception = MP_STATE_VM(mp_pending_exception);
        async_stop(async);
    }
}

#define MAX_ASYNC_TICKER_COUNT 4

static async_t *asyncs[MAX_ASYNC_TICKER_COUNT];

static void async_attach(async_t *async) {
    for (int i = 0; i < MAX_ASYNC_TICKER_COUNT; i++) {
        if (asyncs[i] == NULL) {
            asyncs[i] = async;
        }
    }
}

static void async_detach(async_t *async) {
    for (int i = 0; i < MAX_ASYNC_TICKER_COUNT; i++) {
        if (asyncs[i] == async) {
            asyncs[i] = NULL;
        }
    }
}

void async_tick(void) {
    for (int i = 0; i < MAX_ASYNC_TICKER_COUNT; i++) {
        if (asyncs[i] != NULL) {
            do_tick(asyncs[i]);
        }
    }
}

}
