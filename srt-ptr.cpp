#include <memory>
#include <utility>
#include <iostream>

template<typename T> class myWeak_ptr;
//==============================================================================
template<typename T>
class myUnique_ptr {
private:
	T* ptr = nullptr;

public:
	// explicit - запрет компилятору делать неявные преобразования
	explicit myUnique_ptr(T* p = nullptr)
		: ptr(p)
	{ }

	// Запрет копирования и присваивания
	myUnique_ptr(const myUnique_ptr&) = delete;
	myUnique_ptr& operator=(const myUnique_ptr&) = delete;

	// Конструктор перемещения
	myUnique_ptr(myUnique_ptr&& other) noexcept {
		ptr = other.ptr;
		other.ptr = nullptr;
	}
	// Перемещающее копирование
	myUnique_ptr& operator=(myUnique_ptr&& other) noexcept {
		if (this == &other)
			return *this;

		delete ptr;
		ptr = other.ptr;
		other.ptr = nullptr;
		return *this;
	}

	~myUnique_ptr() {
		delete ptr;
		ptr = nullptr;
	}
	
	// Разыменование
	T& operator*() const { return *ptr; }
	T* operator->() const { return ptr; }

	T* get() const { return ptr; }

	T* release() {
		T* temp = ptr;
		ptr = nullptr;
		return temp;
	}

	void reset(T* p = nullptr) {
		delete ptr;
		ptr = p;
	}
};
//==============================================================================
template<typename T>
class myShared_ptr {
private:
	struct Control_block {
		int ref_count = 0;				// Счетчик ссылок
		int weak_count = 0;
	};

	T* ptr = nullptr;					// Данные
	Control_block* ctrl_blk = nullptr;	// Указатель на счетчик

public:
	// Copy And Swap
	void swap(myShared_ptr& other) noexcept {
		T* temp_ptr = ptr;
		Control_block* temp_blk = ctrl_blk;

		ptr = other.ptr;
		ctrl_blk = other.ctrl_blk;

		other.ptr = temp_ptr;
		other.ctrl_blk = temp_blk;
	}

	explicit myShared_ptr(T* p = nullptr)
		: ptr(p) {
		if (p != nullptr) {
			ctrl_blk = new Control_block();
			ctrl_blk->ref_count = 1;
		}
		else
			return;
	}

	myShared_ptr(const myShared_ptr& other) noexcept {
		// Конструктор копирования
		ptr = other.ptr;
		ctrl_blk = other.ctrl_blk;

		if (ctrl_blk)
		ctrl_blk->ref_count++;
	}
	// Доработка!
	myShared_ptr& operator=(const myShared_ptr& other) noexcept {
		// Оператор копирующего присваивания
		if (this == &other)
			return *this;

		myShared_ptr temp_ptr = other; // Счетчик ref_count увеличится в конструкторе
		swap(temp_ptr);

		return *this;
	}

	myShared_ptr(myShared_ptr&& other) noexcept {
		// Конструктор перемещения

		ptr = std::move(other.ptr);
		ctrl_blk = std::move(other.ctrl_blk);
		other.ptr = nullptr;
		other.ctrl_blk = nullptr;
	}

	myShared_ptr& operator=(myShared_ptr&& other) noexcept {
		// Перемещающий оператор присваивания
		if (this == &other)
			return *this;

		if (ctrl_blk) {
			ctrl_blk->ref_count--;

			if (ctrl_blk->ref_count == 0) {
				delete ptr;

				if (ctrl_blk->weak_count == 0)
					delete ctrl_blk;
			}
		}

		ptr = other.ptr;
		ctrl_blk = other.ctrl_blk;

		other.ptr = nullptr;
		other.ctrl_blk = nullptr;

		return *this;
	}

	~myShared_ptr() {
		if (ctrl_blk == nullptr)
			return;

		if (ctrl_blk) {
			ctrl_blk->ref_count--;

			if (ctrl_blk->ref_count == 0) {
				delete ptr;

				if (ctrl_blk->weak_count == 0)
					delete ctrl_blk;
			}
		}
	}

	T& operator*() const { return *ptr; }
	T* operator->() const { return ptr; }

	friend class myWeak_ptr<T>;
};
//==============================================================================
template<typename T>
class myWeak_ptr {
private:
	T* ptr = nullptr;
	// Обязательно typename
	// (Указываем из какого класса вытягивать структуру)
	typename myShared_ptr<T>::Control_block* ctrl_blk = nullptr;

public:
	// Copy And Swap
	void swap(myWeak_ptr& other) noexcept {
		T* temp_ptr = ptr;
		typename myShared_ptr<T>::Control_block* temp_blk = ctrl_blk;

		ptr = other.ptr;
		ctrl_blk = other.ctrl_blk;

		other.ptr = temp_ptr;
		other.ctrl_blk = temp_blk;
	}

	myWeak_ptr(const myShared_ptr<T>& other) {
		ptr = other.ptr;
		ctrl_blk = other.ctrl_blk;

		if (ctrl_blk)
			ctrl_blk->weak_count++;
	}

	myWeak_ptr(const myWeak_ptr& other) noexcept {
		ptr = other.ptr;
		ctrl_blk = other.ctrl_blk;

		if (ctrl_blk)
			ctrl_blk->weak_count++;
	}

	myWeak_ptr& operator=(const myWeak_ptr& other) noexcept {
		// Оператор копирующего присваивания
		if (this == &other)
			return *this;

		myWeak_ptr temp_ptr = other; // Счетчик weak_count увеличится в конструкторе
		swap(temp_ptr);

		return *this;
	}

	myWeak_ptr(myWeak_ptr&& other) noexcept {
		ptr = std::move(other.ptr);
		ctrl_blk = std::move(other.ctrl_blk);
		other.ptr = nullptr;
		other.ctrl_blk = nullptr;
	}

	myWeak_ptr& operator=(myWeak_ptr&& other) noexcept {
		if (this == &other)
			return *this;

		if (ctrl_blk)
			ctrl_blk->weak_count--;

		if (ctrl_blk->weak_count == 0 && ctrl_blk->ref_count == 0)
			delete ctrl_blk;

		ptr = other.ptr;
		ctrl_blk = other.ctrl_blk;

		other.ctrl_blk = nullptr;
		other.ptr = nullptr;

		return *this;
	}

	~myWeak_ptr() {
		if (ctrl_blk == nullptr)
			return;

		if (ctrl_blk)
			ctrl_blk->weak_count--;

		if (ctrl_blk->weak_count == 0 && ctrl_blk->ref_count == 0)
			delete ctrl_blk;
	}

	myShared_ptr<T> lock() {
		myShared_ptr<T> result;

		if (expired())
			return result;

		result.ptr = ptr;
		result.ctrl_blk = ctrl_blk;
		result.ctrl_blk->ref_count++;

		return result;
	}

	bool expired() {
		if (!ctrl_blk) return true;

		if (ctrl_blk->ref_count == 0) return true;
		
		return false;
	}
};
//==============================================================================

int main() {
		myShared_ptr<int> sp1(new int(500));
		myWeak_ptr<int> wp(sp1);
		
		if (!wp.expired()) std::cout << "GOOD" << '\n\n';

		myShared_ptr<int> sp2 = wp.lock();
		std::cout << *sp2 << '\n\n';

	return 0;
}