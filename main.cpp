#include <iostream>
#include <fstream>
#include <sstream>

// lexer?
struct tokenizer_context {
	bool in_string_literal;
	bool in_long_comment;
	bool in_line_comment;
};

std::stringstream get_src(std::string path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cout << "Failure to open " << path << "\n";
		throw std::runtime_error("Invalid path " + path);
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	file.close();

	return buffer;
}

std::string get_next_token(std::stringstream& src, tokenizer_context& context) {
	std::string token = "";
	bool escape_next_char = false;
	bool last_char_was_asterisk = false;
	while (true) {
		char c = src.get();
		if (context.in_string_literal) {
			if (c == '\\' && !escape_next_char) {
				escape_next_char = true;
			}
			else {
				token += c;

				if (c == '\"' && !escape_next_char) {
					context.in_string_literal = false;
					break;
				}
			}
		}
		else if (context.in_line_comment) {
			if (c == '\n') {
				src.unget();
				break;
			}
		}
		else if (context.in_long_comment) {
			if (c == '*')
				last_char_was_asterisk = true;

		}
		else {
			switch (c)
			case ' ':
			case '(':
			case ')':
			case '\t':
			case '\n':
			case ';':
			case ':':
			case '[':
			case ']':
			case '{':
			case '}':
			case ',':
			case '.':
				if (token.empty()) {
					return std::to_string(c);
				}
				else {
					src.unget();
					return token;
				}
			break;
		}

		escape_next_char = false;
	}

	return token;
}

int main(const char** args, int nargs) {
	std::stringstream src = get_src();

	while (get_next_token() != "") {

	}

	return EXIT_SUCCESS;
}