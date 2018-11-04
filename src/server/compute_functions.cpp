#include "server/compute_functions.h"
#include "server/server_tree.h"

namespace svr {

#if 0
void Compute::compute_initial_state(proj::proto::InitialState* data) {
    data->set_needs_semi_shared_and_transform(data->semi_shared().state() + "_" + data->transform().state());
}

void Compute::compute_state(proj::proto::State* data) {
    data->set_final_msg(data->initial_state().needs_semi_shared_and_transform() + "_"
                        + data->repeated_states().needs_semi_shared());
}
#endif

void Compute::register_compute_functions(ServerTree* server_tree) {
    server_tree->register_function<proj::proto::Inner1>(
        [](proj::proto::Inner1* inner) { inner->set_state("_" + inner->source1().state() + "_"); });

    server_tree->register_function<proj::proto::Inner2>(
        [](proj::proto::Inner2* inner) { inner->set_state("_" + inner->source2().state() + "_"); });

    server_tree->register_function<proj::proto::Inner3>([](proj::proto::Inner3* inner) {
        inner->set_state("_" + inner->source2().state() + "_" + inner->inner1().state() + "_" + inner->inner2().state()
                         + "_");
    });

    server_tree->register_function<proj::proto::Inner4>([](proj::proto::Inner4* inner) {
        inner->set_state("_" + inner->source3().state() + "_" + inner->inner3().state() + "_");
    });

    server_tree->register_function<proj::proto::Inner5>([](proj::proto::Inner5* inner) {
        inner->set_state("_" + inner->inner3().state() + "_" + inner->inner4().state() + "_");
    });

    server_tree->register_function<proj::proto::Inner6>(
        [](proj::proto::Inner6* inner) { inner->set_state("_" + inner->inner4().state() + "_"); });

    server_tree->register_function<proj::proto::Inner7>(
        [](proj::proto::Inner7* inner) { inner->set_state("_" + inner->inner4().state() + "_"); });

    server_tree->register_function<proj::proto::Sink2>([](proj::proto::Sink2* sink) {
        sink->set_final_update("_" + sink->inner5().state() + "_" + sink->inner6().state() + "_"
                               + sink->inner7().state() + "_");
    });
}

} // namespace svr
