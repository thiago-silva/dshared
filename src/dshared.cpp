#include "dshared.hpp"

extern MManager* manager;

sdict_value_t::sdict_value_t(int _tag, int _num, const char* _str, offset_ptr<void> _d, void_allocator alloc)
  : tag(_tag), num(_num), str(_str, alloc), d(_d)
{}

MManager::MManager(const char* _name, unsigned long _size) :
  name(_name),
  size(_size),
  segment(create_only , name /*segment name*/ ,_size), //segment size in bytes
  void_alloc(segment.get_segment_manager())
{}

sdict* MManager::create_sdict() {
  return segment.construct<sdict>
    (boost::interprocess::anonymous_instance)(std::less<char_string>(), void_alloc);
}

char_string* MManager::create_string(const char* str) {
  char_string* s = segment.construct<char_string>
    (boost::interprocess::anonymous_instance)(str, void_alloc);
  return  s;//char_string(str, void_alloc);
}

sdict_value_t* MManager::create_null_value() {
  return segment.construct<sdict_value_t>
    (boost::interprocess::anonymous_instance)(0, 0, "", (void*)NULL, void_alloc);
}

sdict_value_t* MManager::create_number_value(long num) {
  return segment.construct<sdict_value_t>
    (boost::interprocess::anonymous_instance)(1, num, "", (void*)NULL, void_alloc);
}

sdict_value_t* MManager::create_string_value(const char* str) {
  return segment.construct<sdict_value_t>
    (boost::interprocess::anonymous_instance)(2, 0, str, (void*)NULL, void_alloc);
}

sdict_value_t* MManager::create_sdict_value(sdict* d) {
  return segment.construct<sdict_value_t>
    (boost::interprocess::anonymous_instance)(3, 0, "", d, void_alloc);
}

MManager::~MManager() {
  shared_memory_object::remove(name);
}


void sdict_set_null_item(sdict* sd, const char* strkey) {
  char_string*  key = manager->create_string(strkey);
  sdict_value_t* val = manager->create_null_value();
  sdict_pair_type p(*key, val);
  sd->insert(p);
}

void sdict_set_string_item(sdict* sd, const char* strkey, const char* value) {
  char_string*  key = manager->create_string(strkey);
  sdict_value_t* val = manager->create_string_value(value);
  sdict_pair_type p(*key, val);
  sd->insert(p);
}

void sdict_set_number_item(sdict* sd, const char* strkey, long num) {
  char_string*  key = manager->create_string(strkey);
  sdict_value_t* val = manager->create_number_value(num);
  sdict_pair_type p(*key, val);
  sd->insert(p);
}

void sdict_set_sdict_item(sdict* sd, const char* strkey, sdict* value) {
  char_string*  key = manager->create_string(strkey);
  sdict_value_t* val = manager->create_sdict_value(value);
  sdict_pair_type p(*key, val);
  sd->insert(p);
}

offset_ptr<sdict_value_t> sdict_get_item(sdict* sd, const char* strkey) {
  char_string* k = manager->create_string(strkey);
  return sd->at(*k);
}
