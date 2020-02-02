#ifndef MARLIN_SIMULATOR_CORE_INDEXEDSTORAGE_HPP
#define MARLIN_SIMULATOR_CORE_INDEXEDSTORAGE_HPP

#include <list>
#include <tuple>
#include <map>
#include <memory>

namespace marlin {
namespace simulator {

template<typename T, typename MemberType, MemberType (T::*member)()>
struct MapIndex {
private:
	std::map<MemberType, std::shared_ptr<T>> index;
public:
	void add(std::shared_ptr<T> t_ptr) {
		index[(*t_ptr.*member)()] = t_ptr;
	}

	void remove(std::shared_ptr<T> t_ptr) {
		index.erase((*t_ptr.*member)());
	}

	void remove(MemberType m) {
		index.erase(m);
	}

	bool is_empty() {
		return index.empty();
	}

	std::shared_ptr<T> front() {
		return index.begin();
	}

	std::shared_ptr<T> at(MemberType m) {
		return index.at(m);
	}
};

template<typename T, typename MemberType, MemberType (T::*member)()>
struct MultimapIndex {
private:
	std::multimap<MemberType, std::shared_ptr<T>> index;
public:
	void add(std::shared_ptr<T> t_ptr) {
		index.insert(std::make_pair((*t_ptr.*member)(), t_ptr));
	}

	void remove(std::shared_ptr<T> t_ptr) {
		auto [begin, end] = index.equal_range((*t_ptr.*member)());

		while(begin != end) {
			if(begin->second == t_ptr) {
				begin = index.erase(begin);
			} else {
				begin++;
			}
		}
	}

	void remove(MemberType m) {
		index.erase(m);
	}

	bool is_empty() {
		return index.empty();
	}

	std::shared_ptr<T> front() {
		return index.begin()->second;
	}
};

template<
	typename T,
	typename ...Indexes
>
class IndexedStorage {
private:
	std::tuple<Indexes...> indexes;
public:
	void add(std::shared_ptr<T> t) {
		std::apply([t](auto& ...x) { (..., x.add(t)); }, indexes);
	}

	void remove(std::shared_ptr<T> t) {
		std::apply([t](auto& ...x) { (..., x.remove(t)); }, indexes);
	}

	bool is_empty() {
		return std::apply([](auto& ...x) { return (x.is_empty() && ...); }, indexes);
	}

	template<size_t idx>
	auto get() {
		return std::get<idx>(indexes);
	}
};

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_CORE_INDEXEDSTORAGE_HPP
