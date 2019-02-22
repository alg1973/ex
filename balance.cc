#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cstdlib>

namespace tree {


  template<typename KT,typename DT>
  class avl_tree {
    using depth_t=char;
    
    struct node {
      explicit node(const KT& v,const DT& d): key(v),data(d),height(1),left(nullptr),right(nullptr)
      {}

      KT key;
      DT data;
      depth_t height;
      node* left;
      node* right;
    };
  public:
    avl_tree():root(nullptr)
    {}
    avl_tree(const avl_tree&  ) = delete;
    avl_tree& operator=(const avl_tree&) = delete;

    avl_tree(avl_tree&& old) {
      root=old.root;
      old.root=nullptr;
    }

    std::string print(void) const {
      std::ostringstream o;
      int_print(root,o);
      return o.str();
    }
    int height(void) const {
      return static_cast<int>(get_height(root));
    }
    
    void check_h(void) {
      check_height(root);
    }
    void insert(const KT& key,const DT& data);
    
    std::pair<bool,int> find(const KT& key) const; 
    void erase(const KT& k) {
      root=delete_node(root,k);
    }
  private:
    depth_t comp_height(node* r) const {
      if (!r) return 0;
      return std::max(comp_height(r->left),comp_height(r->right))+1;
    }
    depth_t check_height(node* n); 
    node* delete_node(node* n,const KT& k); 
    node*  min(node* n) {
      if (n && n->left) return min(n->left);
      else return n; 
    }
    node* chop_min(node*& n);
    void upd_height(node* r) {
      if (r) r->height = std::max(get_height(r->left),get_height(r->right))+1; 
    }
    depth_t get_height(node* r) const;
    depth_t get_diff(node* r) const {
      if (r) return get_height(r->left)-get_height(r->right);
      return 0;
    }
    node* fix_height(node* r);
    const node* int_find(node* r,const KT& k) const;
    void int_print(node* root,std::ostringstream& o) const;
    void add_node(node* n,const KT& key,const DT& data);
    node* rotate_right(node*);
    node* rotate_left(node*);
    node* root;
  };
}

namespace tree {

  template<typename KT,typename DT> 
  typename avl_tree<KT,DT>::depth_t 
  avl_tree<KT,DT>::check_height(node* n) 
  {
    if (!n)
      return 0;
    depth_t l=check_height(n->left);
    depth_t r=check_height(n->right);
    
    if ((std::max(l,r)-std::min(r,l))>1) {
      std::cout<<"Invalid balance: "<<n->height<<" k: "<<n->key<<" l: "<<l<<" r: "<<r<<std::endl;
    }
    depth_t m=std::max(l,r)+1;
    if (m!=n->height) {
      std::cout<<"Invalid height: "<<n->height<<" k: "<<n->key<<" l: "<<l<<" r: "<<r<<std::endl;
  }
    return m;
  }

  template<typename KT,typename DT> 
  void 
  avl_tree<KT,DT>::insert(const KT& key,const DT& data) 
  {
    if(!root) {
      root = new node(key,data);
    } else {
      add_node(root,key,data);
      upd_height(root);
      root=fix_height(root);
    }
  }

  template<typename KT,typename DT>   
  std::pair<bool,int> 
  avl_tree<KT,DT>::find(const KT& key) const 
  {
    auto r=int_find(root,key);
    if (r)
      return std::make_pair(true,r->data);
    return std::make_pair(false,0);
  }
  
  template<typename KT,typename DT> 
  typename avl_tree<KT,DT>::node* 
  avl_tree<KT,DT>::chop_min(node*& n) 
  {
    if (!n)  return n;
    if (!n->left) {
      node* r = n;
      n=n->right;
      return r;
    } else { 
      node* r=chop_min(n->left);
      upd_height(n->left);
      return r;
    }
  }
 
  template<typename KT,typename DT> 
  typename avl_tree<KT,DT>::depth_t 
  avl_tree<KT,DT>::get_height(avl_tree<KT,DT>::node* r) const
  {
    if (!r)
      return 0;
    return r->height;
  }

  template<typename KT,typename DT> 
  typename avl_tree<KT,DT>::node*
  avl_tree<KT,DT>::fix_height(node* r) 
  {
    depth_t diff = get_diff(r);
    if (diff==0 || abs(diff)==1) 
      return r;
    
    if (diff>0) {
      if (get_diff(r->left)>0)
	r=rotate_right(r);
      else {
	r->left=rotate_left(r->left);
	r=rotate_right(r);
      }
    } else {
      if (get_diff(r->right)<0)
	r=rotate_left(r);
      else {
	r->right=rotate_right(r->right);
	r=rotate_left(r);
      }
    }
    return r;
  }

  template<typename KT,typename DT> 
  void 
  avl_tree<KT,DT>::int_print(node* r,std::ostringstream& o) const {
    if (!r) {
      o<<"[nil]";
      return;
    }
    //o<<"(";
    o<<"[k: "<<r->key<<",d: "<<r->data<<",h: "<<r->height<<"]";
    int_print(r->left,o);
    int_print(r->right,o);
    //o<<")";
  }
  
  template<typename KT,typename DT> 
  void
  avl_tree<KT,DT>::add_node(avl_tree<KT,DT>::node* r,const KT& key,const DT& data) {
    if (!r) {
      return;
    }
    if (key==r->key) {
      r->data=data;
    } else {
      if (key>r->key) {
	if (r->right) {
	  add_node(r->right,key,data);
	  upd_height(r->right);
	  r->right=fix_height(r->right);
	} else {
	  r->right = new node(key,data);
	}
    } else {
      if (r->left) {
	add_node(r->left,key,data);
	upd_height(r->left);
	r->left=fix_height(r->left);
      } else {
	r->left = new node(key,data);
      }
    }
  }
}


  template<typename KT,typename DT> 
  const typename avl_tree<KT,DT>::node* 
  avl_tree<KT,DT>::int_find(node* n, const KT& k) const 
  {
    if (!n)
      return nullptr;
    if (n->key==k)
      return n;
    else if (k>n->key)
      return int_find(n->right,k);
    else
      return int_find(n->left,k);
  }


  template<typename KT,typename DT> 
  typename avl_tree<KT,DT>::node*
  avl_tree<KT,DT>::delete_node(node* n,const KT& k) 
  {
    if (!n) return nullptr;
    
    if (k>n->key) {
      n->right=delete_node(n->right,k);
    } else if (k<n->key) {
      n->left=delete_node(n->left,k);
    } else {
      if (!n->right) {
	delete n;
	return n->left;
      }
      
      if (!n->left) {
	delete n;
	return n->right;
      }
      
      
      node* victim = n;
      
      n = chop_min(victim->right);
      
      n->right = victim->right;
      n->left = victim->left;
      
      delete victim;
    }
    upd_height(n);
    return fix_height(n);
  }



  template<typename KT,typename DT> 
  typename avl_tree<KT,DT>::node*
  avl_tree<KT,DT>::rotate_right(node* r) {
    if (!r || !r->left) {
      return r;
    }
    node* new_root=r->left;
    r->left=new_root->right;
    upd_height(r);
    new_root->right=r;
    upd_height(new_root);
    return new_root;
  }
  
  template<typename KT,typename DT> 
  typename avl_tree<KT,DT>::node*
  avl_tree<KT,DT>::rotate_left(node* r) {
    if (!r || !r->right) {
      return r;
    }
    node* new_root=r->right;
    r->right=new_root->left;
    upd_height(r);
    new_root->left=r;
    upd_height(new_root);
    return new_root;
  }

}

using namespace tree;



int main() {
  avl_tree<int,int> t;

  t.insert(100,1);
  t.insert(70,2);
  t.insert(50,3);
  std::cout<<t.print()<<std::endl;
 
  t.check_h();
  std::cout<<t.print()<<std::endl;

  t.insert(40,4);
  t.check_h();
  std::cout<<t.print()<<std::endl;

  t.erase(50);
  std::cout<<t.print()<<std::endl;
  t.check_h();
  
  t.insert(29,6);
  t.insert(28,7);
  t.insert(27,8);
  t.insert(26,9);
  t.check_h();

  t.erase(29);
  t.insert(24,10);
  t.insert(23,11);
  t.insert(22,12);
  
  t.check_h();
  t.insert(20,18);
  t.insert(21,13);
  t.insert(19,14);
  t.insert(18,15);
  t.check_h();
  t.insert(17,16);
  t.check_h();
  t.insert(16,17);
  

  std::cout<<"Height="<<t.height()<<std::endl;
  t.check_h();
  for(int i=1000,j=10;i>100;--i,++j) {
    t.insert(i,j);
    try {
      t.check_h();
    } catch (...) {
      std::cout<<t.print()<<std::endl;
      std::cout<<"catch: "<<i<<" "<<j<<std::endl;
      std::exit(0);
    }
  }
  t.check_h();

  std::cout<<"Height="<<t.height()<<std::endl;

  for(int i=1001,j=1000;j<10280000;j++,i++)
    t.insert(i,j);
  std::cout<<"Height="<<t.height()<<std::endl;

  for(int i=10000;i<100000;i+=2) {
    t.erase(i);
    auto s=t.find(i);
    if (s.first) {
      std::cout<<"Find i="<<i<<" but haven't found.\n";
    }
  }
  
  t.check_h();
}



