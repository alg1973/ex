// -*- compile-command: "c++ -Wall -std=c++14 dict-trie.cc" -*-
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <cctype>
#include <array>
#include <memory>
#include <cassert>


//The code works with utf-8 and ASCII language texts. For other languages/codepages input should be converted to
//fixed size character set. The HTMLs processing is weak. It's an imaginary HTML format actually. The code is
//case sensitive in any sense. 


//
// This works only for a-Za-z0-9
// 

class trie_node {
public:
	trie_node(): w_end(false),next({{}})
		{}
	trie_node* get(char c) {
		return next[char2index(c)].get();
	}
	trie_node* put(char c) {
		return (next[char2index(c)] = std::make_unique<trie_node>()).get();
	}
	bool end() {
		return w_end;
	}
	bool end(bool w_e) {
		return w_end=w_e;
	}
private:
	int char2index(char c) {
		int index_val=-1;
		if (std::isdigit(c)) {
			index_val = c-'0';
		} else if (std::islower(c)) {
			index_val =  c-'a'+10;
		} else if (std::isupper(c)) {
			index_val = c-'A'+36;
		}
		assert(index_val>=0 && index_val<62);
		return index_val;
	}
	bool w_end;
	std::array<std::unique_ptr<trie_node>,62> next;
};

class trie {
public:
	trie(): root(std::make_unique<trie_node>()), cur_node(root.get()) {	
        }
        void add(const std::string& w) {
		if (w.empty())
			return;
                trie_node *r = root.get();
                for(char c: w) {
                        trie_node* n = r->get(c);
                        if (!n) {
                                n = r->put(c);
                        }
                        r = n;
                }
                r->end(true);
        }
	void reset() {
		cur_node = root.get();
	}
	bool next_char(char c) {
		if (cur_node == nullptr)
			return false;
		cur_node = cur_node->get(c);
		return !had_mismatch();
	}
	bool had_mismatch() const {
		return cur_node == nullptr;
	}
	bool is_match() const {
		return cur_node !=nullptr && cur_node->end();
	}
private:
	std::unique_ptr<trie_node> root;
	//It's the state node of the current word traversal.
	trie_node* cur_node;
};


using dict_t=trie;

class collector {
public:
	collector(dict_t& dict_, std::ostream& o): current_word(), dict(dict_), ofile(o) {
		//current word could grow to only length of the longest dictionary word. 
		current_word.reserve(5);
	}
	void flash() {
		if (have_word()) {
			print_word();
		}
	}
	void operator() (char c, bool isword) {
		if (!isword) {
			flash();
			print_char(c);
		} else {
			add_char(c);
		}
	}
	void print_char(char c) {
		ofile<<c;
	}
	
	void print_word() {
		if (!dict.had_mismatch()) {
			bool dict_word = dict.is_match();
			if (dict_word)
				ofile<<PREFIX;
			ofile<<current_word;
			if (dict_word)
				ofile<<SUFFIX;
		}
		current_word.clear();
		dict.reset();
	}
	
	void add_char(char c) {
		if (dict.had_mismatch()) {
			print_char(c);
			return;
		}
		//current_word could grow only to length of longest word in the dictionary plus 1 
		current_word +=c;
		if (!dict.next_char(c)) { //Mismatch.
			ofile<<current_word;
		}
	}
	
	bool have_word(void) {
		return !current_word.empty();
	}
private:	
	std::string current_word;
	dict_t& dict;
	std::ostream& ofile;
	const std::string SUFFIX = "</i>";
	const std::string PREFIX = "<i class=\"" "src\">";
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
		const std::array<std::array<int, 11>, 13> fsm = {{
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

	const std::string good_chars= "stylecrip";
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
		collect.flash();
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
		collect.flash();
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
	while(dfile>>word) {
		dic.add(word);
	};

	if (av[2]==std::string("text")) {
		doc_reader<collector> rd(collector(dic, std::cout));
		rd.read(std::cin);
	} else if (av[2] == std::string("html")) {
		html_reader<collector> rd(collector(dic, std::cout));
		rd.read(std::cin);
	}
		
	return 0;
}
