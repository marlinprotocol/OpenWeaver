#ifndef MARLIN_SIMULATOR_CORE_INDEXEDSTORAGE_HPP
#define MARLIN_SIMULATOR_CORE_INDEXEDSTORAGE_HPP

#include <cstdint>
#include <list>
#include <tuple>
#include <map>
#include <memory>

namespace marlin {
namespace simulator {

template<typename T, typename MemberType, MemberType (T::*member)()>
struct MapIndex {
private:
	using TPtrType = typename std::list<std::shared_ptr<T>>::iterator;
	std::map<MemberType, TPtrType> index;
public:
	void add(TPtrType t_ptr) {
		index[t_ptr->*member()] = t_ptr;
	}

	void remove(TPtrType t_ptr) {
		index.erase(t_ptr->*member());
	}

	TPtrType front() {
		return index.begin();
	}
};

template<typename T, typename MemberType, MemberType (T::*member)()>
struct MultimapIndex {
private:
	using TPtrType = typename std::list<std::shared_ptr<T>>::iterator;
	std::multimap<MemberType, TPtrType> index;
public:
	void add(TPtrType t_ptr) {
		index.insert(std::make_pair(t_ptr->*member(), t_ptr));
	}

	void remove(TPtrType t_ptr) {
		auto [begin, end] = index.equal_range(t_ptr->*member());

		while(begin != end) {
			if(begin->second == t_ptr) {
				begin = index.erase(begin);
			} else {
				begin++;
			}
		}

		index.insert(std::make_pair(t_ptr->*member(), t_ptr));
	}

	TPtrType front() {
		return index.begin()->second;
	}
};

template<
	typename T,
	typename ...Indexes
>
class IndexedStorage {
private:
	std::list<std::shared_ptr<T>> storage;
	std::tuple<Indexes...> indexes;
public:
	void add(std::shared_ptr<T> t) {
		auto new_iter = storage.insert(storage.end(), t);
		std::apply([new_iter](auto& ...x) { (..., x.add(new_iter)); }, indexes);
	}

	template<size_t idx>
	auto get() {
		return std::get<idx>(indexes);
	}
};

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_CORE_INDEXEDSTORAGE_HPP
