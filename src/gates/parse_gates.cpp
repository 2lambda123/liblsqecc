#include <lsqecc/gates/parse_gates.hpp>

#include <lstk/lstk.hpp>

#include <vector>
#include <stdexcept>

namespace lsqecc {

struct Line
{
    // The parser assumes a line is formed by the following components
    // instruction_name <space> comma_separated_args [; // %comma_separated_annotations]
    // Otherwise, what follows a semicolon is treated as a comment

    std::string_view instruction;
    std::vector<std::string_view> args;
    std::vector<std::string_view> annotations;
};


// TODO specialize for the arbitrary precision case
template<class Integer>
Integer try_parse_int(std::string_view sv)
{
    std::string s{sv};

    errno = 0;
    char* endptr;
    Integer res = std::strtol(s.c_str(), &endptr, 10);

    if (errno != 0)
        throw GateParseException(lstk::cat("Could not parse ", s, "as string, strtol gave errno: ",
                                                static_cast<int>(errno)));
    if (endptr == s.c_str())
        throw GateParseException(lstk::cat("Could not parse ", s, ". No digits were found"));
    if (*endptr != '\0')
        throw GateParseException(lstk::cat("Could not parse ", s, ". Further characters after number"));

//    if(std::to_string(res) != s)
//        throw GateParseException(std::string{"Could not parse "} + s + "as string, atol gave "+std::to_string(res));
    return res;
}

Line split_instruction_and_args(std::string_view gate_str)
{
    if(!lstk::contains(gate_str, ' '))
        return {gate_str, {}};

    auto instr_split = lstk::split_on(gate_str,' ');
    auto instruction = instr_split.at(0);
    auto semicolon_split = lstk::split_on(instr_split.at(1),';');
    auto args =  lstk::split_on(semicolon_split.at(0), ',');
    auto annotation_line = semicolon_split.at(0);

    std::vector<std::string_view> annotations;
    if (annotation_line.starts_with(" // %"))
            annotations = lstk::split_on(annotation_line.substr(3),',');

    return Line{instruction, args, annotations};

}

QubitNum get_index_arg(std::string_view s)
{
    return try_parse_int<QubitNum>(
            lstk::split_on(
                    lstk::split_on(s,'[').at(1),
                    ']'
            ).at(0));
}


gates::Gate parse_qasm_gate(const Line& line)
{
    if(line.instruction == "x") return gates::X(get_index_arg(line.args.at(0)));
    if(line.instruction == "z") return gates::Z(get_index_arg(line.args.at(0)));
    if(line.instruction == "s") return gates::S(get_index_arg(line.args.at(0)));
    if(line.instruction == "t") return gates::T(get_index_arg(line.args.at(0)));
    if(line.instruction == "h") return gates::H(get_index_arg(line.args.at(0)));

    if(line.instruction == "cx")
    {
        if(line.args.size() != 2) throw GateParseException{lstk::cat("cx gate must have 2 args")};
        return gates::CNOT(get_index_arg(line.args.at(1)), get_index_arg(line.args.at(0)));
    }

    if(line.instruction.substr(0,2) == "rz")
    {
        if(line.instruction.substr(2,6) == "(pi/")
        {
            auto pi_frac_den = try_parse_int<ArbitraryPrecisionInteger>(
                    lstk::split_on(line.instruction,')').at(0));
            return gates::RZ{
                get_index_arg(line.args[0]),
                Fraction{1,pi_frac_den}};
        }
        else {
            throw GateParseException{lstk::cat(
                    "Can only parse pi/n for n power of 2 angles as crz args, got ",line.instruction)};
        }
    }
    if(line.instruction.substr(0,3) == "crz")
    {
        if(line.instruction.substr(3,7) == "(pi/")
        {
            auto pi_frac_den = try_parse_int<ArbitraryPrecisionInteger>(
                    lstk::split_on(line.instruction,')').at(0));
            return gates::CRZ(
                get_index_arg(line.args[1]),
                get_index_arg(line.args[0]),
                Fraction{1,pi_frac_den});
        }
        else {
            throw GateParseException{lstk::cat(
                    "Can only parse pi/n for n power of 2 angles as crz args, got ",line.instruction)};
        }
    }

    throw GateParseException{lstk::cat("Instruction not implemented ",line.instruction)};
}

bool is_ignored_instruction(std::string_view instr)
{
    if( instr == "OPENQASM" ||
        instr == "include" ||
        instr == "barrier")
        return true;
    return false;
}



Qreg parse_qreg(std::vector<std::string_view>& args)
{
    if(args.size() != 1)
        throw GateParseException(lstk::cat("Encountered qreg with ", args.size(), "args"));

    auto parts = lstk::split_on(args[0], '[');
    QubitNum num = get_index_arg(args[0]);
    return {
        .name=std::string{parts[0]},
        .size=num
    };
}

ParseGateResult parse_gate(std::string_view str_line)
{
    Line line = split_instruction_and_args(str_line);
    if (!is_ignored_instruction(line.instruction))
    {
        if (line.instruction == "qreg")
            return parse_qreg(line.args);
        else
            return parse_qasm_gate(line);
    }
    return IgnoredInstruction{};
}



void GateStreamFromFile::advance_gate()
{

    ParseGateResult maybe_gate = IgnoredInstruction{};
    while(!std::holds_alternative<gates::Gate>(maybe_gate))
    {
        std::string line;
        while(!gate_file_.eof() && line.size() ==0)
        {
            std::getline(gate_file_, line);
            line_number_++;
        }

        if(line.size()==0)
        {
            next_gate_ = std::nullopt;
            return;
        }

        try
        {
            maybe_gate = parse_gate(std::string_view{line});

            if(auto* qreg = std::get_if<Qreg>(&maybe_gate))
            {
                if(!qreg_)
                    qreg_ = *qreg;
                else
                    throw GateParseException{
                            lstk::cat("Can only handle one qreg, found second at line ", line_number_)};
            }
        }
        catch (const GateParseException& e)
        {
            throw std::runtime_error{
                    lstk::cat("Encountered gate parsing exception at line ", line_number_, ":\n", e.what())};
        }
    }

    next_gate_ = std::make_optional(std::get<gates::Gate>(maybe_gate));
}


GateStreamFromFile::GateStreamFromFile(std::istream& gate_file)
        : gate_file_(gate_file)
{
    advance_gate();

    if(!qreg_)
        throw std::runtime_error{"A single qreg must precede instructions"};
}

gates::Gate GateStreamFromFile::get_next_gate()
{
    gates::Gate gate = next_gate_.value();
    advance_gate();
    return gate;
}

}
