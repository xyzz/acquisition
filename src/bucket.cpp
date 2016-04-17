#include "bucket.h"

// this is required by std::map's operator[]
Bucket::Bucket()
{}

Bucket::Bucket(const ItemLocation &location):
    location_(location)
{}

void Bucket::AddItem(const std::shared_ptr<Item> & item) {
    items_.push_back(item);
}

void Bucket::Sort(const Column &column, Qt::SortOrder order)
{
    std::sort(begin(items_), end(items_), [&](const std::shared_ptr<Item>& lhs, const std::shared_ptr<Item>& rhs) {
        if (order == Qt::AscendingOrder) {
            return column.lt(rhs.get(),lhs.get());
        }
        return column.lt(lhs.get(),rhs.get());
    });
}
