#include "profile.h"

#if PROFILE_ENABLED

#include <cstdio>

namespace profiling {

namespace {

// Currently active scopes.
ScopeInfo *s_active = NULL;

}

FASTCALL int register_scope(ScopeInfo *info) {
    // Add it to the chain.
    info->next = s_active;
    s_active = info;

    // Unused value.
    return 0;
}

FASTCALL void print() {
    for (ScopeInfo *info = s_active; info != NULL; info = info->next) {
        // Log info about this one.
        printf("[PROF] %s: %lld ticks\n", info->name, info->ticks);

        // Clear it and move to the next.
        info->ticks = 0;
    }
}

} // namespace profiling

#endif // PROFILE_ENABLED
