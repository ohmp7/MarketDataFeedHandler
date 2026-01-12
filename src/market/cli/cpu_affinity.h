#pragma once

#ifdef __APPLE__
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#include <pthread.h>
#elif __linux__
#include <pthread.h>
#include <sched.h>
#endif

namespace CPUAffinity {
    inline bool PinToCore(int core_id) {
        #ifdef __APPLE__
            thread_affinity_policy_data_t policy = { core_id };
            thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());
            kern_return_t result = thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);
            return result;
        #elif __linux__
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(core_id, &cpuset);
            return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
        #endif
    }
}