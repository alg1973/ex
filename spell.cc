// -*- compile-command: "c++ -g -std=c++11 spell.cc" -*-

#include <iostream>
#include <array>
#include <vector>
#include <fstream>
#include <string>
#include <cctype>
#include <unordered_map>


namespace spell {

class edit_distance {
public:
	edit_distance(int max_str): max_str_(max_str), mem(max_str+1,std::vector<int>(max_str+1,0))
		{}
	int operator()(const std::string& p, const std::string& t) {
		if (p.size()> max_str_ || std::max(p.size(),t.size())-std::min(p.size(),t.size())>1)
			return -1;
		return compute(p, p.size(), t, t.size());
	}
	void rank(std::vector<int>&, const std::string&) {
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
	int max_str_;
	std::vector<std::vector<int>> mem;
};

class Dict_plain {
public:
	class Iter {
	public:
		Iter(std::vector<std::string>::iterator i): it(i) {
			
		}
		Iter& operator++() {
			++it;
			return *this;
		}
		Iter operator++(int) {
			Iter r = *this;
			++(*this);
			return r;
		}
		std::string& operator*() {
			return *it;
		}
		bool operator==(const Iter& l) const {
			return it == l.it;
		}
		bool operator!=(const Iter& l) const {
			return !(*this == l);
		}
	private:
		std::vector<std::string>::iterator it;
	};
	std::pair<Iter,Iter> get_range(const std::string& word) {
		return std::make_pair(dict.begin(), dict.end());
	}
	Dict_plain(const std::string& file) {
		std::ifstream i;
		i.exceptions(std::ifstream::badbit);
		i.open(file);
		std::copy(std::istream_iterator<std::string>(i), std::istream_iterator<std::string>(), std::back_inserter(dict));
	};
protected:
	const std::vector<std::string>& words(void) const {
		return dict;
	}
private:
	std::vector<std::string> dict;
};


struct trigram_hash {
	size_t operator()(const std::array<char,3>& tri) const {
		return std::hash<char>()(tri.at(0))^std::hash<char>()(tri.at(1))^std::hash<char>()(tri.at(2));
	}
};



class Dict_trigram: public Dict_plain {
public:
	Dict_trigram(const std::string& file) : Dict_plain(file) {
		auto ptr = Dict_plain::get_range("");
		for(int i=0; ptr.first!=ptr.second;  ++ptr.first, ++i) {
			auto tri_v = gen_trigrams(*ptr.first);
			for(auto& tri: tri_v) {
				tri_hash[tri].push_back(i);
			}
		}
		std::cout<<"Buckets: "<<tri_hash.bucket_count()<<" avg bucket size: "<<tri_hash.load_factor()<<std::endl;
	}
	class Iter {
	public:
		Iter(std::vector<std::array<char,3>>::iterator i,const  Dict_trigram* dict_, int off=0): it(i), offset(off), dict(dict_) {
			
		}
		Iter& operator++() {
			++offset;
			if (offset >= dict->tri_hash.at(*it).size()) {
				++it;
				offset=0;
			}
			return *this;
		}
		Iter operator++(int) {
			Iter r = *this;
			++(*this);
			return r;
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
	private:
		std::vector<std::array<char,3>>::iterator it;
		int offset;
		const Dict_trigram* dict;
	};

	std::pair<Iter,Iter> get_range(const std::string& word) const {
		auto tri_v = gen_trigrams(word);
		return std::make_pair(Iter(tri_v.begin(),this), Iter(tri_v.end(),this));
	}

	static std::vector<std::array<char,3>> gen_trigrams(const std::string& w) {
		std::vector<std::array<char,3>> result;
		result.reserve(w.size());
		std::array<char,3> trig = {{'$','$','$'}};
		if (w.size()==0)
			return result;
		// Put all 1 and 2 letters words in special bucket
		if (w.size() == 1 || w.size() == 2) 
			result.push_back(trig);
		if (w.size()==1) {
			trig[1] = w[0];
			result.push_back(trig);
		} else {
			for(int i = 0; i<w.size()-2; ++i) {
				for(int j = 0; j<3; ++j) {
					trig[j]=w[i+j];
				}
				result.push_back(trig);
			}
			trig[0]='$';
			trig[1]=w[0];
			trig[2]=w[1];
			result.push_back(trig);
			trig[0]=w[w.size()-2];
			trig[1]=w[w.size()-1];
			trig[2]='$';
			result.push_back(trig);

		}
		return result;
	}
private:
	std::unordered_map<std::array<char,3>,std::vector<int>,trigram_hash> tri_hash;
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
		char c;
		word = "";
		spacing = "";
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
		return (!word.empty() || !spacing.empty())? true: false;
	}
private:
	std::ifstream ifile;
	std::string word_;
	std::string spacing_;
};

};


int
main(int ac, char* av[])
{
	spell::Lexer lex(av[1]);

	spell::Dictionary<spell::Dict_trigram,spell::edit_distance> dict("./words.txt");

	std::string word,spacing;

	while(lex.next_word(word,spacing)) {
		auto v = spell::Dict_trigram::gen_trigrams(word);
		for(auto t: v) {
			std::cout<<"'"<<t[0]<<t[1]<<t[2]<<"'\n";
		}
		std::clog<<spacing;
		if (word.empty())
			continue;
		std::vector<std::string> candidates(dict.get_close_words(word));
		if (!candidates.empty()) {
			std::clog<<candidates[0];
		} else if (!word.empty())
			std::clog<<"["<<word<<"]";
	}
}
