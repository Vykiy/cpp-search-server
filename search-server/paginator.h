#pragma once
#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator it_begin, Iterator it_end)
        :it_range_(it_begin, it_end) {
    }

    Iterator begin() const {
        return it_range_.first;
    }

    Iterator end() const {
        return it_range_.second;
    }

private:
   std::pair<Iterator, Iterator> it_range_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator it_begin, Iterator it_end, size_t& page_size)
    {
        for (auto it = it_begin; it != it_end;) {
            if (distance(it, it_end) < static_cast<int>(page_size)) {
                pages_.push_back(IteratorRange(it, it_end));
                break;
            } else {
                pages_.push_back(IteratorRange(it, it + page_size));
                advance(it, page_size);
            }
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    size_t size() const {
        return pages_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& output, IteratorRange<Iterator> page) {
    for (auto it = page.begin(); it != page.end(); ++it) {
        output << *it;
    }
    return output;
}
