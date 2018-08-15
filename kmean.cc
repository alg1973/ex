#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <cstdlib>
#include <list>


using vector = std::vector<double>;



template<typename T,typename Container = std::vector<T>>
class sample {
public:
        sample(int n): N(n),k(0),rd(),gen(rd())
                {}
        int get_N(void) {
                return N;
        }
        int get_k(void) {
                return k;
        }
        Container& reservoir(void) {
                return cont;
        }
        void put(const T& v) {
                if (N>k++) {
                        cont.emplace_back(v);
                } else {
                        int l = rnd();
                        if (l<N) {
                                cont[l]=v;
                        }
                }
        }
private:
	int rnd(void) {
                std::uniform_int_distribution<> dis(0, k++);
                return dis(gen);
        }
        int N;
        int k;
        Container cont;
        std::random_device rd;
        std::mt19937 gen;
};

class kmean {
public:
	kmean(const std::vector<vector>& v, const std::vector<vector>& ks_):
		points(v),ks(ks_) {
	}
private:
	static double distance(const  vector &v1, const vector& v2) {
		
		double norm = 0.0;
		for(int i = 0;i<v1.size();++i) {
			double d = v1[i]-v2[i];
			norm += d*d;
		}
		return norm;
	}

	static vector& add_assign(vector& v1, const vector& v2)	{
		for (int i = 0 ; i < v1.size(); ++i)
			v1[i] += v2[i];
		return v1;
	}
	
	std::pair<int,double>  find_closest(const vector& p) {
		int min_index=0;
		double min_distance=distance(p,ks[min_index]);
		
		for(int i = 1; i < ks.size(); ++i) {
			double dist = distance(p,ks[i]);
			if (dist < min_distance) {
				min_distance = dist;
				min_index = i;
			}
		}
		return std::make_pair(min_index,min_distance);
	}
public:
	double show_cost(void) {
		return cost;
	}
	std::vector<int> run(void) {
		std::vector<int> clusters;
		cost = 0.0;
		for(int i = 0; i < points.size(); ++i) {
			auto p = find_closest(points[i]);
			cost += p.second;
			clusters.emplace_back(p.first);
		}
		cost /= static_cast<double> (points.size());
		return clusters;
	}

	void recalculate_ks(const std::vector<int>& clusters )  {
		for(auto& v:ks)
			std::fill(v.begin(), v.end(), 0.0);
		std::vector<int> counters(ks.size(),0);
		for (int i = 0; i <points.size() ; ++i) {
			add_assign(ks[clusters[i]],points[i]);
			counters[clusters[i]]++;
		}
		for(int i = 0; i < ks.size(); ++i) {
			for(auto &v: ks[i])
				v /= static_cast<double>(counters[i]);
		}
	}
private:
	std::vector<vector> ks;
	const std::vector<vector>& points;
	double cost;
};

int
main(int ac, char *av[])
{
	if (ac!=4) {
		std::cout<<"kmean <k> <vector_size> <points_file>"<<std::endl;
		exit(1);
	}
	int k = std::atoi(av[1]);
	int v_size = std::atoi(av[2]);

	std::ifstream data(av[3]);
	if (!data) {
		std::cout<<"Unable to open data file.\n";
		exit(2);
	}

	sample<vector> sampler(k);
	std::vector<vector> points;
	
	int stop = false;
	int j = v_size;
	vector v;
	double x;
	do  {
		if (j>0) {
			if (data>>x) {
				v.emplace_back(x);
				--j;
			} else {
				break;
			}
		} else {
			points.emplace_back(v);
			sampler.put(v);
			v.clear();
			j = v_size;
		}
	} while (true);

	auto ks = sampler.reservoir();
	kmean km(points,ks);
	int i = 10;
	std::vector<int> clusters;
	while(i--) {
		clusters = km.run();
		km.recalculate_ks(clusters);
		std::cout<<"Cost = "<<km.show_cost()<<std::endl;
	}
	for(int i = 0; i < points.size(); ++i) {
		std::cout<<"[";
		for(auto& v: points[i])
			std::cout<<v<<" ";
		std::cout<<"] at [";
		for(auto&v : points[clusters[i]])
			std::cout<<v<<" ";
		std::cout<<"]"<<std::endl;
	}
}
