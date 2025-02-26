#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
#include <unordered_map>
#include <functional>
#include <regex>
#include <variant>

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

struct scope {
	std::unordered_map<std::string, symbol_type> known_symbols;
	scope_type type;
	bool should_return = false;
};

struct parsing_task_info {
	parsing_task task;
	int line_number = -1;
};

struct parser_context {
	std::vector<parsing_task_info> taskStack;
	std::vector<scope> scopeStack;

	bool is_literal(std::string literal) {
		// boolean literal
		if (literal == "true" || literal == "false") return true;

		// null literal
		if (literal == "null") return true;

		// string literal
		if (literal.size() >= 2 && literal[0] == '\"' && literal.back() == '\"') return true;

		// numeric literal (stolen from https://medium.com/@ryan_forrester_/c-check-if-string-is-number-practical-guide-c7ba6db2febf)
		static const std::regex number_regex(
			R"(^[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?$)"
		);
		return std::regex_match(literal, number_regex);
	}

	bool is_variable(std::string symbol) {
		for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); it++) {
			if (it->known_symbols.count(symbol) && it->known_symbols[symbol] == symbol_type::variable) {
				return true;
			}
		}
		return false;
	}

	bool is_basic_type(std::string symbol) {
		while (!symbol.empty() && symbol.back() == '&') symbol.pop_back();
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
};

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

struct operator_ {
	bool unary = false;
	associativity a = associativity::left_to_right;
	int priority = 100; // highest first
};

std::unordered_map<std::string, operator_> unary_operators{
	//{"-", operator_ {.unary = true}},
	{"!", operator_ {.unary = true}}
};

// see https://en.cppreference.com/w/cpp/language/operator_precedence
std::unordered_map<std::string, operator_> binary_operators {
	
	{".", operator_ {.unary = false, .priority = 80}},

	{"*", operator_ {.unary = false, .priority = 70}},
	{"/", operator_ {.unary = false, .priority = 70}},
	{"%", operator_ {.unary = false, .priority = 70}},
	
	{"+", operator_ {.unary = false, .priority = 60}},
	{"-", operator_ {.unary = false, .priority = 60}},

	{">=", operator_ {.unary = false, .priority = 50}},
	{"<=", operator_ {.unary = false, .priority = 50}},
	{"<", operator_ {.unary = false, .priority = 50}},
	{">", operator_ {.unary = false, .priority = 50}},

	{"==", operator_ {.unary = false, .priority = 40}},
	{"!=", operator_ {.unary = false, .priority = 40}},

	{"&&", operator_ {.unary = false, .priority = 30}},

	{"||", operator_ {.unary = false, .priority = 20}},

	{"=", operator_ {.unary = false, .a = right_to_left, .priority = 10}},
	{"+=", operator_ {.unary = false, .a = right_to_left, .priority = 10}},
	{"-=", operator_ {.unary = false, .a = right_to_left, .priority = 10}},
	{"*=", operator_ {.unary = false, .a = right_to_left, .priority = 10}},
	{"/=", operator_ {.unary = false, .a = right_to_left, .priority = 10}},
	{"%=", operator_ {.unary = false, .a = right_to_left, .priority = 10}},
};

//std::unordered_map<std::string, operator_> post_operators{
//	{"(", operator_ {.unary = true, .priority = 120} }, // function call
//	{"[", operator_ {.unary = true, .priority = 120} }, // subscript
//
//}

tokenizer_context tokenizer;
parser_context parser;
std::string src, out;

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

class expression {

};

static std::string get_next_non_empty_token(bool allowEmpty = false);
static void process_code_body();
static std::pair<std::string, expression> get_next_expression();

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
	std::pair<std::string, expression> expr;
};

static std::variant<std::pair<std::string, expression>, variable_assignment> get_expression_or_variable_assignment() {
	std::string current_token = get_next_non_empty_token();
	if (parser.is_type(current_token)) { // then we're defining a variable now.

		std::string var_name = get_next_non_empty_token();
		if (var_name.empty() || !parser.is_valid_symbol_name(var_name))
			throw std::runtime_error("invalid variable name");

		if (get_next_non_empty_token() != "=")
			throw std::runtime_error("expected \"=\" when defining variable");

		auto assignment = get_next_expression();

		return variable_assignment{
			current_token,
			var_name,
			assignment
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
	parser.scopeStack.back().known_symbols[var.var_name] = symbol_type::variable;
}

// can NOT return an empty expression, may throw
static std::pair<std::string, expression> get_next_expression() {
	std::string exp = "";
	expression expression_tree;
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
				groupingSymbolStack.push_back(next);
				last.push_back(0);
				unary.push_back(false);
				exp += next;
			}
			else {
				exp += next;
				while (true) {
					auto [str, subexpression] = get_next_expression();
					exp += str;

					auto subnext = get_next_non_empty_token();
					exp += subnext;
					if (subnext == ",") {
						
					}
					else if ((subnext == ")" && next == "(") || (subnext == "]" && next == "[")) {
						break;
					}
					else {
						throw std::runtime_error("expected \",\" or closing grouping symbol, got \"" + subnext + "\".");
					}
				}
				
				unary.back() = false;
				last.back() = 1;
			}
		}
		else if (next == "}" || next == "]" || next == ")") {
			if (groupingSymbolStack.empty()) {
				return_token(next);
				break;
			}
			else if (equivalent_grouping(groupingSymbolStack.back(), next)) {
				if (last.back() == 0 || unary.back()) throw std::runtime_error(std::string("expected symbol, got \"" + next + "\""));
				groupingSymbolStack.pop_back();
				last.pop_back();
				unary.pop_back();
				last.back() = 1;
				unary.back() = false;
				exp += next;
			}
			else {
				throw std::runtime_error(std::string("unexpected \"") + next + "\" to close " + groupingSymbolStack.back());
			}
		}
		else if (unary_operators.contains(next)) {
			if (unary.back()) throw std::runtime_error("unary operators cannot directly follow each other");
			exp += next;
			if (last.back() == 1) throw std::runtime_error("unary operator cannot follow a symbol (expected a binary operator or end of the expression)");
			unary.back() = true;
		}
		else if (binary_operators.contains(next)) {
			if (unary.back()) throw std::runtime_error("binary operator should not follow unary operator");
			exp += next;
			last.back() = 0;
		}
		else if (next == "function") {

			auto ret_type = get_next_non_empty_token();

			parser.scopeStack.push_back(scope{ .type = scope_type::function, .should_return = ret_type == "void"});
			parser.taskStack.push_back(parsing_task_info{ .task = parsing_task::code_body, .line_number = tokenizer.current_line_number });

			if (!parser.is_type(ret_type)) throw std::runtime_error("unrecognized function return type \"" + ret_type + "\"");
			if (get_next_non_empty_token() != "(") throw std::runtime_error("expected \"(\" after declaring function return type");
			while (true) {
				std::string next_arg = get_next_non_empty_token();

				if (!parser.is_type(next_arg)) throw std::runtime_error("unrecognized function argument type \"" + next_arg + "\"");

				std::string arg_name = get_next_non_empty_token();
				if (!parser.is_valid_symbol_name(arg_name)) throw std::runtime_error("invalid argument name \"" + arg_name + "\"");

				std::string delimiter = get_next_non_empty_token();
				if (delimiter != ")" && delimiter != ",") {
					throw std::runtime_error("expected \"(\" or \",\" after function parameter");
				}
				else {
					parser.scopeStack.back().known_symbols[arg_name] = symbol_type::variable;
					if (delimiter == ")")
						break;
				}
			}

			if (get_next_non_empty_token() != "{") throw std::runtime_error("expected \"{\" before function body");

			if (last.back() == 1) throw std::runtime_error("symbol cannot follow another symbol");
			unary.back() = false;
			last.back() = 1;
			exp += "<function_object>"; // TODO

			process_code_body();

			

		}
		else if (parser.is_variable(next) || parser.is_literal(next) || (!exp.empty() && exp.back() == '.')) { // dot operator doesn't want a variable name/literal
			if (last.back() == 1) throw std::runtime_error("symbol cannot follow another symbol");
			if (is_reserved(next)) throw std::runtime_error("\"" + next + "\" is invalid in this context");
			unary.back() = false;
			last.back() = 1;
			exp += next;
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

	return std::make_pair(exp, expression_tree);
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
		}
		else if (current_token == ";") {}
		else {
			return_token(current_token);

			auto variant = get_expression_or_variable_assignment();

			if (std::holds_alternative<variable_assignment>(variant)) {
				declare_variable(std::get<variable_assignment>(variant));
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
			{"f32", symbol_type::type},
			{"string", symbol_type::type},
			{"void", symbol_type::type},
			{"bool", symbol_type::type},
			{"var", symbol_type::type}
		},
		.type = scope_type::main
	});

	

	src = ";" + get_src("test1.tla");
	out = "";

	

	

	//try {
		std::string current_token;
		while ((current_token = get_next_non_empty_token(true)) != "") {
			return_token(current_token);
			assert(!parser.taskStack.empty());

			//continue;

			
			

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