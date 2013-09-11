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

typedef map<pid_t, void*>  pyobj_cache_t;

class sdict_value_t {
public:
  typedef enum {
    NIL,
    NUMBER,
    STRING,
    PYDICT,
    SDICT
  } TAG;

  int tag;
  int num;
  char_string str;
  offset_ptr<void> d;
  pyobj_cache_t pyobj_cache;
  sdict_value_t(int _tag, int _num, const char* _str, offset_ptr<void> _d, void_allocator alloc);

  bool has_pyobj_cache();
  void* cache();
  void cache_obj(void* p);
};

typedef offset_ptr<sdict_value_t> pair_value_t;
typedef std::pair<const char_string, pair_value_t> sdict_pair_type;
typedef allocator<sdict_pair_type, segment_manager_t>  sdict_pair_type_allocator;
typedef map< char_string, pair_value_t, std::less<char_string>, sdict_pair_type_allocator>  sdict;

class MManager {
public:
  MManager(const char* _name, unsigned long _size);
  ~MManager();

  sdict*          create_sdict();
  char_string*    create_string(const char* str);
  sdict_value_t*  create_null_value();
  sdict_value_t*  create_number_value(long num);
  sdict_value_t*  create_string_value(const char* str);
  sdict_value_t*  create_sdict_value(sdict* d);

private:
  const char* name;
  unsigned long size;
  managed_shared_memory segment;
  void_allocator void_alloc;
};

void sdict_set_null_item(sdict* sd, const char* key);
void sdict_set_string_item(sdict* sd, const char* key, const char* value);
void sdict_set_number_item(sdict* sd, const char* key, long num);
void sdict_set_sdict_item(sdict* sd, const char* key, sdict* value);
offset_ptr<sdict_value_t> sdict_get_item(sdict* sd, const char* key);
bool sdict_has_item(sdict* sd, const char* key);
void sdict_delete_item(sdict* sd, const char* key);

#endif
