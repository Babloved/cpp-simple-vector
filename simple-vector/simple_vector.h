#pragma once

#include <cassert>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include "array_ptr.h"

class ReserveProxyObj{
public:
    ReserveProxyObj(size_t capacity) : capacity_(capacity){
    };

    size_t GetCapacity(){
        return capacity_;
    }

private:
    size_t capacity_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve){
    return ReserveProxyObj(capacity_to_reserve);
};

template<typename Type>
class SimpleVector{
public:
    using Iterator = Type *;
    using ConstIterator = const Type *;
    SimpleVector() noexcept = default;

    SimpleVector(size_t size, const Type &value) :size_(size), capacity_(size),p_data_(size){
        std::fill(this->begin(), this->end(), value);
    }

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) :SimpleVector(size,Type()){}

    explicit SimpleVector(ReserveProxyObj reserveProxyObj) :SimpleVector(reserveProxyObj.GetCapacity()){
        size_ = 0;
    }

    // Создаёт вектор из size элементов, инициализированных значением value


    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) : size_(init.size(),capacity_(init.size()),p_data_(init.size())){
        std::copy(std::make_move_iterator(init.begin()), std::make_move_iterator(init.end()), p_data_.Get());
    }

    SimpleVector(const SimpleVector &other){
        SimpleVector<Type> temp_copy(other.size_);
        std::copy(other.begin(), other.end(), temp_copy.begin());
        this->swap(temp_copy);
    }

    SimpleVector(SimpleVector &&other) : size_(other.size_), capacity_(other.capacity_),
                                         p_data_(std::move(other.p_data_)){
        other.capacity_ = 0;
        other.size_ = 0;
    }

    SimpleVector &operator=(SimpleVector &&other){
        if (&other == this){
            return *this;
        }
        SimpleVector temp(other);
        this->swap(other);
        return *this;
    }

    SimpleVector &operator=(const SimpleVector &rhs){
        if (&rhs == this){
            return *this;
        }
        SimpleVector<Type> temp_copy(rhs);
        this->swap(temp_copy);
        return *this;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(Type item){
        if (size_ < capacity_){
            this->At(size_++) = std::move(item);
        } else{
            this->Resize(size_ + 1);
            this->At(size_ - 1) = std::move(item);
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, Type value){
        assert(pos >= this->begin() && pos <= this->end());
        if (size_ >= capacity_){
            capacity_ = std::max(size_ * 2, static_cast<size_t>(1));
            ArrayPtr<Type> p_temp_new_vec(capacity_);
            auto insert_iter = std::copy(std::make_move_iterator(this->begin()),
                                         std::make_move_iterator(const_cast<Iterator>(pos)), p_temp_new_vec.Get());
            *insert_iter = std::move(value);
            std::copy(std::make_move_iterator(const_cast<Iterator>(pos)), std::make_move_iterator(this->end()),
                      std::next(insert_iter));

            p_data_.swap(p_temp_new_vec);
            ++size_;
            return static_cast<Iterator>(insert_iter);
        } else{
            ++size_;
            std::copy_backward(std::make_move_iterator(const_cast<Iterator>(pos)),
                               std::make_move_iterator(std::prev(this->end())), this->end());
            *const_cast<Iterator>(pos) = std::move(value);
            return const_cast<Iterator>(pos);
        }

    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept{
        return size_ == 0;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept{
        assert(this->IsEmpty());
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos){
        assert(pos >= this->begin() && pos < this->end());
        std::copy(std::make_move_iterator(std::next(const_cast<Iterator>(pos))), std::make_move_iterator(this->end()),
                  Iterator(pos));
        --size_;
        return const_cast<Iterator>(pos);
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector &other) noexcept{
        p_data_.swap(other.p_data_);
        std::swap(capacity_, other.capacity_);
        std::swap(size_, other.size_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept{
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept{
        return capacity_;
    }

    // Возвращает ссылку на элемент с индексом index
    Type &operator[](size_t index) noexcept{
        return p_data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type &operator[](size_t index) const noexcept{
        return p_data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type &At(size_t index){
        if (index >= size_){
            throw std::out_of_range("");
        }
        return p_data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type &At(size_t index) const{
        if (index >= size_){
            throw std::out_of_range("");
        }
        return p_data_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept{
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size){
        if (new_size > size_){
            ArrayPtr<Type> p_temp_new_vec(new_size * 2);
            std::copy(std::make_move_iterator(this->begin()), std::make_move_iterator(this->end()),
                      p_temp_new_vec.Get());
            p_data_.swap(p_temp_new_vec);
            capacity_ = new_size * 2;
        }
        size_ = new_size;
    }

    void Reserve(size_t new_capacity){
        if (new_capacity > capacity_){
            ArrayPtr<Type> p_temp_new_vec(new_capacity);
            std::copy(std::make_move_iterator(this->begin()), std::make_move_iterator(this->end()),
                      p_temp_new_vec.Get());
            p_data_.swap(p_temp_new_vec);
            capacity_ = new_capacity;
        }

    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept{
        return p_data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept{
        return p_data_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept{
        return p_data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept{
        return p_data_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept{
        return p_data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept{
        return p_data_.Get() + size_;
    }

private:
    ArrayPtr<Type> p_data_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template<typename Type>
inline bool operator==(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs){
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<typename Type>
inline bool operator!=(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs){
    return !(lhs == rhs);
}

template<typename Type>
inline bool operator<(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs){
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<typename Type>
inline bool operator<=(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs){
    return !(rhs < lhs);
}

template<typename Type>
inline bool operator>(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs){
    return rhs < lhs;
}

template<typename Type>
inline bool operator>=(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs){
    return !(lhs < rhs);
}