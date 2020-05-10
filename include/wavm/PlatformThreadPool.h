#pragma once

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <vector>

#include <WAVM/Runtime/Runtime.h>
#include <WAVM/Inline/BasicTypes.h>
#include <WAVM/Platform/Thread.h>

#include <util/locks.h>
#include <util/locks.h>
#include <wavm/PlatformThreadPool.h>

namespace wasm {

    using namespace WAVM;

    class WAVMWasmModule;

    namespace openmp {
        struct LocalThreadArgs;
    }

    class PlatformThreadPool {
    public:
        PlatformThreadPool(size_t numThreads, WAVMWasmModule *module);

        friend I64 workerEntryFunc(void* _args);

        std::vector<std::future<I64>> run(std::vector<openmp::LocalThreadArgs> &&threadArgs);

        void clearPromises();

        ~PlatformThreadPool();

    private:
        std::vector<openmp::LocalThreadArgs> workerArgs;
        std::vector<std::promise<I64>> promises;
        std::mutex mutex;
        std::condition_variable condition;
        std::uint32_t runNumber = 0;
        std::vector<WAVM::Platform::Thread *> workers;
        bool stop = false;

    };

    struct WorkerArgs {
        U32 stackTop;
        PlatformThreadPool *pool;
        size_t workerIdx;
    };

}

