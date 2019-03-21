#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <cstdlib>
#include <list>


namespace knn {

	using vector = std::vector<double>;
	using pq_vector = std::vector<unsigned char>;
	
	vector
	operator*(const vector& v, double scalar) {
		vector result = v;
		for(auto & vi: result)
			vi *= scalar;
		return result;
	}
	
	vector
	operator/(const vector& v, double scalar) {
		vector result = v;
		for(auto & vi: result)
			vi /= scalar;
		return result;
	}
	
	vector
	operator+(const vector& v1, const vector& v2) {
		vector result(v1.size(), 0.0);
		for(int i=0; i < v1.size(); ++i)
			result[i] = v1[i] + v2[i];
		return result;
	}
	
	vector
	operator-(const vector& v1, const vector& v2) {
		vector result(v1.size(), 0.0);
		for(int i=0; i < v1.size(); ++i)
			result[i] = v1[i] - v2[i];
		return result;
	}
	
	
	
	
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
		kmean(int vsz, const std::vector<vector>& v, int k):
			points(v), ks(k, vector(vsz,0.0)), counts( ks_ids.size(), 0), clusters(v.size(), -1){
			sample<int> res(k);
			for(int i = 0; i < v.size(); ++i)
				res.put(i);
			ks = res.reservoir();
			int i = 0;
			for(auto id: ks_ids) {
				ks[i++] = points[id];
			}
		}
		kmean(int vsz, const std::vector<vector>& v, std::vector<int>& ks_ids):
			points(v), ks(ks_ids.size(), vector(vsz,0.0)), counts( ks_ids.size(), 0), clusters(v.size(), -1) {
			int i = 0;
			for(auto id: ks_ids) {
				ks[i++] = points[id];
			}
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
		
		std::pair<int,double>  find_closest(const vector& p) const {
			int min_index=0;
			double min_distance=distance(p,ks[min_index]);
			
			for(int i = 0; i < ks.size(); ++i) {
				double dist = distance(p,ks[i]);
				if (dist < min_distance) {
					min_distance = dist;
					min_index = i;
				}
			}
			return std::make_pair(min_index,min_distance);
		}
	public:
		void print_vect(const vector& v) const {
			std::cout<<"[";
			for(auto x: v) {
				std::cout<<x<<" ";
			}
			std::cout<<"]";
		}
		double show_cost(void) const {
			return cost;
		}
		bool run(void) {
			cost = 0.0;
			bool change = false;
			for(int i = 0; i < points.size(); ++i) {
				auto p = find_closest(points[i]);
				cost += p.second;
				int prev_class = clusters[i];
				if (prev_class != -1) {
					if (counts[prev_class] > 1) {
						ks[clusters[i]] = (ks[clusters[i]]*counts[clusters[i]]-points[i]) /
							(static_cast<double>(counts[clusters[i]]-1));
					}
					--counts[clusters[i]];
				}
				++counts[p.first];
				ks[p.first] = (ks[p.first]*(counts[p.first]-1)+points[i]) /
					(static_cast<double>(counts[p.first]));
				clusters[i]=p.first;
				
				
				change = change || (prev_class != p.first); 
			}
			cost /= static_cast<double> (points.size());
			return change;
		}
		const std::vector<int>& classes(void) {
			return clusters;
		}
		const std::vector<vector>& k_means(void) {
			return ks;
		}
	private:
		const std::vector<vector>& points;
		std::vector<vector> ks;
		std::vector<int> counts;
		std::vector<int> clusters;
		double cost;
	};

	class pq {
	public:
		typedef std::vector<vector> data_t;
		typedef std::vector<pq_vector> result_t;
		pq(int n_s): k_(256), split_dim(n_s) {
		}
		void compress(const std::vector<vector>& v, int dim) {
			//Split original matrix into dim/split_dim matrixes
			//with shorter vectors (part of original vectors)
			split(v, dim);
			
			int result_dim = dim/split_dim_;
			compressed_data_.resize(v.size(), pq_vector(result_dim));
			codebook_.resize(result_dim);
			int i = 0;
			// For every shorter matrix. (array of shorter vectors)
			for(auto & vect: splitted_data_) {
				// Build k_ (256) centroids.
				kmean km(split_dim_, vect, k_);
				for(int k = 100; k && km.run(); --i);
				auto & classes = km.classes();
				for(int j = 0; j < classes.size(); ++j) {
					//Assign new centroid number instead of original vector range
					//TODO: instead of vector of vectors better to make own
					//Tensor class based on 1-d vector. With flat stucture sse instructures
					//could be utilized.
					compressed_data_[j][i] = classes[j];
					//save centroids: save k means
					codebook_[i] = km.k_means();
					++i;
				}
			}
		}
		distances(const vector& vect);
		rank(int nth);
	private:
		codebook_distances(const vector& one) {
			dim = one.size();
			clear(codebook_result_);
			for(int row = 0; row < k_; ++row) {
				//along col or row?
				for(int col = 0; col*split_dim_ < dim; ++col) {
					for(int part_dim = 0; part_dim < split_dim_; ++part_dim)
						codebook_result_[row][col] +=codebook_[row][col][part_dim]*one[col*split_dim_ + part_dim]; 
				}
			}
		}
		void split(const std::vector<vector>& v, int dim) {
			if ((dim % split_dim_) != 0)
				throw std::runtime_exception("Dimention can not be divided by exact splits.");
			for(int i = 0; i*split_dim_ < dim; ++i) {
				splitted_data_.push_back(data_t());
			}
			for(auto & row: v) {
				for(int i = 0; i*split_dim_ < row.size(); ++i) {
					splitted_data_[i].push_back(vector(split_dim_));
					std::copy(row.begin()*(i*split_dim_), row.begin()*((i+1)*split_dim_),
						  splitted_data_[i].back().begin());
				}
			}
		}
		int k_;
		int split_dim_;
		std::vector<data_t> splitted_data_;
		result_t compressed_data_;
		std::vector<std::vector<vector>> codebook_;
		std::vector<std::vector<double>> codebook_distances_;
	};
	

};
	
int
main(int ac, char *av[])
{
	if (ac!=3) {
		std::cout<<"kmean <k> <points_file>"<<std::endl;
		std::cout<<"The first number in the points file is the vectors dimentions."<<std::endl;
		exit(1);
	}
	int k = std::atoi(av[1]);
	
	std::ifstream data(av[2]);
	if (!data) {
		std::cout<<"Unable to open data file.\n";
		exit(2);
	}

	int v_size;

	data>>v_size;

	knn::sample<int> sampler(k);
	std::vector<knn::vector> points;
	points.reserve(v_size);
	
	bool stop = false;
	int j = v_size;
	knn::vector v;
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
			sampler.put(std::distance(points.begin(),points.end())-1);
			v.clear();
			j = v_size;
		}
	} while (true);

	auto ks = sampler.reservoir();
	knn::kmean km(v_size, points, ks);
	int i = 10000;
	bool have_change = true;
	while(have_change && i-- ) {
		have_change = km.run();
		std::cout<<"Iteration: "<<i+1<<" Cost = "<<km.show_cost()<<std::endl;
	}
	auto clusters = km.classes();
	for(int i = 0; i < points.size(); ++i) {
		std::cout<<"[";
		for(auto& v: points[i])
			std::cout<<v<<" ";
		std::cout<<"] at "<<clusters[i]<<" mean [";
		for(auto&v : points[clusters[i]])
			std::cout<<v<<" ";
		std::cout<<"]"<<std::endl;
	}
}

// Local Variables:
// compile-command: "c++ -g -std=c++11 kmean.cc"
// End:
