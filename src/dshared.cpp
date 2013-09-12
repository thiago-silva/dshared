#include "dshared.hpp"
#include <unistd.h>
#include <iostream>

bool
strnum_cmp::operator() (const char_string& x, const char_string& y) const {
  if ((x.size() > 0) && (y.size() > 0) && //trying to compare numbers
      (std::isdigit(x[0])) && (std::isdigit(y[0]))) {
    return atoi(x.c_str()) < atoi(y.c_str());
  } else { //fallback to comparing strings
    return x < y;
  }
}


extern MManager* manager;

sdict_value_t::sdict_value_t(MManager* m, int _tag, int _num, const char* _str, offset_ptr<void> _d, offset_ptr<void> __odict__, void* _pyclass, void_allocator alloc)
  : tag(_tag), num(_num), str(_str, alloc), d(_d), _odict_(__odict__), pyclass(_pyclass), pycache(0)
{

  pycache = m->segment.construct<cache_t>(boost::interprocess::anonymous_instance)
    (std::less<int>() ,alloc);
}

bool
sdict_value_t::has_cache() {
  //std::cout << "sdict_value_t::has_cache...\n";
  bool ret = pycache->find(getpid()) != pycache->end();
  //std::cout << "sdict_value_t::has_cache res: " << ret << "\n";
  return ret;
}
void*
sdict_value_t::cache() {
  if (pycache->size() == 0) {
    std::cerr << "BUG: returning null cache\n";
    throw;
  }
  return (*pycache).at(getpid());
}

void
sdict_value_t::cache_obj(void* p) {
  //std::cout << "caching " << p << "\n";
  (*pycache)[getpid()] = p;
}

MManager::MManager(const char* _name, long long _size) :
  name(_name),
  size(_size),
  segment(create_only , name /*segment name*/ ,_size), //segment size in bytes
  void_alloc(segment.get_segment_manager())

{ }

sdict*
MManager::create_sdict() {

  return segment.construct<sdict>
    (boost::interprocess::anonymous_instance)(strnum_cmp(), void_alloc);
}

char_string*
MManager::create_string(const char* str) {
  char_string* s = segment.construct<char_string>
    (boost::interprocess::anonymous_instance)(str, void_alloc);
  return  s;//char_string(str, void_alloc);
}

sdict_value_t*
MManager::create_null_value() {
  return segment.construct<sdict_value_t>
    (boost::interprocess::anonymous_instance)(this, sdict_value_t::NIL, 0, "", (void*)NULL, (void*)NULL, (void*)NULL, void_alloc);
}

sdict_value_t*
MManager::create_number_value(long num) {
  return segment.construct<sdict_value_t>
    (boost::interprocess::anonymous_instance)(this, sdict_value_t::NUMBER, num, "", (void*)NULL, (void*)NULL, (void*)NULL, void_alloc);
}

sdict_value_t*
MManager::create_string_value(const char* str) {
  return segment.construct<sdict_value_t>
    (boost::interprocess::anonymous_instance)(this, sdict_value_t::STRING, 0, str, (void*)NULL, (void*)NULL, (void*)NULL, void_alloc);
}

sdict_value_t*
MManager::create_sdict_value(offset_ptr<sdict> d) {
  return segment.construct<sdict_value_t>
    (boost::interprocess::anonymous_instance)(this, sdict_value_t::SDICT, 0, "", d, (void*)NULL, (void*)NULL, void_alloc);
}

sdict_value_t*
MManager::create_obj_value(offset_ptr<sdict> _dict_, void* pyclass) {
  return segment.construct<sdict_value_t>
    (boost::interprocess::anonymous_instance)(this,
      sdict_value_t::PYOBJ, 0, "", (void*)NULL,
      _dict_, pyclass, void_alloc);
}

MManager::~MManager() {
  shared_memory_object::remove(name);
}


void
sdict_set_null_item(offset_ptr<sdict> sd, const char* strkey) {
  char_string*  key = manager->create_string(strkey);
  sdict_value_t* val = manager->create_null_value();
  // sdict_pair_type p(*key, val);
  (*sd)[*key] = val;
}

void
sdict_set_string_item(offset_ptr<sdict> sd, const char* strkey, const char* value) {
  char_string*  key = manager->create_string(strkey);
  sdict_value_t* val = manager->create_string_value(value);
  // sdict_pair_type p(*key, val);
  (*sd)[*key] = val;
  // sd->insert(p);
}

void
sdict_set_number_item(offset_ptr<sdict> sd, const char* strkey, long num) {
  char_string*  key = manager->create_string(strkey);
  sdict_value_t* val = manager->create_number_value(num);
  // sdict_pair_type p(*key, val);
  (*sd)[*key] = val;
}

void
sdict_set_sdict_item(offset_ptr<sdict> sd, const char* strkey, offset_ptr<sdict> value) {
  char_string*  key = manager->create_string(strkey);
  sdict_value_t* val = manager->create_sdict_value(value);
  //std::cout << "*sdict_set_sdict_item " << val << "\n";
  // sdict_pair_type p(*key, val);
  (*sd)[*key] = val;
}

void
sdict_set_obj_item(offset_ptr<sdict> sd, const char* strkey, offset_ptr<sdict> value,
                   void* pyclass, void* local_pyobj) {
  char_string*  key = manager->create_string(strkey);
  sdict_value_t* val = manager->create_obj_value(value, pyclass);
  val->cache_obj(local_pyobj);
  //std::cout << "-->sdict_set_obj_item " << strkey << ":: " << val << "\n";
  //std::cout << "-->sdict_set_obj_item cached:" << local_pyobj << "::" << val->has_cache() << "\n";
  // sdict_pair_type p(*key, val);
  (*sd)[*key] = val;
}

offset_ptr<sdict_value_t>
sdict_get_item(offset_ptr<sdict> sd, const char* strkey) {
  char_string* k = manager->create_string(strkey);
  return sd->at(*k);
}

bool
sdict_has_item(offset_ptr<sdict> sd, const char* key) {
  char_string* k = manager->create_string(key);
  return sd->find(*k) != sd->end();
}

void
sdict_delete_item(offset_ptr<sdict> sd, const char* key) {
  char_string* k = manager->create_string(key);
  sd->erase(*k);
}
