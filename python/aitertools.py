import asyncio

from typing import (
    Any,
    AsyncGenerator,
    AsyncIterable,
    AsyncIterator,
    Callable,
    Iterable,
    List,
    Tuple,
    TypeVar,
    Union,
)



T = TypeVar("T")
AnyIterable = Union[Iterable[T], AsyncIterable[T]]


def iterate(itr: AnyIterable) -> AsyncIterator[T]:
    """
    Convert a [non-async] iterator to async generator

    Example:

        async for x in iterate(range(10)):
            ...
    """
    if isinstance(itr, AsyncIterator):
        return itr

    async def gen():
        for i in itr:
            yield i

    return gen()


async def islice(gen: AsyncGenerator, start: int, stop: int) -> Any:
    for _ in range(start):
        await gen.__anext__()  # noqa
    for _ in range(start, stop):
        yield await gen.__anext__()  # noqa


async def enumerate(gen: AsyncGenerator, start: int = 0) -> Any:
    n = start
    async for i in gen:
        yield n, i
        n += 1


async def tee(agen: AsyncIterable[T], count: int = 2) -> List[AsyncIterable[T]]:
    sentinel = object()
    queues = [asyncio.Queue() for _ in range(count)]

    async def fill_queues():
        try:
            async for item in agen:
                for queue in queues:
                    await queue.put(item)  # noqa
        finally:
            for queue in queues:
                await queue.put(sentinel)  # noqa

    async def gen(id):
        while True:
            item = await queues[id].get()  # noqa
            if item is sentinel:
                break
            yield item

    # Schedule async work on filling queues
    # As the async generators below try to consume the queue
    # these tasks will be scheduled by the asyncio scheduler
    # to make sure that we don't eagerly queue items
    asyncio.ensure_future(fill_queues())

    gens = [gen(i) for i in range(count)]
    return gens


async def zip(*gens: AsyncGenerator) -> AsyncIterable[Tuple]:
    while True:
        try:
            yield tuple([await gen.asend(None) for gen in gens])  # noqa
        except (StopIteration, StopAsyncIteration, GeneratorExit):
            break


_builtin_map = map


async def map(afunc: Callable, gen: AnyIterable, batch_size: int = 0) -> AsyncGenerator:
    """
    batch_size, if non zero controls how many items are awaited in parallel, when afunc is async
    """
    if isinstance(gen, AsyncGenerator):
        if asyncio.iscoroutinefunction(afunc):
            async for i in _async_map(afunc, gen, batch_size):
                yield i
        else:
            async for i in _sync_map(afunc, gen):
                yield i
    else:
        if asyncio.iscoroutinefunction(afunc):
            async for i in _async_map(afunc, iterate(gen), batch_size):
                yield i
        else:
            for i in _builtin_map(afunc, gen):
                yield i


async def _async_map(
    afunc: Callable, gen: AsyncGenerator, batch_size: int = 0
) -> AsyncGenerator:
    if not batch_size:
        async for i in gen:
            yield await afunc(i)
    else:
        batch = []
        async for i in gen:
            batch.append(i)

            if len(batch) >= batch_size:
                results = await asyncio.gather(*[afunc(i) for i in batch])
                for i in results:
                    yield i
                batch = []
        # Handle the tail
        if batch:
            results = await asyncio.gather(*[afunc(i) for i in batch])
            for i in results:
                yield i


async def _sync_map(afunc: Callable, gen: AsyncGenerator) -> AsyncGenerator:
    async for i in gen:
        yield afunc(i)
