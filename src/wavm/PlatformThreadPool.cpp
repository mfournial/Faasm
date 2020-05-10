#include "PlatformThreadPool.h"

#include <wavm/openmp/ThreadState.h>
#include <wavm/openmp/openmp.h>
#include <wavm/WAVMWasmModule.h>
#include <util/logging.h>

using namespace util;

namespace wasm {
    using namespace openmp;

//    I64 ompThreadEntryFunc(void *threadArgsPtr);

    I64 workerEntryFunc(void *_args) {
        auto args = reinterpret_cast<WorkerArgs *>(_args);
        U32 stackTop = args->stackTop;
        PlatformThreadPool *pool = args->pool;
        size_t workerIdx = args->workerIdx;
        delete args;
        std::uint32_t runNumber = 0;

        for (;;) {

            {
                UniqueLock lock(pool->mutex);
                pool->condition.wait(lock, [&pool, runNumber] { return pool->stop || pool->runNumber > runNumber; });
                if (pool->stop) {
                    // We're done folks
                    return 0;
                }
            }

            std::promise<I64> &promise = pool->promises[workerIdx];
            LocalThreadArgs threadArgs = pool->workerArgs[workerIdx];

            // Set up TLS
            setTLS(threadArgs.tid, threadArgs.level);
            setExecutingModule(threadArgs.parentModule);
            setExecutingCall(threadArgs.parentCall);

            // Use preallocated stack
            threadArgs.spec.stackTop = stackTop;

            // Run
            promise.set_value(threadArgs.parentModule->executeThreadLocally(threadArgs.spec));

            // Wait for next job to come in
            runNumber++;
        }
    }

    PlatformThreadPool::PlatformThreadPool(size_t numThreads, WAVMWasmModule *module) {
        promises.reserve(numThreads);
        for (size_t i = 0; i < numThreads; i++) {
            // Set up workers arguments including pre-allocating a stack for the threads it will execute
            WorkerArgs *args = new WorkerArgs();
            args->stackTop = module->allocateThreadStack();
            args->pool = this;
            args->workerIdx = i;

            // Run worker
            workers.emplace_back(Platform::createThread(0, workerEntryFunc, args));
        }
    }

    std::vector<std::future<I64>> PlatformThreadPool::run(std::vector<openmp::LocalThreadArgs> &&threadArgs) {
        std::vector<std::promise<I64>> newPromises;
        size_t numThreads = threadArgs.size();
        newPromises.resize(numThreads);
        std::vector<std::future<I64>> futures;
        futures.reserve(numThreads);
        for (size_t i = 0; i < numThreads; i++) {
            futures.emplace_back(newPromises[i].get_future());
        }
        promises = std::move(newPromises);

        // Diligently takes the lock, unlikely to be necessary
        {
            UniqueLock lock(mutex);
            runNumber += 1;
        }
        workerArgs = std::move(threadArgs);
        condition.notify_all();
        return futures;
    }

    PlatformThreadPool::~PlatformThreadPool() {
        {
            UniqueLock lock(mutex);
            stop = true;
        }
        condition.notify_all();
        for (auto worker : workers) {
            Platform::joinThread(worker);
        }
    }

    void PlatformThreadPool::clearPromises() {
        promises.clear();
    }

}
