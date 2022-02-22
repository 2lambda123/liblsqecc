#ifndef LSQECC_ROUTING_HPP
#define LSQECC_ROUTING_HPP


#include <lsqecc/patches/slice.hpp>
#include <lsqecc/patches/patches.hpp>

namespace lsqecc {


std::optional<RoutingRegion> graph_search_route_ancilla(
        const Slice& slice,
        PatchId source,
        PauliOperator source_op,
        PatchId target,
        PauliOperator target_op
);

std::optional<RoutingRegion> do_s_gate_routing(Slice& slice, PatchId target);

}
#endif //LSQECC_ROUTING_HPP
