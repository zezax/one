// containers for dfa header

#pragma once

#include <unordered_map>

namespace zezax::red {

template <class K, class V>
class DefaultMap : public std::unordered_map<K, V> {
public:
  typedef std::unordered_map<K, V> Map;

  const V &operator[](const K &key) const {
    const auto it = Map::find(key);
    if (it == Map::end())
      return default_;
    return it->second;
  }

  void set(const K &key, const V &val) {
    if (val != default_)
      Map::operator[](key) = val;
  }

  // this goes on infinitely; iterators make no sense
  Map::iterator begin() const = delete;
  Map::iterator cbegin() const = delete;
  Map::iterator end() const = delete;
  Map::iterator cend() const = delete;

  // under-the-covers access
  const Map &getMap() const { return static_cast<const Map &>(*this); }
  Map &getMap() { return static_cast<Map &>(*this); }

private:
  static constexpr V default_{};
};

} // namespace zezax::red
