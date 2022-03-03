
#include <lsqecc/logical_lattice_ops/ls_instructions_parse.hpp>
#include <lsqecc/logical_lattice_ops/parse_utils.hpp>
#include <lstk/lstk.hpp>

#include <ranges>
#include <stdexcept>

namespace lsqecc {

tsl::ordered_map<PatchId, PauliOperator> parse_multi_body_measurement_dict(std::string_view dict_arg)
{
    auto dict_pairs = lstk::split_on(dict_arg, ',');
    tsl::ordered_map<PatchId, PauliOperator> ret;

    for (auto pair: dict_pairs)
    {
        auto assoc = lstk::split_on(pair, ':');
        if (assoc.size()!=2)
        {
            throw std::logic_error(std::string{"MultiBody dict_pairs not in key pair format:"}+std::string{pair});
        }
        ret.insert_or_assign(parse_patch_id(assoc[0]), PauliOperator_from_string(assoc[1]));
    }
    return ret;
}

std::unordered_set<PatchId> parse_patch_id_list(std::string_view arg)
{
    std::unordered_set<PatchId> ret;
    auto entries = lstk::split_on(arg, ',');
    for(auto e : entries)
    {
        ret.insert(parse_patch_id(e));
    }
    return ret;
}

std::variant<LogicalLatticeOperation, std::unordered_set<PatchId>> parse_ls_instruction(std::string_view line)
{
    auto args = lstk::split_on(line,' ');
    auto args_itr = args.begin();

    auto has_next_arg = [&](){return args_itr<args.end();};
    auto get_next_arg = [&](){
        if(!has_next_arg())
            throw std::out_of_range{"Out of arguments"};
        return *args_itr++;
    };

    std::string_view instruction = get_next_arg();

    if(instruction == "DeclareLogicalQubitPatches")
    {
        return parse_patch_id_list(get_next_arg());
    }
    if(instruction == "Init" || instruction == "0")
    {
        auto patch_id = parse_patch_id(get_next_arg());
        auto state = parse_init_state(get_next_arg());
        return LogicalLatticeOperation{PatchInit{patch_id, state}};
    }
    else if(instruction == "MeasureSinglePatch" || instruction == "1")
    {
        auto patch_id = parse_patch_id(get_next_arg());
        auto op = PauliOperator_from_string(get_next_arg());
        return LogicalLatticeOperation{SinglePatchMeasurement{patch_id,op,false}};
    }
    else if (instruction == "MultiBodyMeasure" || instruction == "2")
    {
        auto patches_dict = get_next_arg();
        return LogicalLatticeOperation{MultiPatchMeasurement{parse_multi_body_measurement_dict(patches_dict)}};
    }
    else if(instruction == "RequestMagicState" || instruction == "3")
    {
        auto patch_id = parse_patch_id(get_next_arg());
        return LogicalLatticeOperation{MagicStateRequest{patch_id}};
    }
    else if (instruction == "LogicalPauli" || instruction == "4")
    {
        auto patch_id = parse_patch_id(get_next_arg());
        auto op = PauliOperator_from_string(get_next_arg());
        return LogicalLatticeOperation{SingleQubitOp{patch_id, static_cast<SingleQubitOp::Operator>(op)}};
    }
    else if (instruction == "HGate" || instruction == "5")
    {
        auto patch_id = parse_patch_id(get_next_arg());
        return LogicalLatticeOperation{SingleQubitOp{patch_id, SingleQubitOp::Operator::H}};
    }
    else if (instruction == "SGate" || instruction == "6")
    {
        auto patch_id = parse_patch_id(get_next_arg());
        return LogicalLatticeOperation{SingleQubitOp{patch_id, SingleQubitOp::Operator::S}};
    }
    else
    {
        throw std::logic_error(std::string{"Operation not supported: "}+std::string{instruction});
    }
}


InMemoryLogicalLatticeComputation parse_ls_instructions(std::string_view source)
{
    InMemoryLogicalLatticeComputation computation;

    auto view = lstk::split_on(source,'\n');
    bool got_patch_id_list = false;
    for (auto line : view)
    {
        if(!line.empty())
        {
            auto instruction = parse_ls_instruction(line);
            if(auto patch_list = std::get_if<std::unordered_set<PatchId>>(&instruction))
            {
                if(got_patch_id_list)
                {
                    throw std::logic_error("Double declaration of patch ids");
                }
                computation.core_qubits = std::move(*patch_list);
                got_patch_id_list=true;
            }
            else
            {
                computation.instructions.push_back(std::get<LogicalLatticeOperation>(instruction));
            }
        }
    }
    return computation;

}
}

