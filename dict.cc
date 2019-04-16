// -*- compile-command: "c++ -Wall -std=c++11 spell.cc" -*-
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <cctype>
#include <array>




//The code works with utf-8 and ASCII language texts. For other languages/codepages input should be converted to
//fixed size character set. The HTMLs processing is weak. It's an imaginary HTML format actually. The code is
//case sensitive in any sense. 

//For finding word in dictionary let's use hash value for strings which is
//resetted on any delimiters in input text. The value is then searched in dictionary hash table.
//For real production application the type of hash value and the hash algorithm should be carefully
//selected.

unsigned
char_hash(char c, unsigned hval)
{
	const size_t prime = 2971215073;
	hval = (hval*6967 + c) % prime;
	return hval;
}

unsigned
str_hash(const std::string& s)
{
	unsigned hval = 0;
	for(char c: s)
		hval = char_hash(c, hval);
	return hval;
}

//This data type scale pretty well for dictionary increase. For real big dictionary and fast initialization
//one can use some sort of hash tables with memory mapped files support.
using dict_t=std::unordered_set<unsigned>;


class collector {
public:
	collector(const dict_t& dict_, std::ostream& o, int max_len): current_word(), s_hash(), dict(dict_),
								      ofile(o), max_dict_len(max_len) {
		current_word.reserve(5);
	}
	void operator() (char c, bool isword) {
		if (!isword) {
			if (have_word()) {
				print_word();
			}
			print_char(c);
		} else {
			add_char(c);
		}
	}
	void print_char(char c) {
		ofile<<c;
	}
	
	void print_word() {
		if (s_hash!=BAD_HASH) {
			bool dict_word = dict.count(s_hash);
			if (dict_word)
				ofile<<PREFIX;
			ofile<<current_word;
			if (dict_word)
				ofile<<SUFFIX;
		}
		current_word.clear();
		s_hash=0;
	}
	
	void add_char(char c) {
		if (s_hash == BAD_HASH) {
			print_char(c);
			return;
		}
		current_word +=c;
		if (current_word.size()>max_dict_word) {
			ofile<<current_word;
			s_hash = BAD_HASH;
		} else {
			s_hash = char_hash(c, s_hash);
		}
	}
	
	bool have_word(void) {
		return !current_word.empty();
	}
private:	
	std::string current_word;
	unsigned s_hash;
	const dict_t& dict;
	std::ostream& ofile;
	int max_dict_len;
	static const unsigned int BAD_HASH = 0xffffffff;  
	static const std::string SUFFIX = "</i>";
	static const std::string PREFIX = "<i class=\"" "src\">";
};


//It's another approach for finding substring in a O(m+n) time. 

class special_state {
public:
	// states 0 - 's', 1 - 't', 2 - 'y', 3 - 'l', 4 - 'e', 5 - 'c', 6 - 'r',
	// 7 - 'i', 8 - 'p', 9 - any other letter, 10 - delimiters (initial state)
	// 11 - foundstate, 12 - 't' in script
	special_state(): state(10)
		{}
	void reset() {
		state = 10;
	}
	bool next_char(char c) {
		int next_state = std::isalnum(c)?9:10;
		if (next_state == 9) {
			//Linear search in the 10 bytes array is ok way.
			//For larger size arrays a hash or an indexed table should be used. 
			next_state = good_chars.find(c);
			if (next_state == good_chars.npos)
				next_state = 9;
		}
		state = fsm[state][next_state];
		return state == 11;
	}
	int get_state() {
		return state;
	}
private:
		// "style" and "script"
		static const std::array<std::array<int, 11>, 13> fsm = {{
			/*                  s  t y l e c r i p X D  !! 2t*/ 
			/* next_state ->    0  1 2 3 4 5 6 7 8 9 10 11 12*/         
			/* 0 - 's'    */  {{9, 1,9,9,9,5,9,9,9,9,10 }},
			/* 1 - 't'    */  {{9, 9,2,9,9,9,9,9,9,9,10 }},
			/* 2 - 'y',   */  {{9, 9,9,3,9,9,9,9,9,9,10 }},
			/* 3 - 'l'    */  {{9, 9,9,9,4,9,9,9,9,9,10 }},
			/* 4 - 'e'    */  {{9, 9,9,9,9,9,9,9,9,9,11 }},
			/* 5 - 'c'    */  {{9, 9,9,9,9,9,6,9,9,9,10 }},
			/* 6 - 'r'    */  {{9, 9,9,9,9,9,9,7,9,9,10 }},
			/* 7 - 'i'    */  {{9, 9,9,9,9,9,9,9,8,9,10 }},
			/* 8 - 'p'    */  {{9,12,9,9,9,9,9,9,9,9,10 }},
			/* 9 - any    */  {{9, 9,9,9,9,9,9,9,9,9,10 }},
			/* 10 - delim */  {{0, 9,9,9,9,9,9,9,9,9,10 }},
			/* 11 - found */  {{11,11,11,11,11,11,11,11,11,11,11}},
			/* 12 - sc...t */ {{9, 9,9,9,9,9,9,9,9,9,11}}
			}};

	static const std::string good_chars= "stylecrip";
	int state;
};

template<typename C>
class html_reader {
public:
	html_reader(const C& collect_): collect(collect_)
		{}

	void read(std::istream& in) {
		bool in_tag = false;
		bool in_quote = false;
		bool skip_mode = false;
		char c;
		while(in.get(c)) {
			bool is_text = false;
			if (!in_tag) {
				if (c == '<') {
					in_tag = true;
					is_style_or_script.reset();
				} else if (std::isalnum(c))
					is_text = true;
			} else {
				if (!in_quote && c == '>')
					in_tag = false;
				else if (!in_quote && c == '\"')
					in_quote = true;
				else if (in_quote) {
					if (c == '\"')
						in_quote = false;
				}
				
				if (is_style_or_script.get_state() != 11 &&
				    is_style_or_script.next_char(in_quote?'?':c)) {
					skip_mode = !skip_mode;
				}
			}
			
			collect(c, skip_mode?false:is_text);
		}
	}
private:
	C collect;
	special_state is_style_or_script;
};

// 
template<typename C>
class doc_reader {
public:
	doc_reader(const C& collect_): collect(collect_)
		{}
	void read(std::istream& in) {
		char c;
		while(in.get(c)) {
			if (std::isalnum(c))
				collect(c, true);
			else
				collect(c, false);
		}
	}
private:
	C collect;
};


int
main(int ac, char* av[])
{
	if (ac!=3) {
		std::cerr<<"Usage: "<<av[0]<<": dictionary text|html"<<std::endl;
		return 1;
	}
	std::ifstream dfile(av[1]);
	if (!dfile) {
		std::cerr<<"Can't open dictionary file."<<std::endl;
		return 1;
	}
	std::string word;
	dict_t dic;
	int max_len = 0;
	while(dfile>>word) {
		dic.insert(str_hash(word));
		max_len = std::max(max_len, word.size());
	};

	if (av[2]==std::string("text")) {
		doc_reader<collector> rd(collector(dic, std::cout, max_len));
		rd.read(std::cin);
	} else if (av[2] == std::string("html")) {
		html_reader<collector> rd(collector(dic, std::cout, max_len));
		rd.read(std::cin);
	}
		
	return 0;
}
