#include <array>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <time.h>
#include <vector>

class Model;
struct CodeInfo;
struct StepInfo;

static constexpr const char* NL  = "\n";
static constexpr const char* TAB = "\t";

// utility for Conf
struct check_c_char {
    auto operator()(char const* a, char const* b) const {
        return std::strcmp(a, b) < 0;
    }
};

// map to store float values
struct Conf : public std::map<const char*, float, check_c_char> {
    // utility to read key from Conf object
    template <typename T> auto get_value(const char* id, T ref) const {
        auto v = find(id);
        if (v != end())
            return static_cast<T>(v->second);
        return ref;
    }
};

// list of source code lines
using CodeLines = std::vector<std::string>;

// a step in a recipe
struct RecipeStep {
    const char* const _name;
    const std::function<void(StepInfo&)> _info;
    const std::function<bool(const Conf&, Model&)> _execute;
    const std::function<void(const Conf&, CodeLines&, CodeInfo&)> _code;
};

// registry to store RecipeStep objects
class Registry {
public:
    Registry() {
        _steps.reserve(64);
    };

    ~Registry() = default;

    // returns the number of registered steps
    auto get_count() const {
        return static_cast<unsigned int>(_steps.size());
    }

    // returns the step of the given index
    std::optional<const RecipeStep* const> get_step(unsigned int index) const {
        if (index < _steps.size())
            return &_steps[index];
        return std::nullopt;
    }

    // returns step with the given id
    std::optional<const RecipeStep* const> get_step(const char* id) const {
        for (const auto& s : _steps)
            if (std::strcmp(s._name, id) == 0)
                return &s;
        return std::nullopt;
    }

    // registers a new step
    template <typename... ARGS> void reg(ARGS... args) {
        _steps.emplace_back(args...);
    }

    // returns true if all registered steps are valid
    auto validate() const {
        for (const auto& s : _steps) {
            if (s._name == nullptr)
                return false;
            if (s._info == nullptr)
                return false;
            if (s._execute == nullptr)
                return false;
            if (s._code == nullptr)
                return false;
        }
        return true;
    }

private:
    std::vector<RecipeStep> _steps;
};

// instance of a RecipeStep; storing additional Conf object
struct RecipeStepInstance {
    // executes the step
    auto execute(Model& m) const {
        return _step->_execute(_config, m);
    }

    // creates code for the step
    void make_code(CodeLines& code, CodeInfo& info) const {
        _step->_code(_config, code, info);
    }

    // stores a key in the Conf member
    void set_config(const char* id, float v) {
        _config[id] = v;
    }

    const RecipeStep* const _step;
    Conf _config;
};

// recipe stores list of RecipeStepInstance objects
class Recipe {
public:
    Recipe() {
        _steps.reserve(64);
    }
    ~Recipe() = default;

    // adds a new RecipeStepInstance based on the given RecipeStep
    auto add_step(const RecipeStep* step) {
        _steps.push_back({step, {}});
        return count() - 1u;
    }

    // returns number of stored steps
    unsigned int count() const {
        return static_cast<unsigned int>(_steps.size());
    }

    // returns RecipeStepInstance at the given index
    std::optional<RecipeStepInstance* const> get(unsigned int index) {
        if (index < _steps.size())
            return &_steps[index];
        return std::nullopt;
    }

    // returns reference to list of step instances
    const std::vector<RecipeStepInstance>& all() const {   
        return _steps;
    }

    // stores Recipe to text file
    void store(const char* file) {
        std::ofstream file_stream {file, std::ofstream::out};
        for (const auto& s : _steps) {
            file_stream << s._step->_name << NL;
            if (!s._config.empty()) {
                for (const auto& [key, value] : s._config)
                    file_stream << "-->" << key << ":" << std::to_string(value) << NL;
            }
        }
    }

private:
    std::vector<RecipeStepInstance> _steps;
};

// ---------------------------- Data Model ----------------------------

// data mode; modified by RecipeStep objects
class Model {
public:
    Model() {};
    ~Model() {};

    std::vector<float> _data;
    float _res = 0.0f;

    static void setup_code(CodeLines& code) {
        code.push_back("std::vector<float> data;");
        code.push_back("auto res = .0f;");
    }
    static void cleanup_code(CodeLines& code) {
        code.push_back("data.clear();");
        code.push_back("data.shrink_to_fit();");
    }
};

// information on the generated code
struct CodeInfo {
    bool needs_scope = false;
};

// information on the specific step
struct StepInfo {
    bool always_same_code     = false;
    bool returns_stop         = false;
    const char* stop_variable = nullptr;
};

// ---------------------------- cook the recipe ----------------------------

// runs a recipe
static void run(Recipe& recipe,
                std::function<void(unsigned int, const char*)> progress,
                std::function<void(const char*, float)> print_key,
                std::function<void(long long)> print_time) {
    Model model;

    auto start = std::chrono::system_clock::time_point {};

    auto i = 0;
    for (const auto& s : recipe.all()) {
  
        progress(i, s._step->_name);

        for (const auto& [key, v] : s._config)
            print_key(key, v);

        start = std::chrono::system_clock::now();

        if (!s.execute(model))
            return;

        const auto end = std::chrono::system_clock::now();
        const auto diff =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        print_time(diff);
        start = end;

        ++i;
    }
}

// create code from the Recipe
static void create_code(Recipe& recipe, const char* file) {

    std::ofstream stream {file, std::ofstream::out};

    {
        const auto now  = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);
        char buffer[26];
        ctime_s(buffer, sizeof buffer, &time);
        stream << "// " << buffer << NL;
    }

    CodeLines code;
    code.reserve(64);

    stream << "#include<vector>" << NL;
    stream << "#include<iostream>" << NL << NL;
    stream << "int main() {" << NL << NL;

    code.clear();
    Model::setup_code(code);
    for (const auto& line : code)
        stream << "\t" << line << "\n";

    CodeLines cleanup_code;
    cleanup_code.push_back("// cleanup");
    Model::cleanup_code(cleanup_code);

    for (const auto& s: recipe.all()) {
    
            CodeInfo info;
            code.clear();
            s.make_code(code, info);

            stream << "\n";
            if (info.needs_scope)
                stream << "\t{" << NL;

            auto* tabs = "\t";
            if (info.needs_scope)
                tabs = "\t\t";

            stream << tabs << "// " << s._step->_name << NL;

            for (const auto& [key, v] : s._config)
                stream << tabs << "// " << key << " : " << v << NL;

            for (const auto& line : code)
                stream << tabs << line << NL;

            StepInfo stepInfo;
            s._step->_info(stepInfo);
            if (stepInfo.returns_stop) {
                stream << tabs << "if (!" + std::string {stepInfo.stop_variable} + ") {" << NL;

                for (const auto& line : cleanup_code)
                    stream << tabs << "\t" << line << NL;

                stream << tabs << "\treturn 0;" << NL;
                stream << tabs << "}" << NL;
            }

            if (info.needs_scope)
                stream << "\t}" << NL;
       
    }
    {
        stream << NL;
        for (const auto& line : cleanup_code)
            stream << "\t" << line << NL;
    }

    stream << NL << "\treturn 0;" << NL;
    stream << "}" << NL;

    stream.close();
}

// create code from the Recipe
static void create_code_func(Recipe& recipe, const char* cpp_file, const char* header_file) {
    const auto cnt = recipe.count();

    auto func_name = [](const RecipeStep* step, const StepInfo& info, unsigned int i) {
        if (info.always_same_code)
            return std::string(step->_name);

        return std::string(step->_name) + "_" + std::to_string(i);
    };

    {
        std::ofstream header_stream {header_file, std::ofstream::out};

        header_stream << "#pragma once" << NL;
        header_stream << "#include<vector>" << NL;
        header_stream << "#include<iostream>" << NL << NL;

        std::set<std::string> func_names;

        for (auto i = 0u; i < cnt; ++i) {
            auto step = recipe.get(i);
            if (step) {
                const auto* s = step.value();

                StepInfo stepInfo;
                s->_step->_info(stepInfo);

                const auto function_name = func_name(s->_step, stepInfo, i);

                if (!func_names.contains(function_name)) {
                    std::string returnValue = "void";
                    if (stepInfo.returns_stop)
                        returnValue = "auto";

                    const auto func_declaration = "inline " + returnValue + " " + function_name +
                                                  "(std::vector<float>&data, float&res)" + NL;

                    header_stream << func_declaration;
                    header_stream << "{" << NL;

                    for (const auto& [key, v] : s->_config)
                        header_stream << "\t// " << key << " : " << v << NL;

                    CodeLines code;
                    CodeInfo info;
                    s->make_code(code, info);
                    for (const auto& line : code)
                        header_stream << "\t" << line << NL;

                    if (stepInfo.returns_stop && stepInfo.stop_variable)
                        header_stream << "\treturn " << stepInfo.stop_variable << ";" << NL;

                    header_stream << "}" << NL << NL;

                    func_names.insert(function_name);
                }
            }
        }
        {
            CodeLines code;
            Model::cleanup_code(code);

            header_stream << "inline void _cleanup(std::vector<float>&data, float&res)" << NL;
            header_stream << "{" << NL;

            for (const auto& line : code)
                header_stream << "\t" << line << NL;

            header_stream << "}" << NL << NL;
        }
        header_stream.close();
    }
    {
        std::ofstream cpp_stream {cpp_file, std::ofstream::out};

        cpp_stream << "#include \"" << header_file << "\"" << NL << NL;
        cpp_stream << "int main() {" << NL << NL;

        {
            CodeLines code;
            Model::setup_code(code);
            for (const auto& line : code)
                cpp_stream << "\t" << line << NL;
        }

        for (auto i = 0u; i < cnt; ++i) {
            auto step = recipe.get(i);
            if (step) {
                const auto* s = step.value();

                StepInfo info;
                s->_step->_info(info);

                const auto fname = func_name(s->_step, info, i) + "(data, res)";

                if (info.returns_stop) {
                    cpp_stream << "\n\tif (!" << fname << ") {" << NL;
                    cpp_stream << "\t\t_cleanup(data, res);" << NL;
                    cpp_stream << "\t\treturn 0;" << NL;
                    cpp_stream << "\t}" << NL;
                } else {
                    cpp_stream << NL;
                    cpp_stream << "\t" << fname << ";" << NL;
                }
            }
        }

        cpp_stream << NL << "\t_cleanup(data, res);" << NL;

        cpp_stream << NL << "\treturn 0;" << NL;
        cpp_stream << "}" << NL;

        cpp_stream.close();
    }
}



#define KEY(key) constexpr static const char* key = #key;

// ---------------------------- Example Elements ----------------------------

namespace conf {
    namespace print_number {
        KEY(num)
    }
    namespace add_values {
        KEY(cnt)
    }
    namespace check_value {
        KEY(ref)
    }
} // namespace conf

static auto hello_world(const Conf&, Model&) {
    std::cout << "Hello World!\n";
    return true;
}

static void hello_world_code(const Conf&, CodeLines& code, CodeInfo&) {
    code.push_back("std::cout << \"Hello World !\\n\";");
}

static auto print_number(const Conf& conf, Model&) {
    const auto value = conf.get_value(conf::print_number::num, .0f);
    std::cout << "Number: \"" << value << "\"\n";
    return true;
}

static void print_number_info(StepInfo& info) {
    info.always_same_code = false;
}

static void print_number_code(const Conf& conf, CodeLines& code, CodeInfo&) {
    const auto say    = conf.get_value(conf::print_number::num, .0f);
    const auto sayStr = std::to_string(say);
    code.push_back("std::cout<<\"Number: \"<<" + sayStr + "<<\"\\n\";");
}

static auto add_values(const Conf& conf, Model& m) {
    const auto cnt = conf.get_value(conf::add_values::cnt, 0);
    m._data.resize(cnt);
    for (auto i = 0; i < cnt; ++i)
        m._data[i] = static_cast<float>(i);
    return true;
}

static void add_values_info(StepInfo& info) {
    info.always_same_code = false;
}

static void add_values_code(const Conf& conf, CodeLines& code, CodeInfo& info) {
    const auto cnt = conf.get_value(conf::add_values::cnt, 0);
    if (cnt > 0) {
        code.push_back("const auto cnt = " + std::to_string(cnt) + ";");
        code.push_back("data.resize(cnt);");
        code.push_back("for (auto i = 0; i < cnt; ++i) {data[i] = static_cast<float>(i);}");
        info.needs_scope = true;
    } else {
        code.push_back("data.clear();");
    }
}

static auto calculate_sum(const Conf&, Model& m) {
    m._res = 0.0f;
    for (const auto& v : m._data)
        m._res += v;
    return true;
}

static void calculate_sum_code(const Conf&, CodeLines& code, CodeInfo&) {
    code.push_back("res = 0.0f;");
    code.push_back("for (const auto&v:data) {res += v;}");
}

static auto print_value(const Conf&, Model& m) {
    std::cout << "Result: " << m._res << "\n";
    return true;
}

static void print_value_code(const Conf&, CodeLines& code, CodeInfo&) {
    code.push_back("std::cout << \"Result: \" << res <<\"\\n\";");
}

static auto print_data(const Conf&, Model& m) {
    std::cout << "Data:\n";
    for (const auto& v : m._data)
        std::cout << v << "\n";
    return true;
}

static void print_data_code(const Conf&, CodeLines& code, CodeInfo&) {
    code.push_back("std::cout << \"Data :\\n\";");
    code.push_back("for (const auto& v : data)");
    code.push_back("\tstd::cout << v << \"\\n\";");
}

static auto clear_values(const Conf&, Model& m) {
    m._data.clear();
    m._res = .0f;
    return true;
}

static void clear_values_code(const Conf&, CodeLines& code, CodeInfo&) {
    code.push_back("data.clear();");
    code.push_back("res = 0.0f;");
}

static auto calculate_product(const Conf&, Model& m) {
    m._res = 1.0f;
    for (const auto& v : m._data)
        m._res *= v;
    return true;
}

static void calculate_product_code(const Conf&, CodeLines& code, CodeInfo&) {
    code.push_back("res = 1.0f;");
    code.push_back("for (const auto&v:data) {res *= v;}");
}

static auto check_value(const Conf& conf, Model& m) {
    const auto ref = conf.get_value(conf::check_value::ref, 0.0f);
    return ref == m._res;
}

static void check_value_info(StepInfo& info) {
    info.always_same_code = false;
    info.returns_stop     = true;
    info.stop_variable    = "res_ok";
}

static void check_value_code(const Conf& conf, CodeLines& code, CodeInfo& info) {
    const auto ref    = conf.get_value(conf::check_value::ref, 0.0f);
    const auto refStr = std::to_string(ref);
    code.push_back("const auto expected_value = " + refStr + "f;");
    code.push_back("const auto res_ok = expected_value == res;");
    info.needs_scope = true;
}

static auto check_data(const Conf&, Model& m) {
    return !m._data.empty();
}

static void check_data_info(StepInfo& info) {
    info.always_same_code = true;
    info.returns_stop     = true;
    info.stop_variable    = "populated";
}

static void check_data_code(const Conf&, CodeLines& code, CodeInfo& info) {
    code.push_back("const auto populated = !data.empty();");
    info.needs_scope = true;
}

static void always_same_code(StepInfo& info) {
    info.always_same_code = true;
}

namespace step {
    KEY(print_number)
    KEY(hello_world)
    KEY(set_values)
    KEY(sum)
    KEY(product)
    KEY(print)
    KEY(print_data)
    KEY(check)
    KEY(check_data)
    KEY(reset)
} // namespace step

int main() {

    Registry reg;
    {
        reg.reg(step::print_number, print_number_info, print_number, print_number_code);
        reg.reg(step::hello_world, always_same_code, hello_world, hello_world_code);
        reg.reg(step::set_values, add_values_info, add_values, add_values_code);
        reg.reg(step::sum, always_same_code, calculate_sum, calculate_sum_code);
        reg.reg(step::product, always_same_code, calculate_product, calculate_product_code);
        reg.reg(step::print, always_same_code, print_value, print_value_code);
        reg.reg(step::print_data, always_same_code, print_data, print_data_code);
        reg.reg(step::reset, always_same_code, clear_values, clear_values_code);
        reg.reg(step::check, check_value_info, check_value, check_value_code);
        reg.reg(step::check_data, check_data_info, check_data, check_data_code);

        const auto valid = reg.validate();
        assert(valid);
        if (!valid)
            return 1;
    }

    Recipe recipe;
    {
        auto add_step = [&](const char* id) {
            auto res = reg.get_step(id);
            assert(res);
            if (res) recipe.add_step(res.value());
        };

        auto add_step_configure = [&](const char* id, const char* key, float v) {
            auto res = reg.get_step(id);
            assert(res);
            if (res) {
                const auto index = recipe.add_step(res.value());
                recipe.get(index).value()->set_config(key, v);
            }
        };

        // define the behaviour

        add_step(step::hello_world);
        add_step_configure(step::print_number, conf::print_number::num, 42.0f);
        add_step_configure(step::set_values, conf::add_values::cnt, 0);
        add_step_configure(step::set_values, conf::add_values::cnt, 10);
        add_step(step::check_data);
        add_step(step::sum);
        add_step(step::print);
        add_step_configure(step::check, conf::check_value::ref, 45.0f);
        add_step(step::reset);
        add_step_configure(step::set_values, conf::add_values::cnt, 20);
        add_step(step::check_data);
        add_step(step::print_data);
        add_step(step::product);
        add_step(step::print);

        recipe.store("test.recipe");
    }

    {
        auto print_progress = [](unsigned int s, const char* name) {
            std::cout << "\n\033[1;32mStep " << s << " :\t\033[0m\033[1;36m" << name << "\033[0m\n";
        };

        auto print_keys = [](const char* key, float v) {
            std::cout << "\t\t\033[1;33mKey: " << key << ", Value: " << v << "\033[0m \n";
        };

        auto print_time = [](long long ms) {
            std::cout << "\n\t\t\033[1;37mTime: " << ms << " ns\t\033[0m\n";
        };

        run(recipe, print_progress, print_keys, print_time);
    }

    create_code(recipe, "my_app.cpp");

    create_code_func(recipe, "my_app_2.cpp", "my_header.h");

    return 0;
}
