#include <vector>
#include <cmath>
#include <parallel/numeric>
#include <fstream>
#include <iostream>

using vector = std::vector<long double>;

inline vector
operator + (const vector& lhs, const vector& rhs)
{
	vector result(lhs.size(), 0.0);
	for(int i = 0; i < lhs.size(); ++i)
		result[i] = lhs[i] + rhs[i];
	return result;
}

inline vector&
operator += (vector& lhs, const vector& rhs)
{
        for(int i = 0; i < lhs.size(); ++i)
                lhs[i] += rhs [i];
	return lhs;
}

inline vector
operator - (const vector& lhs, const vector& rhs)
{
	vector result(lhs.size(), 0.0);
	for(int i = 0; i < lhs.size(); ++i)
		result[i] = lhs[i] - rhs[i];
	return result;
}

inline vector
operator / (const vector& lhs, long double rhs)
{
	vector result(lhs.size(), 0.0);
	for(int i = 0; i < lhs.size(); ++i)
		result[i] = lhs[i] / rhs;
	return result;	
}

inline vector&
operator /= (vector& lhs, long double rhs)
{
	for(int i = 0; i < lhs.size(); ++i)
		lhs[i] = lhs[i] / rhs;
	return lhs;	
}

inline vector&
max_vector(vector& lhs, const vector& rhs)
{
	for(int i = 0; i < lhs.size(); ++i)
		lhs[i] = std::max(lhs[i], rhs[i]);
	return lhs;	
}

inline vector&
min_vector(vector& lhs, const vector& rhs)
{
	for(int i = 0; i < lhs.size(); ++i)
		lhs[i] = std::min(lhs[i], rhs[i]);
	return lhs;	
}

inline vector
pow_vector(const vector& lhs, long double pw)
{
	vector result(lhs.size(), 0.0);
	for(int i = 0; i < lhs.size(); ++i)
		result[i] = std::pow(lhs[i],pw);
	return result;
}

inline vector
sqrt_vector(const vector& lhs)
{
	vector result(lhs.size(), 0.0);
	for(int i = 0; i < lhs.size(); ++i)
		result[i] = std::sqrt(lhs[i]);
	return result;
}

long double
dot_product(const vector& lhs, const vector& rhs)
{
	// std::transform_reduce will be available with C++17
	// return std::transform_reduce(theta.begin(),theta.end(),x.begin(),0.0);
	//long double result = 0.0;
	//for (int i = 0; i < lhs.size(); ++i)
	//	result += lhs[i]*rhs[i];
	return std::inner_product(lhs.begin(),lhs.end(),rhs.begin(),0.0);
	//return result;
}

class linear_regression {
public:
	linear_regression(int n, long double a): N(n),alpha(a),theta(n,0.0) {
		init_theta();
	}
	void init_theta(void) {
	}
	long double hypothesis(const vector& x) {
		return dot_product(x,theta);
	}
	
	std::pair<vector,vector> mean_range(const std::vector<vector>& Xs) {
		vector mean(N,0.0);
		vector maxv = Xs[0];
		vector minv = Xs[0];
		for (int i = 0; i < Xs.size() ; ++i) {
			mean +=Xs[i];
			max_vector(maxv, Xs[i]);
			min_vector(minv, Xs[i]);
		}
		mean /= static_cast<long double>(Xs.size());
		return std::make_pair(mean , maxv - minv);
	}
	
	vector standard_deviation(const std::vector<vector>& Xs, const vector& mean) {
		vector sd(N,0.0);
		for(int i = 0; i < Xs.size(); ++i) {
			sd += pow_vector(Xs[i]-mean,2);
		}
		return sqrt_vector(sd/static_cast<long double>(Xs.size()));
	}
	
	long double mse(const std::vector<vector>& Xs, const std::vector<long double>& Ys) {
		long double mse_=0.0;
		for(int i=0; i < Xs.size(); ++i) {
			mse_ += std::pow(hypothesis(Xs[i])-Ys[i],2);
		}
		return mse_/static_cast<long double>(2*Xs.size());
	}
	long double derivative(const std::vector<vector>&Xs, const std::vector<long double>& Ys, int j) {
		long double result = 0.0;
		for(int i=0; i < Xs.size(); ++i) {
			result +=(hypothesis(Xs[i])-Ys[i])*Xs[i][j];
		}
		return result/static_cast<long double>(Xs.size());
	}
	// returns a cost and make an iteration (compute Theta)
	long double step(const std::vector<vector>& Xs, const std::vector<long double>& Ys) {
		vector new_theta(N,0.0);
		for(int j = 0; j<N; ++j) {
			new_theta[j] = theta[j]-alpha*derivative(Xs,Ys,j);
		}
		new_theta.swap(theta);
		return mse(Xs,Ys);
	}
private:
	int N; // dimention of Theta and X should be N X0 always 1;
	vector theta;
	long double alpha;
};


int
main(int ac, char* av[])
{
	if (ac!=6) {
		std::cout<<"usage: "<<av[0]<<": <dim> <alpha> <datafile> <iterations> <test>"<<std::endl;
		return 0;
	}
	int N = std::atoi(av[1]);
	long double alpha = std::atof(av[2]);
	std::ifstream in(av[3]);
	long int iterations = std::atol(av[4]);

	vector test(N,1);
	test[1] = std::atof(av[5]);
	std::vector<vector> X;
	std::vector<long double> Y;
	for(;;) {
		int n = N-1;
		long double val;
		vector x;
		x.emplace_back(1.0);
		bool the_end = false;
		while(n--) {
			if (!(in>>val)) {
				the_end = true;
				break;
			}
			x.emplace_back(val);
		}
		if (!the_end) {
			X.emplace_back(x);
			in>>val;
			Y.emplace_back(val);
		} else
			break;
	}
	linear_regression lg(N,alpha);
	long double last_cost = 0.0;
	int n_eq = 100;
	for(long int i = 1; i < iterations; ++i) {
		long double cost = lg.step(X,Y);
		if ((i % 1000000) == 0)
			std::cout<<std::sqrt(cost)<<std::endl;
		if (cost == last_cost)
			--n_eq;
		else
			n_eq = 100;
		last_cost = cost;
		if (n_eq == 0)
			break;
		
	}
	std::cout<<"hypothesis("<<test[1]<<")="<<lg.hypothesis(test)<<std::endl;
	return 0;
}

// Local Variables:
// compile-command: "c++ -g -O3 -std=c++11 lregress.cc"
// End:
