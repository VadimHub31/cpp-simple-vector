#pragma once

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    ReserveProxyObj(size_t capacity) : capacity_(capacity) {}
    size_t capacity_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) {
        ArrayPtr<Type> new_arr(size);
        std::fill(new_arr.Get(), new_arr.Get() + size, Type());
        arr_.swap(new_arr);
        size_ = size;
        capacity_ = size;
    }

    SimpleVector(ReserveProxyObj reserve_obj) {
        if (reserve_obj.capacity_ > capacity_) {
            ArrayPtr<Type> new_arr(reserve_obj.capacity_);
            std::copy(begin(), end(), new_arr.Get());
            arr_.swap(new_arr);
            capacity_ = reserve_obj.capacity_;
        }
        else { 
            return;
        }
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) {
        if (size == 0) {
            SimpleVector();
        }
        else {
            ArrayPtr<Type> new_arr(size);
            std::fill(new_arr.Get(), new_arr.Get() + size, value);
            arr_.swap(new_arr);
            size_ = size;
            capacity_ = size;
        }
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) {
        if (init.size() == 0) {
            SimpleVector();
        }
        else {
            ArrayPtr<Type> new_arr(init.size());
            std::copy(init.begin(), init.end(), new_arr.Get());
            arr_.swap(new_arr);
            size_ = init.size();
            capacity_ = init.size();
        }
    }

    SimpleVector(const SimpleVector& other) {
        ArrayPtr<Type> tmp_arr(other.GetSize());
        std::copy(other.begin(), other.end(), tmp_arr.Get());
        arr_.swap(tmp_arr);
        size_ = other.GetSize();
        capacity_ = other.GetCapacity();
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            auto tmp(rhs);
            arr_.swap(tmp.arr_);
            size_ = rhs.GetSize();
            capacity_ = rhs.GetCapacity();
        }
        return *this;
    }

    SimpleVector(SimpleVector&& other) 
    : arr_(std::move(other.arr_)), size_(std::exchange(other.size_, 0)), capacity_(std::exchange(other.capacity_, 0)) {}

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs) {
            arr_ = std::move(rhs.arr_);
            size_ = std::exchange(rhs.size_, 0);
            capacity_ = std::exchange(rhs.capacity_, 0);
        }
        return *this; 
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> new_arr(new_capacity);
            std::move(begin(), end(), new_arr.Get());
            arr_.swap(new_arr);
            capacity_ = std::move(new_capacity);
        }
        else {
            return;
        }
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (size_ < capacity_) {
            arr_[size_] = item;
            ++size_;
        }
        else {
            size_t new_capacity = 0;
            new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_arr(new_capacity);
            std::copy(begin(), end(), new_arr.Get());
            new_arr[size_] = item;
            arr_.swap(new_arr);
            capacity_ = new_capacity;
            ++size_;
        }
    }

    void PushBack(Type&& item) {
        if (size_ < capacity_) {
            arr_[size_] = std::move(item);
            ++size_;
        }
        else {
            size_t new_capacity = 0;
            new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_arr(new_capacity);
            std::move(begin(), end(), new_arr.Get());
            new_arr[size_] = std::move(item);
            arr_ = std::move(new_arr);
            capacity_ = std::move(new_capacity);
            ++size_;
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        auto insert_position = std::distance(cbegin(), pos);
        if (size_ < capacity_) {
            std::copy_backward(arr_.Get() + insert_position, end(), end() + 1); 
            arr_[insert_position] = value;
            ++size_;
        }
        else {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_arr(new_capacity);
            std::copy(begin(), end(), new_arr.Get());
            arr_.swap(new_arr);

            std::copy_backward(arr_.Get() + insert_position, end(), end() + 1);
            arr_[insert_position] = value;
            
            ++size_;
            capacity_ = new_capacity;
        }
        return arr_.Get() + insert_position;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        auto insert_position = std::distance(cbegin(), pos);
        if (size_ < capacity_) {
            std::move(arr_.Get() + insert_position, end(), arr_.Get() + insert_position + 1); 
            arr_[insert_position] = std::move(value);
            ++size_;
        }
        else {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_arr(new_capacity);
            std::move(begin(), end(), new_arr.Get());
            arr_ = std::move(new_arr);
            std::move(arr_.Get() + insert_position, end(), arr_.Get() + insert_position + 1);
            arr_[insert_position] = std::move(value);
            
            ++size_;
            capacity_ = std::move(new_capacity);
        }
        return arr_.Get() + insert_position;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (size_) {
            --size_;
        }
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        auto delete_position = std::distance(cbegin(), pos);
        std::move(arr_.Get() + delete_position + 1, end(), arr_.Get() + delete_position);
        --size_;
        return arr_.Get() + delete_position;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        arr_.swap(other.arr_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return arr_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return arr_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("out of range error");
        }
        else {
            return arr_[index];
        }
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("out of range error");
        }
        else {
            return arr_[index];
        }
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ -= (size_ - new_size);
        }
        else if (new_size > size_ && new_size <= capacity_) {
            Fill(end(), arr_.Get() + new_size);
            size_ = new_size;
        }
        else {
            size_t new_capacity = 0;
            if (capacity_ == 0) {
                new_capacity = new_size;
            }
            else {
                new_capacity = capacity_;
                while (new_capacity < new_size) {
                    new_capacity *= 2;
                }
            }
            ArrayPtr<Type> new_arr(new_capacity);
            std::move(begin(), end(), new_arr.Get());
            Fill(new_arr.Get() + size_, new_arr.Get() + new_size);
            size_ = new_size;
            capacity_ = size_ * 2;
            arr_.swap(new_arr);
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return arr_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return arr_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return arr_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return arr_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return arr_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return arr_.Get() + size_;
    }
private:
    void Fill(Iterator begin, Iterator end) {
        for (auto it = begin; it != end; ++it) {
            *it = Type();
        }
    }

    ArrayPtr<Type> arr_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    if (&lhs == &rhs) { 
        return true;
    }
    else if (lhs.GetSize() != rhs.GetSize()) {
        return false;
    }
    return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
