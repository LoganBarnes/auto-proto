#pragma once

#include <proj/state.pb.h>

namespace svr {

class ServerTree;

class Compute {
public:
#if 0
        void compute_initial_state(proj::proto::InitialState* data);
        static void compute_state(proj::proto::State* data);
#endif

    void register_compute_functions(ServerTree* server_tree);
};

} // namespace svr
