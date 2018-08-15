#include <string>
#include <random>
#include <cmath>


class rolling_hash {
public:
	rolling_hash() {
		q = find_prime(rand(1,1000000000));
		x = rand(0,q-1);
	}
	long char_hash(const std::string& s, long i, long sz, long start) {
		return (static_cast<long>(s[i+start])*static_cast<long>(pow(x,sz-1-i)+0.5)) % q;
	}
	long first_hash(const std::string& s, long m, long start=0) {
		long hval = 0;
		for(long i = 0; i < m; ++i) {
			hval += char_hash(s,i,m,start);
			hval %=q;
		}
		return hval;
	}

	long begin(const std::string& txt, long sz, long start_pos=0) {
		index = start_pos;
		pat_sz = sz;
		current_hash = first_hash(txt, sz, index);
		return current_hash;
	}
	long next(const std::string& txt) {
		++index;
		if (index < txt.size()) {
			current_hash -= char_hash(txt,index-1,pat_sz,start_pos);
			current_hash = (current_hash * x) % q;
			current_hash = (current_hash+static_cast<long>(txt[start_pos+index+pat_sz-1])) % q; 
		}
		return current_hash;
	}
	
	long  rand(long from, long to) {
		std::uniform_int_distribution<> dis(from,to);
                return dis(gen);
	}
	
	long find_prime(long n) {
		std::vector<long> num(n,0);
		for(long i=2; (i-2)<n; ++i)
			num[i-2]=i;
		
		for(long i=0;;) {
			//find first prime 
			while(i<n && num[i]==0)
				++i;
			if (i == n)
				break;
			long prime = num[i++];
			for(long j = i ; j < n; ++j)
				if ((num[j] % prime) == 0)
					num[j]=0;
		}
		long p = rand(1,n/2);
		long i = 0;
		while(p) {
			while(num[i]==0) {
				i++;
				if (i==n)
					i=0;
			}
			--p;
		}
		return num[i];
	}
	
	long index;
	std::random_device rd;
	std::mt19937 gen;
	long q;
	long x;
};


std::vector<long>
rabin_karp(const std::string& txt, const std::string& pat)
{
	rolling_hash rh;
	long target_hash = rh.first_hash(pat,pat.size());
	
	std::vector<long> res;
	for(long hash = rh.begin(txt,pat.size()), long i=0; i < txt.size()-pat.size();
	    ++i, hash = rh.next(txt)) {
		if (hash == target_hash) {
			int j = 0;
			while(j < pat.size() && txt[j+i] == pat[j])
				++j;
			if (j == pat.size())
				res.push_back(i);
		}
	}
	return res;
}


