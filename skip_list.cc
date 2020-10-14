#include <optional>
#include <functional>
#include <iostream>
#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <string>
#include <map>

template<typename KEY, typename VALUE>
class skip_list {
public:
    static const int max_lvl = 64;
    using keyval_t = std::pair<KEY,VALUE>;
    struct node {
      node(): keyval(), size(max_lvl)
      {
          init();
      }
      node(KEY k, VALUE v, int lvl): keyval(std::in_place, std::move(k), std::move(v)), size(lvl)
      {
          init();
      }
      void init(void) {
          for(int i=0;i<size;++i) next[i] = nullptr;
      }
      std::optional<keyval_t> keyval;
      int size;
      node* next[1];
    };
private:
    void* alloc(int lvl) {
      int sz = sizeof(node) + sizeof(node*)*(lvl-1);
      return  ::operator new(sz);
    }
    node* create_node(KEY k, VALUE v, int lvl) {
      void* mem = alloc(lvl);
      return  new (mem) node(std::move(k), std::move(v), lvl);
    }
    node* create_node() {
      void* mem = alloc(max_lvl);
      return  new (mem) node;
    }
public:
  skip_list(): mt(rnd()), dist(0, std::numeric_limits<int>::max())  {
    head = create_node();   
  }
private:
  int rand_lvl() {
        int dice = dist(mt);
        for(int i = 1, t=2; i<max_lvl; ++i, t+=t) {
            if (dice > std::numeric_limits<int>::max()/t) {
                m_lvl = std::max(m_lvl, i);
                return i;
            }
        }
        return max_lvl;
  }
  node* search(node* ptr,const KEY& target, int lvl) const {
        if (ptr == nullptr) {
            return nullptr;
        }
        if (ptr->keyval.has_value() && ((*ptr->keyval).first == target))
            return ptr;
        node* nxt = ptr->next[lvl];
        if (nxt == nullptr || target<(*nxt->keyval).first)
            return lvl == 0? nullptr: search(ptr, target, lvl-1);
        else
            return search(nxt, target, lvl);
    }
public:
    std::optional<std::reference_wrapper<VALUE>> search(const KEY& target) {
        if (auto ptr = search(head, target, m_lvl)) {
            return {std::ref((*ptr->keyval).second)};
        }
        return std::nullopt;
    }

  std::optional<std::reference_wrapper<const VALUE>> search(const KEY& target) const {
      if (auto ptr = search(head, target, m_lvl)) {
            return {std::cref(*ptr->keyval)};
        }
        return std::nullopt;
    }
  std::optional<std::reference_wrapper<keyval_t>> search_kv(const KEY& target) {
      if (auto ptr = search(head, target, m_lvl)) {
            return {std::ref(*ptr->keyval)};
        }
        return std::nullopt;
  }

  std::optional<std::reference_wrapper<const keyval_t>> search_kv(const KEY& target) const {
      if (auto ptr = search(head, target, m_lvl)) {
            return {std::cref((*ptr->keyval).second)};
        }
        return std::nullopt;
    }
private:
    node* insert(node* ptr, node* cur, int lvl) {
        node* nxt = ptr->next[lvl];
        if (nxt == nullptr || (*cur->keyval).first < (*nxt->keyval).first) {
            if (lvl < cur->size) {
                ptr->next[lvl] = cur;
                cur->next[lvl] = nxt;
            }
            return lvl == 0? cur : insert(ptr, cur, lvl-1);
        } else
            return insert(nxt, cur, lvl);
    }
public:
    void add(KEY k, VALUE v) {
        node* n = create_node(std::move(k),std::move(v), rand_lvl());
        insert(head, n, m_lvl);
    }
private:
    bool erase(node* ptr,const KEY& k, int lvl) {
        if (ptr == nullptr)
            return false;
        node* nxt = ptr->next[lvl];
        if (nxt == nullptr || k < (*nxt->keyval).first) {
            if (lvl == 0)
                return false;
            return erase(ptr, k, lvl-1);
        }
    
        if ((*nxt->keyval).first == k) {
                ptr->next[lvl] = nxt->next[lvl];
                if (lvl == 0) {
                    nxt->~node();
                    ::operator delete(reinterpret_cast<void*>(nxt));
                    return true;
                }
                return erase(ptr, k, lvl-1);
        } else 
                return erase(nxt, k, lvl);
    }
public:
    bool erase(const KEY& num) {
        return erase(head, num, m_lvl);   
    }
  int get_max_lvl() {
    return m_lvl;
  }
private:
    int m_lvl = 0;
    node* head;
    std::random_device rnd;
    std::mt19937 mt;
    std::uniform_int_distribution<> dist;
};


struct Test {
    Test(): v(0) {
    }
    Test(int v_): v(v_) {
    }
    Test(const Test& v_): v(v_.v) {
    }
    Test(Test&& v_): v(v_.v) {
    }
    int v;
};

bool
operator<(const Test& l, const Test& r)
{
    return l.v < r.v;
}

bool
operator==(const Test& l, const Test& r)
{
    return l.v == r.v;
}


int
main(int ac, char* av[])
{
    int iters = 1000;

    if(ac>1)
        iters = std::stoi(av[1]);

    int test = 3;
    if (ac>2) {
      test = std::stoi(av[2]);
    }
    
    skip_list<int,int> sk;
    sk.add(1,1);
    sk.add(100,100);
    sk.add(3,3);
    auto r = sk.search(3);
    auto check = [](int k, int v, const auto& r) {
                     if (!r.has_value())
                         std::cout<<"no "<<k<<std::endl;
                     else {
                         if (*r!=v) {
                             std::cout<< "v!=rv " <<v<<"!="<<*r<<std::endl;
                         }
                     }
                 };
    check(3,3,r);
    r = sk.search(0);
    check(0,0,r);
    sk.add(1000,1000);
    sk.add(3000,3000);
    r = sk.search(3000);
    check(3000,3000,r);
    sk.erase(3000);
    r = sk.search(3000);
    check(3000,3000,r);

    std::random_device rnd;
    std::mt19937 mt(rnd());
    std::uniform_int_distribution<> dist(0,std::numeric_limits<int>::max());


    std::vector<int> data;
    data.reserve(iters);
    std::cout<<"Starting gen data.\n";
    for(int i=0;i<iters;++i) {
        data.push_back(dist(mt));
    }
    std::cout<<"Ending gen data.\n";

    std::cout<<"Starting inserting into skip list.\n";
    
    auto start = std::chrono::steady_clock::now();
    if (test & 1) {
      for(int v: data)
        sk.add(v,v);
    }
    auto end = std::chrono::steady_clock::now();
    std::cout<<"Ending inserting into skip list.\n";
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num /
        std::chrono::nanoseconds::period::den <<" sec ";
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num %
        std::chrono::nanoseconds::period::den <<" nSec "<<std::endl;



    std::cout<<"Max level: "<<sk.get_max_lvl()<<"\n";
    std::cout<<"Starting searching skip list.\n";
    start = std::chrono::steady_clock::now();

    int skcount = 0;
    if (test & 1) {
      for(int v: data) {
          auto r = sk.search(v);
          skcount += r.has_value()?1:0;
      }
    }
    end = std::chrono::steady_clock::now();
    std::cout<<"Ending searching skip list. "<<skcount<<" goods.\n";
    elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num /
        std::chrono::nanoseconds::period::den <<" sec ";
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num %
        std::chrono::nanoseconds::period::den <<" nSec "<<std::endl;

    std::map<int,int> rb;
    std::cout<<"Starting inserting into RB map.\n";
    start = std::chrono::steady_clock::now();
    if (test & 2) {
      for(int v: data)
        rb[v]=v;
    }
    end = std::chrono::steady_clock::now();
    std::cout<<"Ending inserting into RB map.\n";
    elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num /
        std::chrono::nanoseconds::period::den <<" sec ";
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num %
        std::chrono::nanoseconds::period::den <<" nSec "<<std::endl;

    std::cout<<"Starting searching RB map.\n";
    start = std::chrono::steady_clock::now();
    int rbcount = 0;
    if (test & 2) {
      for(int v: data)
          rbcount += rb.find(v) == rb.end()?0:1;
    }
    end = std::chrono::steady_clock::now();
    std::cout<<"Ending searching RB map. "<<rbcount<<" goods. \n";

    elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num /
        std::chrono::nanoseconds::period::den <<" sec ";
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num %
        std::chrono::nanoseconds::period::den <<" nSec "<<std::endl;

    skip_list<Test,Test> sk2;
    sk2.add(1,11);
    sk2.add(Test(2), Test(22));
    Test t1(3), t2(33);
    sk2.add(t1,t2);
    if (sk2.search(Test(2)).has_value() && sk2.search(Test(2)).value() == Test(22))
        std::cout<<"Ok\n";
    else
        std::cout<<"Fail\n";
}
