// -*- compile-command: "c++ -g -O0 -Wall -std=c++11 spell.cc" -*-

#include <iostream>
#include <iterator>
#include <array>
#include <vector>
#include <fstream>
#include <string>
#include <cctype>
#include <unordered_map>
#include <memory>

namespace spell {
	
using char_t=char;
using trigram_t=std::array<char_t,3>;
using string_t=std::basic_string<char_t>;

const int MAX_STRING=50;
const int DEFAULT_DISTANCE=1;
	
struct Dummy_norm {
	//Normalization for computing edit distance.
	//Processing lower & upper case, first capital letter etc.
	string_t operator()(const string_t& in) {
		return in;
	}
	//De-normalization for output word substitution.
	string_t out(const string_t& dict_word, const string_t& orig_word) {
		return dict_word;
	}
};
	
// Functor for output words ranking. Best word comes first. One way to do it is to use modified
// edit_distant function with different weight for addition, deletion and replacement.
struct Dummy_rank {
	void operator()(std::vector<string_t>& candidates, const string_t& orig_word) {
		
	}
	int operator()(const string_t& word, const string_t& dict_word) {
		return 1;
	}
};


class Edit_distance {
public:
	Edit_distance(unsigned int max_s): max_s_(max_s), mem(max_s+1,std::vector<int>(max_s+1,0))
		{}
	int operator()(const string_t& pattern, const string_t& text) {
		if (pattern.size()> max_s_ || std::max(pattern.size(),text.size())-std::min(pattern.size(),text.size())>1)
			return -1;
		return compute(pattern, pattern.size(), text, text.size());
	}
private:
	int compute(const string_t& ps, int plen, const string_t& ts, int tlen) {

		mem[0][0] = 0;
		for(int p = 1; p < plen+1; ++p)
			mem[p][0] = mem[p-1][0] + 1;
		for(int t = 1; t < tlen+1; ++t)
			mem[0][t] = mem[0][t-1] + 1;

		for(int t = 1; t < tlen + 1; ++t)
			for(int p = 1; p < plen + 1; ++p)
				if (ps[p-1] == ts[t-1])
					mem[p][t] = mem[p-1][t-1];
				else {
					int change = mem[p-1][t-1]+1;
					int del = mem[p-1][t]+1;
					int add = mem[p][t-1]+1;
					mem[p][t] = std::min(change,std::min(del, add));
				}
		return mem[plen][tlen];
	}
private:
	unsigned int max_s_;
	std::vector<std::vector<int>> mem;
};

class Dict_plain {
public:
	Dict_plain(const string_t& file) {
		std::ifstream i;
		i.exceptions(std::ifstream::badbit);
		i.open(file);
		std::copy(std::istream_iterator<string_t>(i), std::istream_iterator<string_t>(),
			  std::back_inserter(dict));
		std::sort(dict.begin(), dict.end());
	};
	using Dict_iter=std::vector<string_t>::const_iterator;
	std::pair<Dict_iter,Dict_iter> get_range(const string_t& word) const {
		return std::make_pair(dict.begin(), dict.end());
	}
	bool exist(const string_t& word) {
		return std::binary_search(dict.begin(),dict.end(), word);
	}
protected:
	const std::vector<string_t>& words(void) const {
		return dict;
	}
private:
	std::vector<string_t> dict;
};


struct trigram_hash {
	size_t operator()(const trigram_t& tri) const {
		return (std::hash<char_t>()(tri[0])*31+std::hash<char_t>()(tri[1]))*31+std::hash<char_t>()(tri[2]);
	}
};



class Dict_trigram: public Dict_plain {
public:
	Dict_trigram(const string_t& file) : Dict_plain(file) {
		auto ptr = Dict_plain::get_range("");
		for(int i=0; ptr.first!=ptr.second;  ++ptr.first, ++i) {
			auto tri_v = gen_trigrams(*ptr.first);
			for(auto& tri: *tri_v) {
				tri_hash[tri].push_back(i);
			}
		}
		std::cerr<<"Buckets: "<<tri_hash.bucket_count()<<" load factor: "<<tri_hash.load_factor()<<std::endl;
	}
	class Iter {
	public:
		Iter(std::vector<trigram_t>::iterator i, const  Dict_trigram* dict_,
		     std::shared_ptr<std::vector<trigram_t>> t, unsigned int off=0): it(i), offset(off),
									    dict(dict_), trigrams(t) {
			skip_absent();
		}
		Iter& operator++() {
			++offset;
			if (offset >= dict->tri_hash.at(*it).size()) {
				++it;
				skip_absent();
				offset=0;
			}
			return *this;
		}
		const string_t& operator*() const {
			return (dict->words())[dict->tri_hash.at(*it)[offset]];
		}
		bool operator==(const Iter& l) const {
			return it == l.it && offset == l.offset;
		}
		bool operator!=(const Iter& l) const {
			return !(*this == l);
		}
		void skip_absent() {
			if (it == trigrams->end())
				return;
			while(it!=trigrams->end() && dict->tri_hash.find(*it)==dict->tri_hash.end())
				++it;
		}
	private:
		std::vector<trigram_t>::iterator it;
		unsigned int offset;
		const Dict_trigram* dict;
		std::shared_ptr<std::vector<trigram_t>> trigrams;
	};

	std::pair<Iter,Iter> get_range(const string_t& word) const {
		auto tri_v = gen_trigrams(word);
		return std::make_pair(Iter(tri_v->begin(), this, tri_v),
				      Iter(tri_v->end(), this, tri_v));
	}

	static std::shared_ptr<std::vector<trigram_t>> gen_trigrams(const string_t& w) {
		auto result = std::make_shared<std::vector<trigram_t>>();
		result->reserve(w.size());
		
		if (w.size()==0)
			return result;
		// Put all 1`s and 2`s letters words in the special bucket
		if (w.size() == 1 || w.size() == 2)
			result->emplace_back(trigram_t({{'$','$','$'}}));
		if (w.size()==1) {
			result->emplace_back(trigram_t({{'$',w[0],'$'}}));
		} else {
			for(size_t i = 0; i<w.size()-2; ++i) {
				result->emplace_back(trigram_t({{w[i],w[i+1],w[i+2]}}));
			}
			result->emplace_back(trigram_t({{'$',w[0],w[1]}}));
			result->emplace_back(trigram_t({{w[w.size()-2],w[w.size()-1],'$'}}));
		}
		return result;
	}
private:
	std::unordered_map<trigram_t,std::vector<int>,trigram_hash> tri_hash;
};



template<typename Dict_impl, typename Edit_fn>
class Dictionary {
public:
	Dictionary(const string_t& dict_file): dict(dict_file), e_distance(MAX_STRING) {

	}
	std::vector<string_t> get_close_words(const string_t& word, int target_distance) {
		std::vector<string_t> result;
		//Shortcut to correctlly spelling words O(log(n))
		if (dict.exist(word)) {
			result.push_back(word);
		} else {
			auto ptr = dict.get_range(word);
			while(ptr.first!=ptr.second) {
				if (e_distance(word, *(ptr.first)) == target_distance)
					result.push_back(*(ptr.first));
				++ptr.first;
			}
		}
		return result;
	}
private:
	Dict_impl dict;
	Edit_fn e_distance;
};


class Lexer {
public:
	Lexer(const string_t& fname) {
		ifile.exceptions(std::ifstream::badbit);
		ifile.open(fname);
	}
	bool next_word(string_t& word, string_t& spacing) {
		word.clear();
		spacing.clear();
		char_t c;
		while (ifile.get(c)) {
			if (std::iscntrl(c) || std::isblank(c) || std::ispunct(c)) {
				if (word.empty())
					spacing +=c;
				else {
					ifile.putback(c);
					return true;
				}
			} else
				word +=c;
		}
		return (!word.empty() || !spacing.empty())?true:false;
	}
private:
	std::ifstream ifile;
};

};


int
main(int ac, char* av[])
{
	if (ac!=3) {
		std::cerr<<"Usage: "<<av[0]<<" <Dictionary_file> <Text_for_spelling_file>"<<std::endl;
		return 0;
	}
	try {
		spell::Lexer lex(av[2]);
		spell::Dictionary<spell::Dict_trigram,spell::Edit_distance> dict(av[1]);

		spell::string_t word,spacing;
		word.reserve(spell::MAX_STRING);
		spacing.reserve(1024);
		while(lex.next_word(word,spacing)) {
			std::cout<<spacing;
			if (word.empty())
				continue;
			std::vector<spell::string_t> candidates=dict.get_close_words(spell::Dummy_norm()(word),
										 spell::DEFAULT_DISTANCE);
			if (!candidates.empty()){
				spell::Dummy_rank()(candidates, word);
				std::cout<<spell::Dummy_norm().out(candidates[0],word);
			} else if (!word.empty())
				std::cout<<"["<<word<<"]";
		}
	} catch (std::exception& e) {
		std::cerr<<"Error: "<<e.what()<<std::endl;
		return 1;
	}
	return 0;
}


