// -*- compile-command: "c++ -g -O0 -std=c++11 spell.cc" -*-

#include <iostream>
#include <iterator>
#include <array>
#include <vector>
#include <fstream>
#include <string>
#include <cctype>
#include <unordered_map>
#include <memory>

namespace spell
{

using trigram_t=std::array<char,3>;

class Edit_distance {
public:
	Edit_distance(unsigned int max_s): max_s_(max_s), mem(max_s+1,std::vector<int>(max_s+1,0))
		{}
	int operator()(const std::string& p, const std::string& t) {
		if (p.size()> max_s_ || std::max(p.size(),t.size())-std::min(p.size(),t.size())>1)
			return -1;
		return compute(p, p.size(), t, t.size());
	}
	void rank(std::vector<std::string>&, const std::string&) {
	}
private:
	int compute(const std::string& ps, int plen, const std::string& ts, int tlen) {

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
	class Iter {
	public:
		Iter(std::vector<std::string>::const_iterator i): it(i) {

		}
		Iter& operator++() {
			++it;
			return *this;
		}
		const std::string& operator*() const{
			return *it;
		}
		bool operator==(const Iter& l) const {
			return it == l.it;
		}
		bool operator!=(const Iter& l) const {
			return !(*this == l);
		}
	private:
		std::vector<std::string>::const_iterator it;
	};
	std::pair<Iter,Iter> get_range(const std::string& word) const {
		return std::make_pair(Iter(dict.begin()), Iter(dict.end()));
	}
	Dict_plain(const std::string& file) {
		std::ifstream i;
		i.exceptions(std::ifstream::badbit);
		i.open(file);
		std::copy(std::istream_iterator<std::string>(i), std::istream_iterator<std::string>(),
			  std::back_inserter(dict));
	};
protected:
	const std::vector<std::string>& words(void) const {
		return dict;
	}
private:
	std::vector<std::string> dict;
};


struct trigram_hash {
	size_t operator()(const trigram_t& tri) const {
		return (std::hash<char>()(tri[0])*31+std::hash<char>()(tri[1]))*31+std::hash<char>()(tri[2]);
	}
};



class Dict_trigram: public Dict_plain {
public:
	Dict_trigram(const std::string& file) : Dict_plain(file) {
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
		const std::string& operator*() const {
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

	std::pair<Iter,Iter> get_range(const std::string& word) const {
		auto tri_v = gen_trigrams(word);
		return std::make_pair(Iter(tri_v->begin(), this, tri_v),
				      Iter(tri_v->end(), this, tri_v));
	}

	static std::shared_ptr<std::vector<trigram_t>> gen_trigrams(const std::string& w) {
		auto result = std::make_shared<std::vector<trigram_t>>();
		result->reserve(w.size());
		trigram_t trig = {{'$','$','$'}};
		if (w.size()==0)
			return result;
		// Put all 1`s and 2`s letters words in the special bucket
		if (w.size() == 1 || w.size() == 2)
			result->push_back(trig);
		if (w.size()==1) {
			trig[1] = w[0];
			result->push_back(trig);
		} else {
			for(size_t i = 0; i<w.size()-2; ++i) {
				for(int j = 0; j<3; ++j) {
					trig[j]=w[i+j];
				}
				result->push_back(trig);
			}
			trig[0]='$';
			trig[1]=w[0];
			trig[2]=w[1];
			result->push_back(trig);
			trig[0]=w[w.size()-2];
			trig[1]=w[w.size()-1];
			trig[2]='$';
			result->push_back(trig);
		}
		return result;
	}
private:
	std::unordered_map<trigram_t,std::vector<int>,trigram_hash> tri_hash;
};

const int MAX_STRING=50;

template<typename Dict_impl, typename Edit_fn>
class Dictionary {
public:
	Dictionary(const std::string& dict_file): dict(dict_file), e_distance(MAX_STRING) {

	}
	std::vector<std::string> get_close_words(const std::string& word) {
		std::vector<std::string> result;
		auto ptr = dict.get_range(word);
		while(ptr.first!=ptr.second) {
			int distance = e_distance(word, *(ptr.first));
			if (distance == 0) {
				result.clear();
				result.push_back(*(ptr.first));
				break;
			} else if (distance == 1)
				result.push_back(*(ptr.first));
			++ptr.first;
		}
		return result;
	}
private:
	Dict_impl dict;
	Edit_fn e_distance;
};


class Lexer {
public:
	Lexer(const std::string& fname) {
		ifile.exceptions(std::ifstream::badbit);
		ifile.open(fname);
	}
	bool next_word(std::string& word, std::string& spacing) {
		word.clear();
		spacing.clear();
		char c;
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
	try {
		spell::Lexer lex(ac>1?av[1]:"/dev/fd/0");
		spell::Dictionary<spell::Dict_trigram,spell::Edit_distance> dict("./words.txt");

		std::string word,spacing;
		word.reserve(spell::MAX_STRING);
		spacing.reserve(1024);
		while(lex.next_word(word,spacing)) {
			std::clog<<spacing;
			if (word.empty())
				continue;
			std::vector<std::string> candidates(dict.get_close_words(word));
			if (!candidates.empty()) {
				std::clog<<candidates[0];
			} else if (!word.empty())
				std::clog<<"["<<word<<"]";
		}
	} catch (std::exception& e) {
		std::cerr<<"Error: "<<e.what()<<std::endl;
		return 1;
	}
	return 0;
}
