#include <memory>
#include <utility>
#include <iostream>
#include <atomic>

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

	// Запрет копирования и оператора копирующего присваивания
	myUnique_ptr(const myUnique_ptr&) = delete;
	myUnique_ptr& operator=(const myUnique_ptr&) = delete;

	// Конструктор перемещения
	myUnique_ptr(myUnique_ptr&& other) noexcept
		: ptr(std::exchange(other.ptr, nullptr))
	{ }

	// Перемещающий оператор присваивания
	myUnique_ptr& operator=(myUnique_ptr&& other) noexcept {
		if (this == &other)
			return *this;

		delete ptr;
		ptr = std::exchange(other.ptr, nullptr);
		return *this;
	}

	~myUnique_ptr() {
		delete ptr;
		ptr = nullptr;
	}

	T& operator*() const { return *ptr; }
	T* operator->() const { return ptr; }
	explicit operator bool() const noexcept { return ptr != nullptr; }

	T* get() const noexcept { return ptr; }

	T* release() {
		T* temp = ptr;
		ptr = nullptr;
		return temp;
	}

	void reset(T* p = nullptr) {
		if (ptr == p) return;

		delete ptr;
		ptr = p;
	}

	void swap(myUnique_ptr& other) noexcept{
		std::swap(ptr, other.ptr);
	}

	std::default_delete<T> get_deleter() {
		return std::default_delete<T>();
	}
};
//==============================================================================
template<typename T>
class myShared_ptr {
private:
	struct Control_block {
		std::atomic<int> ref_count = 0;				// Счетчик ссылок
		std::atomic<int> weak_count = 0;
	};

	T* ptr = nullptr;					// Данные
	Control_block* ctrl_blk = nullptr;	// Указатель на счетчик

	// Конструктор для метода lock() из myWeak_ptr
	explicit myShared_ptr(T* p, Control_block* cb, bool tf)
		: ptr(p), ctrl_blk(cb)
	{
		if (p && cb && !tf)
			ctrl_blk->ref_count.fetch_add(1);
	}

public:
	explicit myShared_ptr(T* p = nullptr)
		: ptr(p) {
		if (p)
			ctrl_blk = new Control_block{ 1, 0 };
	}

	myShared_ptr(const myShared_ptr& other) noexcept
		:ptr(other.ptr), ctrl_blk(other.ctrl_blk) {
		// Конструктор копирования
		if (ctrl_blk)
		ctrl_blk->ref_count.fetch_add(1);
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

	// Конструктор перемещения
	myShared_ptr(myShared_ptr&& other) noexcept
		: ptr(std::exchange(other.ptr, nullptr)), ctrl_blk(std::exchange(other.ctrl_blk, nullptr))
	{ }
		
	myShared_ptr& operator=(myShared_ptr&& other) noexcept {
		// Перемещающий оператор присваивания
		if (this == &other)
			return *this;

		myShared_ptr temp(std::move(other));
		swap(temp);
		return *this;
	}

	~myShared_ptr() {
		if (ctrl_blk && ctrl_blk->ref_count.fetch_sub(1) == 1) {
			delete ptr;

			if (ctrl_blk->weak_count.load() == 0)
				delete ctrl_blk;
		}
	}

	T& operator*() const { return *ptr; }
	T* operator->() const { return ptr; }
	explicit operator bool() const noexcept { return ptr != nullptr; }

	T* get() const noexcept { return ptr; }

	// Copy And Swap
	void swap(myShared_ptr& other) noexcept {
		std::swap(ptr, other.ptr);
		std::swap(ctrl_blk, other.ctrl_blk);
	}

	int use_count() const noexcept {
		// Возвращает количество владельцев
		return ctrl_blk ? ctrl_blk->ref_count.load() : 0;
	}

	void reset(T* p = nullptr) {
		if (ptr == p) return;

		myShared_ptr(p).swap(*this);
	}	// Деструктор позаботится о счетчиках

	bool unique() const noexcept {
		return ctrl_blk && ctrl_blk->ref_count.load() == 1;
	}

	bool owner_before(const myShared_ptr& other) const noexcept{
		return ctrl_blk < other.ctrl_blk;
	}


	template<typename> friend class myWeak_ptr;
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
	myWeak_ptr()
		: ptr(nullptr), ctrl_blk(nullptr)
	{ }

	myWeak_ptr(const myShared_ptr<T>& other)
	: ptr(other.ptr), ctrl_blk(other.ctrl_blk) {

		if (ctrl_blk)
			ctrl_blk->weak_count.fetch_add(1);
	}

	myWeak_ptr(const myWeak_ptr& other) noexcept
	: ptr(other.ptr), ctrl_blk(other.ctrl_blk) {

		if (ctrl_blk)
			ctrl_blk->weak_count.fetch_add(1);
	}

	myWeak_ptr& operator=(const myWeak_ptr& other) noexcept {
		// Оператор копирующего присваивания
		if (this == &other)
			return *this;

		myWeak_ptr temp_ptr = other; // Счетчик weak_count увеличится в конструкторе
		swap(temp_ptr);

		return *this;
	}

	myWeak_ptr(myWeak_ptr&& other) noexcept 
		: ptr(std::exchange(other.ptr, nullptr)), ctrl_blk(std::exchange(other.ctrl_blk, nullptr)) 
	{ }

	myWeak_ptr& operator=(myWeak_ptr&& other) noexcept {
		if (this == &other)
			return *this;

		myWeak_ptr temp(std::move(other));
		this->swap(temp);

		return *this;
	}

	~myWeak_ptr() {
		if (ctrl_blk && ctrl_blk->weak_count.fetch_sub(1) == 1 && ctrl_blk->ref_count.load() == 0)
			delete ctrl_blk;
	}

	explicit operator bool() const { return ptr != nullptr; }

	bool expired() const noexcept {
		// Проверяет был ли удален объект
		return ctrl_blk == nullptr || ctrl_blk->ref_count.load() == 0;
	}

	myShared_ptr<T> lock() const {
		// Создаёт и возвращает новый myShared_ptr
		if (!ctrl_blk) return myShared_ptr<T>();

		int cur_ref = ctrl_blk->ref_count.load();

		while (cur_ref != 0) {
			if (ctrl_blk->ref_count.compare_exchange_weak(cur_ref, cur_ref + 1))
				return myShared_ptr<T>(ptr, ctrl_blk, true);
		}

		return myShared_ptr<T>();
	}

	int use_count() const noexcept {
		// Возвращает количество владельцев
		return ctrl_blk ? ctrl_blk->ref_count.load() : 0;
	}

	// Copy And Swap
	void swap(myWeak_ptr& other) noexcept {
		std::swap(ptr, other.ptr);
		std::swap(ctrl_blk, other.ctrl_blk);
	}

	bool owner_before(const myWeak_ptr& other) const noexcept {
		return ctrl_blk < other.ctrl_blk;
	}

	void reset() {
		myWeak_ptr temp;
		this->swap(temp);
	}
};
//==============================================================================
