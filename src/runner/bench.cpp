#include <boost/filesystem.hpp>
#include <fstream>
#include <module_cache/WasmModuleCache.h>
#include <util/config.h>
#include <util/func.h>
#include <util/logging.h>
#include <util/timing.h>

void wasmRun(const std::string &user, const std::string &funcName, int wasmIterations, const std::string &outfile);
void wasmRunIteration(std::ofstream &profOut, message::Message &call, long num_iterations, const std::string &iteration_name,
                      int num_threads, wasm::WAVMWasmModule module);

int main(int argc, char *argv[]) {
    util::initLogging();
    const std::shared_ptr<spdlog::logger> &logger = util::getLogger();

    if (argc != 3) {
        logger->error("Usage:\nbench_omp <function> <nWasm>");
        return 1;
    }

    std::string user = "omp";
    std::string funcName = argv[1];
    int wasmIterations = std::stoi(argv[2]);

    std::string outfile = std::string("/usr/local/code/faasm/wasm/omp/")  + funcName + "/bench_pool.csv";
    // Clean up for container (and ensuring we will be writing to the same file
    boost::filesystem::remove(outfile);

    logger->info("Benchmarking {} ({}x wasm)", funcName, wasmIterations);

    wasmRun(user, funcName, wasmIterations, outfile);

    logger->info("Finished benchmark - {}", funcName);
    return 0;
}

void wasmRun(const std::string &user, const std::string &funcName, int wasmIterations, const std::string &outfile) {

    const std::shared_ptr<spdlog::logger> &logger = util::getLogger();

    std::ofstream profOut;
    profOut.open(outfile, std::fstream::app);

    logger->info("Running Wasm benchmark");
    if (!boost::filesystem::exists(outfile)) {
        throw std::runtime_error("Could not find native benchmark output at " + outfile);
    }

    std::vector<long> iterations = {200000l, 20000000l, 200000000l};
    std::vector<std::string> iter_names = {"Tiny,", "Small,", "Big,"};

    message::Message call = util::messageFactory(user, funcName);
    module_cache::WasmModuleCache &moduleCache = module_cache::getWasmModuleCache();
    wasm::WAVMWasmModule &cachedModule = moduleCache.getCachedModule(call);

    const util::TimePoint wasmTp = util::startTimer();

    for (int run = 1; run <= wasmIterations; run++) {
        logger->info("WASM - {} ({}/{})", funcName, run, wasmIterations);
        for (size_t i = 0; i < iterations.size(); i++) {
            wasmRunIteration(profOut, call, iterations[i], iter_names[i], 1, cachedModule);
            for (int num_threads = 2; num_threads < 25; num_threads += 2) {
                wasmRunIteration(profOut, call, iterations[i], iter_names[i], num_threads, cachedModule);
            }
        }
    }

    const long wasmTime = util::getTimeDiffMillis(wasmTp);
    logger->info("Done executing native in {} ms", wasmTime);


    profOut.flush();
    profOut.close();
}

void wasmRunIteration(std::ofstream &profOut, message::Message &call, long num_iterations, const std::string &iteration_name,
                      int num_threads, wasm::WAVMWasmModule cachedModule) {
    auto args = fmt::format("{} {}", num_threads, num_iterations);
    call.set_cmdline(args.c_str());

    const util::TimePoint iterationTp = util::startTimer();
    wasm::WAVMWasmModule module{cachedModule};
    module.execute(call);
    const long wasmIterationTime = util::getTimeDiffMicros(iterationTp);

    profOut << iteration_name << num_threads << ",wasm-pool," << wasmIterationTime << std::endl;
}

