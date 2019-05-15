#include <vector>
#include <array>
#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <queue>

//
// It's an Aho-Corasick string searching algorithm implimentation.
//


class fgrep {
public:
	fgrep(): save_state(0) {
		std::array<int,256> state;
		state.fill(-1);
		goto_states.emplace_back(state);
	}
	void add(const std::string& w) {
		if (w.empty())
			return;
		int current_state = 0;
		for(char c: w) {
			if (goto_states[current_state][0xff&c] == -1) {
				std::array<int,256> state;
				state.fill(-1);
				goto_states.emplace_back(state);
				goto_states[current_state][0xff&c] = goto_states.size()-1;
			}
			current_state = goto_states[current_state][0xff&c];
		}
		if (current_state+1 > output.size()) {
			output.resize(current_state+1);
		}
		patterns.push_back(w);
		output[current_state].push_back(patterns.size()-1);
	}

	void build_failure(void) {
		if (output.empty())
			return;
		failure.resize(goto_states.size());
		std::fill(failure.begin(),failure.end(),-1);
		failure[0] = 0;
		std::queue<int> bfsq;
		fsm.resize(goto_states.size(), std::array<int,256>({{}}));
		for(int i=0; i<256; ++i) {
			if (int next_state = goto_states[0][i] ; next_state !=-1) {
				failure[next_state] = 0;
				bfsq.push(next_state);
			} else
				goto_states[0][i] = 0;
			
			fsm[0][i]=goto_states[0][i];
		}

		while(!bfsq.empty()) {
			int r = bfsq.front();
			bfsq.pop();
			for(int i = 0; i<256; ++i) {
				if (int next_state = goto_states[r][i]; next_state!=-1) {
					int state = failure[r];
					while(goto_states[state][i]==-1)
						state = failure[state];
					failure[next_state] = state;
					bfsq.push(next_state);
					// If prefix ("state") have output then suffix ("next_state") should have it.
					// so adding here output of some prefix to suffix.
					merge_output(state, next_state);
					fsm[r][i]=goto_states[r][i];
				} else {
					fsm[r][i]=goto_states[failure[r]][i];
				}
			}
		}
	}


	std::pair<int,std::list<int>> search_with_fsm(const std::string& text) {
		if (!output.empty()) {
			int j = 0;
			int state = 0;
			while(j<text.size()) {
				state = fsm[state][0xff & text[j++]];
				if(count_output(state)>0) {
					save_state = state;
					return std::make_pair(j-1, output_strings(state));
				}
			}
		}
		return std::make_pair(-1,std::list<int>());
	}

	std::pair<int,std::list<int>> search_with_failure(const std::string& text) {
		if (!output.empty()) {
			int j = 0;
			int state = 0;
			while(j<text.size()) {
				while(goto_f(state, text[j]) == -1)
					state = failure[state];
				state = goto_f(state, text[j++]);
				if(count_output(state)>0) {
					save_state = state;
					return std::make_pair(j-1, output_strings(state));
				}
			}
		}
		return std::make_pair(-1,std::list<int>());
	}
	
	int count_output(int state) const {
		return output[state].size();
	}
	
	std::list<int> output_strings(int state) const {
		return output[state];
	}

	std::string get_string(int n) const {
		return patterns[n];
	}
	bool empty() {
		return output.empty();
	}
private:
	void merge_output(int from, int to) {
		std::list<int> temp(output[from]);
		output[to].splice(output[to].begin(),temp);
	}
	
	int goto_f(int state, char c) const {
		return goto_states[state][0xff & c];
	}
	
	std::vector<std::array<int,256>> goto_states;
	std::vector<std::array<int,256>> fsm;
	std::vector<std::list<int>> output;
	std::vector<std::string> patterns;
	std::vector<int> failure;
	int save_state;
};

int
main(int ac, char *av[])
{
	if (ac!=3) {
		std::cerr<<"Usage: "<<av[0]<<": <patterns_file> <text_file>"<<std::endl;
		return 1;
	}
	fgrep f;
	std::string w;
	std::ifstream in(av[1]);
	
	while(in>>w)
		f.add(w);
	f.build_failure();

	if (f.empty()) {
		std::cerr<<"Empty pattern!"<<std::endl;
		return 0;
	}
	
	std::ifstream itext(av[2]);
	std::string text;
	std::copy(std::istream_iterator<char>(itext), std::istream_iterator<char>(),
		  std::back_inserter(text));

	auto r = f.search_with_fsm(text);

	std::cout<<"Matched position with fsm "<<r.first<<std::endl;

	for(int i: r.second) {
		std::cout<<f.get_string(i)<<std::endl;
	}

	r = f.search_with_failure(text);

	std::cout<<"Matched position with failure "<<r.first<<std::endl;
	
	for(int i: r.second) {
		std::cout<<f.get_string(i)<<std::endl;
	}
	return 0;
}

