#ifndef LSQECC_FAST_PATCH_COMPUTATION_HPP
#define LSQECC_FAST_PATCH_COMPUTATION_HPP


#include <lsqecc/logical_lattice_ops/logical_lattice_ops.hpp>
#include <lsqecc/patches/slice.hpp>
#include <lsqecc/patches/patches.hpp>
#include <lsqecc/layout/layout.hpp>
#include <lsqecc/layout/router.hpp>

#include <lstk/lstk.hpp>

#include <chrono>

namespace lsqecc {

class PatchComputation
{
public:

    PatchComputation (
            const LogicalLatticeComputation& logical_computation,
            std::unique_ptr<Layout>&& layout,
            std::unique_ptr<Router>&& router,
            std::optional<std::chrono::seconds> timeout);

private:

    void make_slices(const LogicalLatticeComputation& logical_computation, std::optional<std::chrono::seconds> timeout);

    /// Assumes that there already is a slice
    Slice& new_slice();
    Slice& last_slice();

    std::unique_ptr<Layout> layout_;
    std::unique_ptr<Router> router_;
    std::vector<Slice> slices_;

    std::vector<std::vector<lstk::bool8>> is_cell_free_;

private:
    void compute_free_cells();
    std::optional<Cell> find_place_for_magic_state(size_t distillation_region_idx) const;

public:
    const std::vector<Slice>& get_slices()const {return slices_;}
    const Layout& get_layout() const {return *layout_;}

};


}

#endif //LSQECC_FAST_PATCH_COMPUTATION_HPP
