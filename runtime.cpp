/*
 * runtime.cpp
 *
 * Minimal C++ ABI runtime hooks for freestanding bare-metal builds.
 *
 * Because there is no hosted runtime, these handlers avoid unresolved
 * references if the compiler emits calls for pure virtual handling.
 */

extern "C" void __cxa_pure_virtual(void) {
    while (true) {
        /* trap */
    }
}
