#ifndef DSHARED_HPP
#define DSHARED_HPP
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <sys/types.h>

using namespace boost::interprocess;

typedef managed_shared_memory::segment_manager  segment_manager_t;
typedef allocator<void, segment_manager_t> void_allocator;
typedef allocator<char, segment_manager_t> char_allocator;
typedef basic_string <char, std::char_traits<char>, char_allocator> char_string;

typedef std::pair<const pid_t, void*> cache_pair_t;
typedef allocator<cache_pair_t, segment_manager_t> cache_allocator;
typedef map<pid_t, void*, std::less<pid_t>, cache_allocator> cache_t;

class MManager;

class smap_value_t {
public:
  typedef enum {
    NIL,
    NUMBER,
    STRING,
    SDICT,
    SLIST,
    PYOBJ,
  } TAG;

  int tag;
  int num;
  char_string str;
  offset_ptr<void> d;

  offset_ptr<void> _odict_;

  void* pyclass; //WARNING: we are assuming this address points to the same
                 //         object in the concurring processes.

  offset_ptr<cache_t> pycache;

  smap_value_t(MManager*, int _tag, int _num, const char* _str, offset_ptr<void> _d,
                offset_ptr<void> __odict__, void* pyclass,
                void_allocator alloc);

  // caches for heap PyObject* handlers for each process.
  bool has_cache();
  void* cache();
  void cache_obj(void* p);
};


struct strnum_cmp : public std::binary_function<char_string, char_string, bool> {
  bool operator() (const char_string& __x, const char_string& __y) const;
};


typedef offset_ptr<smap_value_t> pair_value_t;
typedef std::pair<const char_string, pair_value_t> smap_pair_type;
typedef allocator<smap_pair_type, segment_manager_t>  smap_pair_type_allocator;
typedef map<char_string, pair_value_t, strnum_cmp, smap_pair_type_allocator>  smap;

class MManager {
public:
  MManager(const char* _name, long long _size);
  ~MManager();

  smap*          create_smap();
  char_string*    create_string(const char* str);
  smap_value_t*  create_null_value();
  smap_value_t*  create_number_value(long num);
  smap_value_t*  create_string_value(const char* str);
  smap_value_t*  create_sdict_value(offset_ptr<smap> d);
  smap_value_t*  create_slist_value(offset_ptr<smap> d);
  smap_value_t*  create_obj_value(offset_ptr<smap> _dict_, void* pytype);

  const char* name;
  unsigned long size;
  managed_shared_memory segment;
  void_allocator void_alloc;
};

void smap_set_null_item(offset_ptr<smap> sd, const char* key);
void smap_set_string_item(offset_ptr<smap> sd, const char* key, const char* value);
void smap_set_number_item(offset_ptr<smap> sd, const char* key, long num);
void smap_set_sdict_item(offset_ptr<smap> sd, const char* key, offset_ptr<smap> value);
void smap_set_slist_item(offset_ptr<smap> sd, const char* key, offset_ptr<smap> value);
void smap_set_obj_item(offset_ptr<smap> sd, const char* key, offset_ptr<smap> value,
                        void* pyclass, void* local_obj_value);

offset_ptr<smap_value_t> smap_get_item(offset_ptr<smap> sd, const char* key);
bool smap_has_item(offset_ptr<smap> sd, const char* key);
void smap_delete_item(offset_ptr<smap> sd, const char* key);

#endif
