#include "bucket.h"
#include "QMessageBox"

// this is required by std::map's operator[]
Bucket::Bucket()
{}

Bucket::Bucket(const ItemLocation &location):
    location_(location)
{}

void Bucket::AddItem(const std::shared_ptr<Item> & item) {
    items_.push_back(item);
}

const std::shared_ptr<Item> &Bucket::item(int row) const
{   
    if (row < 0 || row >= items_.size()) {
        QMessageBox::critical(0, "Fatal Error", QString("Item row out of bounds: ") +
                              QString::number(row) + " item count: " + QString::number(items_.size()) +
                              ". Program will abort.");
        abort();
    }
    return items_[row];
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
