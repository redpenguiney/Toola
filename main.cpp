#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
#include <unordered_map>
#include <functional>
#include <regex>
#include <variant>
#include <optional>

std::string get_next_assembly_name();

class operand {
public:
	// returns the MCASM needed to retrieve the operand's value, and the MCASM symbol name in which that value is stored.
	virtual std::pair<std::string, std::string> retrieve_asmvar() = 0;

	virtual std::string get_type() = 0;

	virtual ~operand() = default;
};

class varname : public operand {
public:
	std::string symname;
	std::string asmvarname;
	std::string type;

	varname(std::string avn, std::string type, std::string svn = "COMPILER_TEMPORARY") : symname(svn), type(type), asmvarname(avn) {}

	// effectively returns reference
	std::pair<std::string, std::string> retrieve_asmvar() override {
		return std::make_pair("", asmvarname);
	}

	std::string get_type() { return type; }
};


// lexer?
struct tokenizer_context {
	int current_line_number = 1;
};

enum class parsing_task {
	code_body,
	class_body
};

enum class symbol_type {
	variable, // (functions are variables too)
	type
};

enum class scope_type {
	while_,
	for_,
	if_, // and else_if
	else_,
	main,
	function
};

class varname;

struct scope {
	std::unordered_map<std::string, symbol_type> known_symbols;
	std::unordered_map < std::string, std::shared_ptr<varname>> variables = {};
	scope_type type;
	bool should_return = false;
};

struct parsing_task_info {
	parsing_task task;
	int line_number = -1;
};

struct function_info {
	std::string cumulative_type;
	std::string ret_typename;
	std::vector<std::string> arg_typenames;
	std::string assemblyfuncname = get_next_assembly_name();

	function_info(std::string returntype, std::string totaltype, std::vector<std::string> argtypes, std::string asmname) : 
		cumulative_type(totaltype), ret_typename(returntype), arg_typenames(argtypes), assemblyfuncname(asmname) 
	{}
};

struct parser_context {
	std::vector<parsing_task_info> taskStack;
	std::vector<scope> scopeStack;

	// returns nullopt if not, otherwise will return "boolean", "string", "null", "double", or "int"
	std::optional<std::string> is_literal(std::string literal) {
		// boolean literal
		if (literal == "true" || literal == "false") return "boolean";

		// null literal
		if (literal == "null") return "null";

		// string literal
		if (literal.size() >= 2 && literal[0] == '\"' && literal.back() == '\"') return "string";

		// numeric literal (stolen from https://medium.com/@ryan_forrester_/c-check-if-string-is-number-practical-guide-c7ba6db2febf)
		static const std::regex number_regex(
			R"(^[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?$)"
		);
		if (std::regex_match(literal, number_regex)) {
			if (literal.find_first_of(".") != std::string::npos) {
				return "double";
			}
			else {
				return "int";
			}
		}
		else {
			return std::nullopt;
		}
	}

	bool is_variable(std::string symbol) {
		for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); it++) {
			//if (it->known_symbols.count(symbol) && it->known_symbols[symbol] == symbol_type::variable) {
			if (it->variables.contains(symbol))
				return true;
			//}
		}
		return false;
	}

	std::shared_ptr<varname> get_variable(std::string varname) {
		assert(is_variable(varname));
		for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); it++)
			//if (it->known_symbols.count(symbol) && it->known_symbols[symbol] == symbol_type::variable) {
			if (it->variables.contains(varname))
				return it->variables[varname];
	}

	bool is_basic_type(std::string symbol) {
		if (!symbol.empty() && symbol.back() == '&') symbol.pop_back();
		if (!symbol.empty() && symbol.back() == '&') throw std::runtime_error("cannot have reference to reference");
		if (symbol == "var") return true;
		for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); it++) {
			if (it->known_symbols.count(symbol) && it->known_symbols[symbol] == symbol_type::type) {
				return true;
			}
		}
		return false;
	}
		
	bool is_type(std::string symbol) {
		
		if (is_basic_type(symbol)) return true;
		else { // function type
			int first = symbol.find_first_of("(");
			int second = symbol.find_last_of(")");
			if (first != std::string::npos && first == symbol.find_last_of("(") && second == symbol.find_first_of(")") && second == symbol.size() - 1 && first < second) {
				int i = first + 1;

				std::string currentType;
				int typeState = 0; // 0 means future letters must be whitespace or part of a type or comma, 1 means future characters must be whitespace or a comma
				while (i < second) {
					if (symbol[i] == ' ' || symbol[i] == '\t' || symbol[i] == '\n') {
						if (typeState == 0) {
							typeState = 1;
							if (!is_basic_type(currentType)) throw std::runtime_error(std::string("unrecognized type ") + currentType);
							currentType = "";
						}
					}
					else if (symbol[i] == ',') {
						if (typeState == 1) {
							typeState = 0;
						}

						if (!is_basic_type(currentType)) throw std::runtime_error(std::string("unrecognized type ") + currentType);
						currentType = "";
					}
					else if (symbol[i] == '&') {
						if (typeState == 1) throw std::runtime_error("expected \",\", not \"&\"");
						if (currentType.empty()) throw std::runtime_error("type may not begin with \"&\"");
						currentType += "&";
					}
					else if (std::isdigit(symbol[i])) {
						if (typeState == 1) throw std::runtime_error("expected \",\", not type");
						if (currentType.empty()) throw std::runtime_error("type may not begin with digit");
						currentType += symbol[i];
					}
					else if (std::isalpha(symbol[i])) {
						if (typeState == 1) throw std::runtime_error("expected \",\", not type");
						if (currentType.size() > 0 && currentType.back() == '&') throw std::runtime_error("expected \"&\" or \",\" after \"&\"");
						currentType += symbol[i];
					}
					else {
						throw std::runtime_error("invalid character in function type specification");
					}

					i++;
				}
				
				return true;
			}

			return false;
		}
	}

	bool is_symbol(std::string symbol) {
		return is_type(symbol) || is_variable(symbol);
	}

	bool is_valid_symbol_name(std::string name) {
		assert(!name.empty());
		if (is_symbol(name)) return false;
		if (!std::isalpha(name[0])) return false;
		for (char& c : name) {
			if (!std::isalnum(c)) return false;
		}
		return true;
	}

	std::unordered_map<std::string, std::shared_ptr<function_info>> fmap;

	std::shared_ptr<function_info> get_function_info(std::string asmfuncname) {
		return fmap.at(asmfuncname);
	}
};

tokenizer_context tokenizer;
parser_context parser;
std::string src, out;


// returns the script backwards.
std::string get_src(std::string path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cout << "Failure to open " << path << "\n";
		throw std::runtime_error("Invalid path " + path);
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	file.close();

	assert(buffer.good());

	auto src = buffer.str();
	std::reverse(src.begin(), src.end());

	return src;
}

// if a token contains this character, then this character is the last and only character in the token.
//const std::vector<char> ALWAYS_LAST = {
	
//};

// if this character is in a token, then it is the only character in that token.
const std::vector<char> ALWAYS_STANDALONE = {
	' ',
	'(',
	')',
	'[',
	']',
	'{',
	'}',
	';',
	':',
	',',
	'\t',
	'\n'
}; 

enum associativity {
	left_to_right,
	right_to_left
};

class expression;
struct binary_operator;
//using operand = std::variant<literal, varname, funccall, std::shared_ptr<expression>, operator_>;



static const inline std::string return_asmvar = "ret";

std::string copy(std::string asmdst, std::string asmsrc) {
	return "\ncvar " + asmsrc + " " + asmdst;
}

// returns the code to convert the value of the given variable of the given type into the second type, if such a conversion is legal under implicit conditions (between binary operators). 
std::optional<std::string> implicit_convert_to_type(std::string asm_varname, std::string ti_type, std::string tf_type) {
	if (ti_type == tf_type) return "";// "\ndvar " + tf_asm_varname + " sym:" + ti_asm_varname;
	else if (tf_type == "double") {
		if (ti_type == "int") {
			return "\ns2d sym:" + asm_varname + " " + asm_varname;
		}
		else {
			return std::nullopt;
		}
	}
	else return std::nullopt;
}

// returns the code to convert the value of the given variable of the given type into the second type, if such a conversion is legal under explicit conditions. 
std::optional<std::string> explicit_convert_to_type(std::string asm_varname, std::string ti_type, std::string tf_type) {
	auto attempt = implicit_convert_to_type(asm_varname, ti_type, tf_type);
	if (attempt.has_value()) return attempt;
	else if (tf_type == "int") {
		if (ti_type == "double") {
			return "\nd2s sym:" + asm_varname + " " + asm_varname;
		}
		else if (ti_type == "boolean") {
			// boolean is already sint of either 1 or 2, this is easy
			return "";
		}
		else {
			return std::nullopt;
		}
	}
	else if (tf_type == "double") {
		if (ti_type == "boolean") {
			// same as an int cast
			return "\ns2d sym:" + asm_varname + " " + asm_varname;
		}
		else {
			return std::nullopt;
		}
	}
}

class funccall : public operand {
public:
	std::shared_ptr<function_info> function;
	std::vector<std::shared_ptr<expression>> args;
	std::pair<std::string, std::string> retrieve_asmvar();
	std::string get_type() { return function->ret_typename; }
	~funccall() = default;
};

class literal : public operand {
public:
	std::string type;
	std::string value;

	literal(std::string t, std::string v) : type(t), value(v) {};

	std::pair<std::string, std::string> retrieve_asmvar() override {
		auto literaltype = parser.is_literal(value);
		assert(literaltype);

		if (literaltype == "null") return std::make_pair("", "sint:0");
		else if (literaltype == "string") {
			std::string v = "str:";
			if (value == "") v += "null";
			else {
				std::string litstr = value.substr(1, value.size() - 2);
				for (char c : litstr) {
					v += std::to_string((uint8_t)c) + ",";
				}
			}
			v.pop_back();
			return std::make_pair("", v);
		}
		else if (literaltype == "boolean") return std::make_pair("", value == "true" ? "sint:1" : "sint:0");
		else if (literaltype = "int") return std::make_pair("", "sint:" + value);
		else if (literaltype = "double") return std::make_pair("", "dbl:" + value);
		else assert(false);
	}
	std::string get_type() { return type; }

	~literal() = default;
};



struct binary_operator_result {
	std::string type = "DNE"; // the type stored in the given variable
	std::string src = ""; // the MCASM that will store the result of applying this operand on its two arguments
};

struct binary_operator {
	associativity a = associativity::left_to_right;
	int priority = 100; // highest first

	// returns the code needed to store the result of this operation in the given assembly variable name. (the variable being store to must already be declared)
	std::function<binary_operator_result(std::string, operand&, operand&)> func =
		[](std::string, operand&, operand&) { assert(false); return binary_operator_result {}; };
};

struct unary_operator {
	int priority = 70;
	std::function < std::string(std::string, operand&)> func = [](std::string, operand&) {assert(false); return ""; };
};

std::unordered_map<std::string, unary_operator> unary_operators{
	//{"-", operator_ {.unary = true}},
	{"!", unary_operator {}}
};

std::string without_ref(std::string type) {
	if (type.back() == '&') {
		type.pop_back();
	}
	assert(parser.is_type(type));
	return type;
}

// handle4th: if 0 instruction takes 3 args, if 1 we discard 3rd arg and store 4th, if 2 we discard 4th and store 3rd
// TODO: HANDLE REFERENCE TYPES
std::function < binary_operator_result(std::string, operand&, operand&)> make_math_func(std::string dblinstruction, std::string intinstruction, bool modifyFirst = false, int handle4th = 0) {
	return [dblinstruction, intinstruction, handle4th, modifyFirst](std::string varname, operand& o1, operand& o2) {
		std::string out;
		auto [src1, o1v] = o1.retrieve_asmvar();
		out += src1;
		auto [src2, o2v] = o2.retrieve_asmvar();
		out += src2;

		std::string outtype;

		if (handle4th == 1) varname = "discard_result " + varname;
		else if (handle4th == 2) varname = varname + " discard_result";
			
		if (o1.get_type() == "double" || o2.get_type() == "double") {
			outtype = "double";
			if (o1.get_type() != "double") {
				if (!modifyFirst) {
					// copy o1 bc we don't want to change value of o1v.
					std::string temp = get_next_assembly_name();
					out += copy(temp, o1v);
					implicit_convert_to_type(temp, o1.get_type(), "double");
					o1v = temp;
				}
				else {
					throw std::runtime_error("incompatible operands");;
				}
			}
			else if (o2.get_type() != "double") {
				// copy o2 bc we don't want to change value of o1v.
				std::string temp = get_next_assembly_name();
				out += copy(temp, o2v);
				implicit_convert_to_type(temp, o2.get_type(), "double");
				o2v = temp;
			}


			out += "\n" + dblinstruction + "sym:" + o1v + " sym:" + o2v + " " + varname;
		}
		else if (o1.get_type() == "int" || o2.get_type() == "int") {
			outtype = "int";
			if (o1.get_type() != "int") {
				if (!modifyFirst) {
					// copy o1 bc we don't want to change value of o1v.
					std::string temp = get_next_assembly_name();
					out += copy(temp, o1v);
					implicit_convert_to_type(temp, o1.get_type(), "int");
					o1v = temp;
				}
				else {
					throw std::runtime_error("incompatible operands");;
				}
			}
			else if (o2.get_type() != "int") {
				// copy o2 bc we don't want to change value of o1v.
				std::string temp = get_next_assembly_name();
				out += copy(temp, o2v);
				implicit_convert_to_type(temp, o2.get_type(), "int");
				o2v = temp;
			}

			out += "\n" + intinstruction + "sym:" + o1v + " sym:" + o2v + " " + varname;
		}
		else {
			throw std::runtime_error("incompatible operands");
		}
		//return "\ndvar " + varname + " "

		return binary_operator_result{ .type = outtype, .src = out };
	};
}

// see https://en.cppreference.com/w/cpp/language/operator_precedence
std::unordered_map<std::string, binary_operator> binary_operators {
	
	{".", binary_operator { .priority = 80}},

	{"*", binary_operator {.priority = 70, .func = make_math_func("dmul", "smul")}},
	{"/", binary_operator { .priority = 70, .func = make_math_func("ddiv", "sdiv", false, 2)}},
	{"%", binary_operator { .priority = 70, .func = make_math_func("ddiv", "sdiv", false, 1)}},
	
	{"+", binary_operator { .priority = 60, .func = make_math_func("dadd", "sadd")}},
	{"-", binary_operator { .priority = 60, .func = make_math_func("dsub", "ssub")}},

	{">=", binary_operator { .priority = 50, .func = make_math_func("dmul", "smul")}},
	{"<=", binary_operator { .priority = 50, .func = make_math_func("dmul", "smul")}},
	{"<", binary_operator { .priority = 50, .func = make_math_func("dmul", "smul")}},
	{">", binary_operator { .priority = 50, .func = make_math_func("dmul", "smul")}},

	{"==", binary_operator { .priority = 40}},
	{"!=", binary_operator { .priority = 40}},

	{"&&", binary_operator { .priority = 30}},

	{"||", binary_operator { .priority = 20}},

	{"=", binary_operator { .a = right_to_left, .priority = 10}},
	{"+=", binary_operator { .a = right_to_left, .priority = 10, .func = make_math_func("dadd", "sadd", true)}},
	{"-=", binary_operator { .a = right_to_left, .priority = 10, .func = make_math_func("dsub", "ssub", true)}},
	{"*=", binary_operator { .a = right_to_left, .priority = 10, .func = make_math_func("dmul", "smul", true)}},
	{"/=", binary_operator { .a = right_to_left, .priority = 10, .func = make_math_func("dsub", "ssub", true, 2)}},
	{"%=", binary_operator { .a = right_to_left, .priority = 10, .func = make_math_func("dsub", "ssub", true, 1)}},
};

//std::unordered_map<std::string, operator_> post_operators{
//	{"(", operator_ {.unary = true, .priority = 120} }, // function call
//	{"[", operator_ {.unary = true, .priority = 120} }, // subscript
//
//}



void return_token(std::string token) {
	// add backwards
	std::reverse(token.begin(), token.end());
	src += token;
	if (token == "\n") tokenizer.current_line_number--;
}



std::string get_next_token() {

	std::string token = "";
	bool escape_next_char = false;
	int is_number = -1; // 0 if no, 1 if left side of decimal point, 2 if right side of decimal point
	constexpr int NONE = -10000;

	bool in_string_literal = false;
	bool in_long_comment = false;
	bool in_line_comment = false;

	int last_char = NONE;
	while (src.size() > 0) {
		char c = src.back();
		src.pop_back();

		if (last_char == NONE) {
			is_number = std::isdigit(c) ? 1 : 0;
			if (c == '-') {
				char next = src.back();
				if (std::isdigit(next)) is_number = 1;
			}
		}

		if (c == '\n') tokenizer.current_line_number++;
		
		if (is_number != 0 && !(c == '-' || c == '.' || std::isdigit(c))) { // then this number has ended
			src.push_back(c);
			break;
		}

		if (in_string_literal) {
			if (c == '\\' && !escape_next_char) {
				escape_next_char = true;
			}
			else {
				token += c;

				if (c == '\"' && !escape_next_char) {
					in_string_literal = false;
					break;
				}
			}
		}
		else if (in_line_comment) {
			if (c == '\n') {
				//src.push_back(c);
				token += {'\n'};
				break;
			}
		}
		else if (in_long_comment) {
			if (c == '/' && last_char == '*')
				in_long_comment = false;

		}
		else if (c == '\"' && !in_string_literal) {
			if (token.empty()) {
				token += { c };
				in_string_literal = true;
			}
			else {
				src.push_back(c);
				break;
			}
		}
		else if (c == '/' && last_char == '/') {
			in_line_comment = true;
			// remove the first slash from the token
			token.pop_back();
			last_char = NONE;
		}
		else if (last_char == '|') {
			if (c == '|') {
				token += {c};
				break;
			}
			else {
				src.push_back(c);
				break;
			}
		}
		else if (c == '&') {
			if (parser.is_type(token)) {
				token += { c };
			}
			else if (last_char == '&') {
				token += { c };
				break;
			}
			else {
				src.push_back(c);
				break;
			}
		}
		// these characters ALWAYS form their own token. 
		// if we already started a token with something else, return the token we already had and return this character the next time get_next_token() is called.
		else if (std::find(ALWAYS_STANDALONE.begin(), ALWAYS_STANDALONE.end(), c) != ALWAYS_STANDALONE.end())
			if (token.empty()) {
				token = { c };
				break;
			}
			else {
				src.push_back(c);
				break;
			}
		else if (c == '=') {
			if (last_char == NONE) {
				// the token might end up being ==, so keep going
				token += { c };
			}
			else if ( // handle +=, -=, ==, etc.
				last_char == '+' ||
				last_char == '-' ||
				last_char == '*' ||
				last_char == '/' ||
				last_char == '=' ||
				last_char == '%' ||
				last_char == '!' ||
				last_char == '<' ||
				last_char == '>'
				)
			{ 
				token += { c };
				break;
			}
			else { // this = should be part of the next token, not this one
				src.push_back(c);
				break;
			}
		}
		// numeric literals (TODO: hexadecimal would be simple) (todo: suffixes like 3.0f?)
		// (we don't treat it as this if it's following letters because that means they just making a symbol name like x3 or bobis32)
		// implicitly handled, actually
		//else if (std::isdigit(c) && !(last_char != NONE && std::isalpha(last_char))) {
			
			//token += c;
		//}
		// could be decimal point, could be member access syntax 
		else if (c == '.') {
			if (is_number == 2) {
				throw std::runtime_error("malformed number");
			}
			else if (is_number == 1) { // add decimal to literal
				is_number = 2;
				token += { c };
			}
			else { // member access syntax, should be its own token
				if (token.empty()) {
					token = { c };
					break;
				}
				else {
					src.push_back(c);
					break;
				}
			}
		}
		// handle operators that weren't +=, -=, etc.
		else if (last_char == '+' || last_char == '-' || last_char == '*' || last_char == '/' || last_char == '%') {
			if (c == last_char && (c == '+' || c == '-')) {
				token += {c};
				break;
			}
			else {
				if (last_char == '-' && std::isdigit(c)) {
					token += {c};
				} 
				else {
					src.push_back(c);
					break;
				}
			}
		}

		// all other tokens are just user-defined names of stuff or keywords, just add the letter and move on
		else {
			token += { c };
		}

		last_char = c;
		escape_next_char = false;
	}

	//std::cout << "|" << token;
	/*if (token == "class") {
		std::cout << "";
	}*/

	return token;
}

bool is_reserved(std::string str) {
	return str == "class" || str == "function" || str == "for" || str == "while";
}

std::unordered_map<std::string, std::string> symbol_to_assembly_names; // key is a function or variable name in user program. value is corresponding name 
int i = 0;
std::string get_next_assembly_name() {
	return "v" + std::to_string(i++);
}

class expression: public operand {
public:

	

	using token = std::variant<binary_operator, unary_operator, std::shared_ptr<operand>>;
	std::vector<token> tokens; 
	std::string type;
	bool sorted = false;
	bool computed_type = false;

	expression(std::vector<token> t): tokens(t) {}

	// sort operands into polish postfix notation (https://en.wikipedia.org/wiki/Shunting_yard_algorithm)
	void shunting_yard() {
		assert(!sorted);
		auto remaining = tokens;
		std::vector<binary_operator> stack;
		std::vector<token> out;
		std::reverse(remaining.begin(), remaining.end());
		while (!remaining.empty()) {
			auto thing = remaining.back();
			remaining.pop_back();

			if (std::holds_alternative<unary_operator>(thing)) {
				unary_operator op = std::get<unary_operator>(thing);
					
				if (remaining.empty()) throw std::runtime_error("expected symbol after unary");
				auto next = remaining.back();
				remaining.pop_back();
				if (!std::holds_alternative<std::shared_ptr<operand>>(next)) throw std::runtime_error("expected symbol after unary, not operator");
				thing = std::shared_ptr<expression>(new expression(std::vector<token>{op, next}));
			}

			if (std::holds_alternative<std::shared_ptr<operand>>(thing))
			{
				out.push_back(thing);
			}
			else {
				while (!stack.empty()
					&& (stack.back().priority > std::get<binary_operator>(thing).priority ||
						(stack.back().priority == std::get<binary_operator>(thing).priority && std::get<binary_operator>(thing).a == associativity::left_to_right))) {
					auto o = stack.back();
					stack.pop_back();
					out.push_back(o);
				}
				stack.push_back(std::get<binary_operator>(thing));
			}
		}


		while (!stack.empty()) {
			auto o = stack.back();
			stack.pop_back();
			out.push_back(o);
		}

		std::reverse(out.begin(), out.end());

		sorted = true;
		tokens = out;
	}

	std::pair<std::string, std::string> retrieve_asmvar() override {
		assert(sorted);
		std::string out;
		std::vector<token> mathables;
		auto rtokens = tokens;
		std::reverse(rtokens.begin(), rtokens.end());
		for (auto& op : rtokens) {
			if (std::holds_alternative<std::shared_ptr<operand>>(op)) {
				mathables.push_back(op);
			}
			else {
				assert(mathables.size() > 1);
				auto o1 = mathables.back();
				mathables.pop_back();
				auto o2 = mathables.back();
				mathables.pop_back();
				auto tempasmname = get_next_assembly_name();
				auto subout = std::get<binary_operator>(op).func(tempasmname, *std::get<std::shared_ptr<operand>>(o1), *std::get<std::shared_ptr<operand>>(o2));
				mathables.push_back(std::shared_ptr<operand>((operand*)(new varname(tempasmname, subout.type))));
				out += subout.src;
			}
		}

		assert(mathables.size() == 1);
		auto [src, storedpos] = std::get<std::shared_ptr<operand>>(mathables.back())->retrieve_asmvar();
		out += src;

		type = std::get<std::shared_ptr<operand>>(mathables.back())->get_type();
		computed_type = true;
		return std::make_pair(out, storedpos);
	}

	std::string get_type() override {
		if (!computed_type) retrieve_asmvar();
		return type;
	}
};

static std::string get_next_non_empty_token(bool allowEmpty = false);
static void process_code_body();
static std::pair<std::string, std::shared_ptr<expression>> get_next_expression();

static bool equivalent_grouping(std::string a, std::string b) {
	if (a == "(") return b == ")";
	if (a == ")") return b == "(";
	if (a == "]") return b == "]";
	if (a == "[") return b == "[";
	if (a == "{") return b == "}";
	if (a == "}") return b == "{";
	return false;
}

struct variable_assignment {
	std::string type_name;
	std::string var_name;
	std::string asm_name;
	std::pair<std::string, std::shared_ptr<expression>> expr;
};

static std::variant<std::pair<std::string, std::shared_ptr<expression>>, variable_assignment> get_expression_or_variable_assignment() {
	std::string current_token = get_next_non_empty_token();
	if (parser.is_type(current_token)) { // then we're defining a variable now.

		std::string var_name = get_next_non_empty_token();
		if (var_name.empty() || !parser.is_valid_symbol_name(var_name))
			throw std::runtime_error("invalid variable name");

		if (get_next_non_empty_token() != "=")
			throw std::runtime_error("expected \"=\" when defining variable");

		auto assignment = get_next_expression();

		return variable_assignment{
			.type_name = assignment.second->get_type(),
			.var_name = var_name,
			.asm_name = assignment.second->retrieve_asmvar().second,
			.expr = assignment,
		};
	}
	else {
		return_token(current_token);
		return get_next_expression();
	}
}

static std::string get_next_non_empty_token(bool allowEof) {
	while (true) {
		auto token = get_next_token();
		if (token == "")
			if (allowEof) return "";
			else throw std::runtime_error("unexpected eof");
		if (token != " " && token != "\t" && token != "\n") {
			//std::cout << "Token \"" + token + "\"\n";
			if (token == "function") {
				std::cout << "";
			}
			return token;
		}

	}
}

static void declare_variable(variable_assignment var) {
	//assert(var.var_name != "joe");
	parser.scopeStack.back().known_symbols[var.var_name] = symbol_type::variable;
	parser.scopeStack.back().variables[var.var_name] = std::make_shared<varname>(var.asm_name, var.type_name, var.var_name);
}

// can apparently (???) return an empty expression, may throw
static std::pair<std::string, std::shared_ptr<expression>> get_next_expression() {
	std::string expString = "";
	std::shared_ptr<expression> expression_parse = std::make_shared<expression>(std::vector<expression::token> {});
	std::vector<std::string> groupingSymbolStack = {};
	std::vector<int> last = { 0 }; // if 0, last was a binary operator (or nonexistent) so next better be a symbol, if 1, last was a symbol (so we better get a binary operand now)
	std::vector<bool> unary = { false };
	while (true) {
		std::string next = get_next_non_empty_token();
		if (next == "") {
			if (groupingSymbolStack.empty()) {
				return_token(next);
				break;
			}
			else
				throw std::runtime_error("unexpected <eof>");
		}
		else if (next == ";") {
			if (groupingSymbolStack.empty()) {
				return_token(next);
				break;
			}
			else
				throw std::runtime_error("unexpected \";\"");
		}
		else if (next == "{")
			throw std::runtime_error("unexpected \"{\"");
		else if (next == "(" || next == "[") {
			if (last.back() == 0) { // then its
				if (next == "[") throw std::runtime_error("unexpected \"[\" in expression");

				auto [str, subexpression] = get_next_expression();
				expString += str;
				expression_parse->tokens.push_back(subexpression);

				last.push_back(1);
				unary.push_back(false);
				/*groupingSymbolStack.push_back(next);
				last.push_back(0);
				unary.push_back(false);
				expString += next;*/
			}
			else {
				expString += next;
				assert(!expression_parse->tokens.empty());
				auto function = expression_parse->tokens.back();
				expression_parse->tokens.pop_back();
				std::shared_ptr<funccall> call = std::make_shared<funccall>();
				auto s = std::get<std::shared_ptr<operand>>(function)->retrieve_asmvar().second;
				call->function = parser.get_function_info(s);

				while (true) {
					auto [str, subexpression] = get_next_expression();
					if (!str.empty()) {
						expString += str;

						call->args.push_back(subexpression);

					}

					auto subnext = get_next_non_empty_token();
					expString += subnext;
					if (subnext == ",") {
						
					}
					else if ((subnext == ")" && next == "(") || (subnext == "]" && next == "[")) {
						break;
					}
					else {
						throw std::runtime_error("expected \",\" or closing grouping symbol, got \"" + subnext + "\".");
					}
				}
				
				expression_parse->tokens.push_back(call);
				unary.back() = false;
				last.back() = 1;

				if (call->function->arg_typenames.size() != call->args.size()) {
					throw std::runtime_error(std::string("expected ") + std::to_string(call->function->arg_typenames.size()) + " args, got " + std::to_string(call->args.size()) + " args instead");
				}
			}
		}
		else if (next == "}" || next == "]" || next == ")") {
			if (groupingSymbolStack.empty()) {
				return_token(next);
				break;
			}
			else if (equivalent_grouping(groupingSymbolStack.back(), next)) {
				assert(false);
				/*if (last.back() == 0 || unary.back()) throw std::runtime_error(std::string("expected symbol, got \"" + next + "\""));
				groupingSymbolStack.pop_back();
				last.pop_back();
				unary.pop_back();
				last.back() = 1;
				unary.back() = false;
				expString += next;*/
			}
			else {
				throw std::runtime_error(std::string("unexpected \"") + next + "\" to close " + groupingSymbolStack.back());
			}
		}
		else if (unary_operators.contains(next)) {
			if (unary.back()) throw std::runtime_error("unary operators cannot directly follow each other");
			if (last.back() == 1) throw std::runtime_error("unary operator cannot follow a symbol (expected a binary operator or end of the expression)");
			expString += next;
			expression_parse->tokens.push_back(unary_operators[next]);
			unary.back() = true;
		}
		else if (binary_operators.contains(next)) {
			if (unary.back()) throw std::runtime_error("binary operator should not follow unary operator");
			expString += next;
			expression_parse->tokens.push_back(binary_operators[next]);
			last.back() = 0;
		}
		else if (next == "function") {

			auto ret_type = get_next_non_empty_token();

			std::vector<std::string> argtypes;
			std::string func_type = ret_type + "(";

			parser.scopeStack.push_back(scope{ .type = scope_type::function, .should_return = ret_type == "void"});
			parser.taskStack.push_back(parsing_task_info{ .task = parsing_task::code_body, .line_number = tokenizer.current_line_number });

			if (!parser.is_type(ret_type)) throw std::runtime_error("unrecognized function return type \"" + ret_type + "\"");
			if (get_next_non_empty_token() != "(") throw std::runtime_error("expected \"(\" after declaring function return type");
			int argi = 0;
			while (true) {
				std::string next_arg = get_next_non_empty_token();
				func_type += next_arg;
				argtypes.push_back(next_arg);
				if (!parser.is_type(next_arg)) throw std::runtime_error("unrecognized function argument type \"" + next_arg + "\"");

				std::string arg_name = get_next_non_empty_token();
				if (!parser.is_valid_symbol_name(arg_name)) throw std::runtime_error("invalid argument name \"" + arg_name + "\"");

				std::string delimiter = get_next_non_empty_token();
				if (delimiter != ")" && delimiter != ",") {
					throw std::runtime_error("expected \"(\" or \",\" after function parameter");
				}
				else {
					auto v = std::make_shared<varname>("arg" + std::to_string(argi), next_arg, arg_name);
					auto e = std::make_shared<expression>(std::vector<expression::token> { v });
					variable_assignment assignment = {
						.type_name = next_arg,
						.var_name = arg_name,
						.asm_name = get_next_assembly_name(),
						.expr = std::make_pair(std::string("??FIJIWJI"), e)
					};
					declare_variable(assignment);
					func_type += delimiter;
					if (delimiter == ")")
						break;
				}
				argi++;
			}

			if (get_next_non_empty_token() != "{") throw std::runtime_error("expected \"{\" before function body");

			if (last.back() == 1) throw std::runtime_error("symbol cannot follow another symbol");
			unary.back() = false;
			last.back() = 1;
			expString += "<function_object>"; // TODO
			auto asm_funcname = get_next_assembly_name();
			parser.fmap[asm_funcname] = std::make_shared<function_info>(ret_type, func_type, argtypes, asm_funcname);
			auto func = std::make_shared<varname>(asm_funcname, func_type);
			expression_parse->tokens.push_back(func); // TODO: this function is anonymous 

			process_code_body();

			

		}
		else if (parser.is_variable(next) || parser.is_literal(next) || (!expString.empty() && expString.back() == '.')) { // dot operator doesn't want a variable name/literal
			if (last.back() == 1) throw std::runtime_error("symbol cannot follow another symbol");
			if (is_reserved(next)) throw std::runtime_error("\"" + next + "\" is invalid in this context");
			unary.back() = false;
			last.back() = 1;
			expString += next;
			
			if (next == "joe") {
				std::cout << "";
			}

			if (parser.is_variable(next)) {
				//if (!symbol_to_assembly_names.contains(next)) {
					//auto asmname = get_next_assembly_name();
					//symbol_to_assembly_names[next] = asmname;
				//}
				expression_parse->tokens.push_back(parser.get_variable(next));
			}
			else if (parser.is_literal(next).has_value()) {
				expression_parse->tokens.push_back(std::make_shared<literal>(*parser.is_literal(next), next)); // TODO
			}
			else {
				throw std::runtime_error("unimplemented");
				//expression_parse.operands.push_back(liter);// TODO: how to handle dot operator?
			}
		}
		else {
			// they gave something that doesn't go in the expression; if the expression is done that's fine, if it's not then we error
			if (groupingSymbolStack.empty() && !unary.back() && last.back() == 1) {
				return_token(next);
				break;
			}
			else {
				throw std::runtime_error(std::string("unexpected \"") + next + "\"");
			}
		}
	}

	expression_parse->shunting_yard();
	return std::make_pair(expString, expression_parse);
	};

// IGNORES SCOPE, THAT'S YOUR JOB
static void process_class_body() {
	int i = parser.taskStack.size();
	std::cout << "processing class block\n";
	std::string current_token;
	while (parser.taskStack.size() >= i) {
		assert(parser.taskStack[i - 1].task == parsing_task::class_body);

		if ((current_token = get_next_non_empty_token()) == "") {
			throw std::runtime_error("expected member declaration or \"}\", got <eof>");
			// program ended
		}

		// TODO: class bodies only have member declarations, no constructors
		if (current_token == "}") { // end class body
			parser.taskStack.pop_back();
		}
		else if (parser.is_type(current_token)) { // then we're defining a variable now.

			// check if we're defining a function type
			std::string isParen = get_next_non_empty_token();
			if (isParen == "(") {

				current_token += "(";
				// then this is hopefully a function type
				while (true) {
					std::string next_arg = get_next_non_empty_token();

					if (!parser.is_type(next_arg)) throw std::runtime_error("unrecognized function argument type \"" + next_arg + "\"");
					current_token += next_arg;

					// the type of the function obviously doesn't have argument names
					// 
					//std::string arg_name = get_next_non_empty_token();
					//if (!parser.is_valid_symbol_name(arg_name)) throw std::runtime_error("invalid argument name \"" + arg_name + "\"");
					//current_token += arg_name;

					std::string delimiter = get_next_non_empty_token();
					if (delimiter != ")" && delimiter != ",") {
						throw std::runtime_error("expected \"(\" or \",\" after function parameter");
					}
					else {
						current_token += delimiter;
						//parser.scopeStack.back().known_symbols[arg_name] = symbol_type::variable;
						if (delimiter == ")")
							break;
					}
				}

				assert(parser.is_type(current_token));
			}

			std::string var_name = isParen == "(" ? get_next_non_empty_token() : isParen;
			if (var_name.empty() || !parser.is_valid_symbol_name(var_name)) // TODO: naming should be more lax here
				throw std::runtime_error("invalid variable name");


			auto maybeEquals = get_next_non_empty_token();
			if (maybeEquals != "=") { // then that's fine; we'll give our own default value
				return_token(maybeEquals);
			}
			else {
				auto default_value_expression = get_next_expression();
			}
		}
		else if (current_token == ";") {} // ok whatever
		else {
			throw std::runtime_error("expected member declaration or \"}\", got \"" + current_token + "\".");
		}


	}
	std::cout << "exiting class block\n";
}

// IGNORES SCOPE, THAT'S YOUR JOB
static void process_code_body() {
	std::cout << "processing code block\n";
	int i = parser.taskStack.size();
	std::string current_token;

	while (parser.taskStack.size() >= i) {
		assert(parser.taskStack[i - 1].task == parsing_task::code_body);
		current_token = get_next_non_empty_token(true);
		if (current_token == "") {
			break; // program ended
		}
		// everything in a code body is either a return, a while loop, a for loop, an if statement, a class definition, a variable initialization + assignment, or an expression. (function definitions are expressions)
		if (current_token == "class") {
			std::string class_name = get_next_non_empty_token();
			if (class_name.empty() || !parser.is_valid_symbol_name(class_name))
				throw std::runtime_error("invalid class name");

			if (get_next_non_empty_token() != "{")
				throw std::runtime_error("expected \"{\" after class name");

			parser.scopeStack.back().known_symbols[class_name] = symbol_type::type;
			parser.taskStack.push_back(parsing_task_info{ .task = parsing_task::class_body });

			process_class_body();
		}
		else if (current_token == "return") {
			if (parser.scopeStack.back().should_return) {
				auto return_expression = get_next_expression();
				auto [asmt, varname] = return_expression.second->retrieve_asmvar();
				out += asmt;
				out += "\ndvar " + return_asmvar += " sym:" + varname;
				out += "\njmp function_end_lbl";
			}
		}
		else if (current_token == "while") {
			if (get_next_non_empty_token() != "(")
				throw std::runtime_error("expected \"(\" before while loop header");

			auto loop_condition = get_next_expression();

			if (get_next_non_empty_token() != ")")
				throw std::runtime_error("expected \")\" to close while loop header");

			if (get_next_non_empty_token() != "{")  // TODO: one-expression loops/ifs without brackets?
				throw std::runtime_error("expected \"{\" after while loop header");

			parser.taskStack.push_back(parsing_task_info{ parsing_task::code_body, -1 });
			parser.scopeStack.push_back(scope{ .known_symbols = {}, .type = scope_type::while_ });
		}
		else if (current_token == "else") {
			throw std::runtime_error("invalid else");
		}
		else if (current_token == "elseif") {
			throw std::runtime_error("invalid elseif");
		}
		else if (current_token == "for") {
			std::cout << "Parsing for loop.\n";

			if (get_next_non_empty_token() != "(")
				throw std::runtime_error("expected \"(\" before for loop header");

			parser.taskStack.push_back(parsing_task_info{ parsing_task::code_body, -1 }); // make sure for loop's defined variable is part of this scope, not the outer scope
			parser.scopeStack.push_back(scope{ .known_symbols = {}, .type = scope_type::for_ });

			auto loop_initial = get_expression_or_variable_assignment();
			if (get_next_non_empty_token() != ",")
				throw std::runtime_error("expected \",\" between for loop header initial expression and conditional expression");
			if (std::holds_alternative<variable_assignment>(loop_initial)) {
				declare_variable(std::get<variable_assignment>(loop_initial));
			}

			auto loop_condition = get_next_expression();
			if (get_next_non_empty_token() != ",")
				throw std::runtime_error("expected \",\" between for loop header conditional expression and iteration expression");
			auto loop_increment = get_next_expression();

			if (get_next_non_empty_token() != ")")
				throw std::runtime_error("expected \")\" to close for loop header");

			if (get_next_non_empty_token() != "{")  // TODO: one-expression loops/ifs without brackets?
				throw std::runtime_error("expected \"{\" after for loop header");


		}
		else if (current_token == "if") {

			if (get_next_non_empty_token() != "(")
				throw std::runtime_error("expected \"(\" before if condition");

			auto if_condition = get_next_expression();
			// TODO: ASSERT CONVERTIBILITY TO BOOLEAN



			if (get_next_non_empty_token() != ")")
				throw std::runtime_error("expected \")\" to close if condition");


			if (get_next_non_empty_token() != "{")  // TODO: one-expression loops/ifs without brackets?
				throw std::runtime_error("expected \"{\" after if statement");

			parser.taskStack.push_back(parsing_task_info{ parsing_task::code_body, -1 });
			parser.scopeStack.push_back(scope{ .known_symbols = {}, .type = scope_type::if_ });
		}
		else if (current_token == "}") { // exit code body
			bool could_have_else = parser.scopeStack.back().type == scope_type::if_;
			parser.taskStack.pop_back();
			parser.scopeStack.pop_back();
			if (parser.taskStack.empty()) throw std::runtime_error("expected <eof>, got \"}\"");

			if (could_have_else) {
				auto next = get_next_non_empty_token();
				if (next == "else") {
					//if (!could_have_else) throw std::runtime_error("invalid else");

					if (get_next_non_empty_token() != "{")  // TODO: one-expression loops/ifs without brackets?
						throw std::runtime_error("expected \"{\" after else statement");

					parser.taskStack.push_back(parsing_task_info{ parsing_task::code_body, -1 });
					parser.scopeStack.push_back(scope{ .known_symbols = {}, .type = scope_type::else_ });
				}
				else if (next == "elseif") {
					//if (!could_have_else) throw std::runtime_error("invalid elseif");

					auto if_condition = get_next_expression();
					// TODO: ASSERT CONVERTIBILITY TO BOOLEAN

					if (get_next_non_empty_token() != "{")  // TODO: one-expression loops/ifs without brackets?
						throw std::runtime_error("expected \"{\" after if statement");

					parser.taskStack.push_back(parsing_task_info{ parsing_task::code_body, -1 });
					parser.scopeStack.push_back(scope{ .known_symbols = {}, .type = scope_type::if_ });
				}
				else {
					return_token(next);
				}
			}
			else if (parser.scopeStack.back().type == scope_type::function) {
				out += "\nlabel function_end_lbl";
			}
		}
		else if (current_token == ";") {}
		else {
			return_token(current_token);

			auto variant = get_expression_or_variable_assignment();

			if (std::holds_alternative<variable_assignment>(variant))4 {

				declare_variable(std::get<variable_assignment>(variant));
				auto [asmcode, asmvar] = std::get<variable_assignment>(variant).expr.second->retrieve_asmvar();
				out += asmcode + "\ndvar " + std::get<variable_assignment>(variant).asm_name + " sym:" + asmvar;
			}
			else {
				out += std::get<0>(variant).second->retrieve_asmvar().first;
				
			}
		}
	}
	std::cout << "exiting code block\n";
}

int main(const char** args, int nargs) {
	
	parser.taskStack.push_back(parsing_task_info { parsing_task::code_body, -1 });
	parser.scopeStack.push_back(scope {
		.known_symbols = {
			{"i32", symbol_type::type},
			{"f64", symbol_type::type},
			{"string", symbol_type::type},
			{"void", symbol_type::type},
			{"bool", symbol_type::type},
			{"var", symbol_type::type}
		},
		.type = scope_type::main
	});

	

	src = ";\n;\n;\n;" + get_src("test1.tla");
	out = "";

	

	

	//try {
		std::string current_token;
		while ((current_token = get_next_non_empty_token(true)) != "") {
			return_token(current_token);
			assert(!parser.taskStack.empty());

			//continue;

			std::cout << "bruh";
			

			if (parser.taskStack.back().task == parsing_task::code_body) {
				process_code_body();
			}
			else if (parser.taskStack.back().task == parsing_task::class_body) {
				process_class_body();
			}

		}
	//}
	//catch (std::exception& exception) {
		//std::cout <<  "\n\ncompilation error : " << "(" << tokenizer.current_line_number << ") " << exception.what() << "\n";
		//throw exception;
		//return EXIT_FAILURE;
	//}
	assert(src.empty());

	std::cout << "\nOUTPUT:\n\n" << out;

	return EXIT_SUCCESS;
}

std::pair<std::string, std::string> funccall::retrieve_asmvar()  {
	std::string prep_asm = "";
	std::string cfunc_asm = "\ncfunc " + function->assemblyfuncname + " ";
	if (function->arg_typenames.size() != args.size()) {
		throw std::runtime_error(std::string("expected ") + std::to_string(function->arg_typenames.size()) + " args, got " + std::to_string(args.size()) + " args instead");
	}
	if (function->arg_typenames.size() == 0) {
		cfunc_asm += "null";
	}
	else {
		for (int i = 0; i < function->arg_typenames.size(); i++) {
			// try to convert each arg to the desired type
			auto [prepcode, arglocationname] = args[i]->retrieve_asmvar();
			if (function->arg_typenames[i].back() == '&') { // then this is pass by reference; can't do any conversions, must be exact type
				if (function->arg_typenames[i] != args[i]->get_type()) {
					throw std::runtime_error(std::string("error: mismatched types at argument #" + std::to_string(i + 1)));
				}

				prep_asm += prepcode;
				cfunc_asm += "sym:" + arglocationname;
			}
			else {
				auto copiedarg = "arg_temp" + std::to_string(i);
				auto conversion_asm = implicit_convert_to_type(copiedarg, function->arg_typenames[i], args[i]->get_type());
				if (!conversion_asm.has_value()) {
					throw std::runtime_error(std::string("error: mismatched types at argument #" + std::to_string(i + 1) + " and no valid implicit conversion exists"));
				}

				prep_asm += prepcode + copy(copiedarg, arglocationname) + *conversion_asm;
			}

			cfunc_asm += "/";
		}
		cfunc_asm.pop_back();
	}
	return std::make_pair(prep_asm + cfunc_asm, return_asmvar);
}