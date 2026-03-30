#ifndef BLACKBOARDENTRY_HPP
#define BLACKBOARDENTRY_HPP

#include <core/Blackboard.hpp>
#include <memory>
#include <type_traits>

/// \brief Обертка над BB, описывающая одну запись
template<typename T>
class BlackboardEntry {
public:
	BlackboardEntry(std::string aName, std::shared_ptr<Blackboard> aBb) : name{aName}, bb{aBb}
	{
	}

	/// \brief Установить значение
	/// \param aValue значение типа T, если T это enum то записывается int
	/// \return true если запись успешна
	bool set(T aValue)
	{
		if constexpr (std::is_enum_v<T>) {
			int value = static_cast<int>(aValue);
			return bb->set(name, value);
		} else {
			return bb->set(name, aValue);
		}
	}

	/// \brief Есть ли значение в ии
	/// \return true если есть
	bool present() const
	{
		return bb->has(name);
	}

	/// \brief Прочитать значение типа T (throwable!)
	/// \return значение
	T read() const
	{
		if constexpr (std::is_enum_v<T>) {
			int value = bb->get<int>(name).value();
			return static_cast<T>(value);
		} else {
			return bb->get<T>(name).value();
		}
	}

	/// \brief Подписать entryObserver
	/// \param aObs обсервер
	void subscribe(AbstractEntryObserver *aObs)
	{
		return bb->subscribe(name, aObs);
	}

	auto operator=(T aValue)
	{
		return set(aValue);
	}

	T operator()() const
	{
		return read();
	}

	std::string_view getName() const
	{
		return name;
	}

private:
	std::string name;
	std::shared_ptr<Blackboard> bb;
};

#endif // BLACKBOARDENTRY_HPP
