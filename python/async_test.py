import asyncio

from contextlib import contextmanager


def async_test(f):
    def wrapper(*args, **kwargs):
        coro = asyncio.coroutine(f)
        future = coro(*args, **kwargs)
        loop = asyncio.get_event_loop()
        loop.run_until_complete(future)

    return wrapper


@contextmanager
def _get_event_loop():
    loop = asyncio.get_event_loop()
    if not loop.is_running():
        yield loop
    else:
        yield asyncio.new_event_loop()


def wait_for(coro):
    with _get_event_loop() as loop:
        return loop.run_until_complete(coro)
