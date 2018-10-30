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
    if (row >= 0) {
        std::vector<Items>::size_type row_t = (size_t) row;  // Assumes int max() always able to fit in unsigned long long
        if (row_t < items_.size()) {
            return items_[row_t];
        }
    }

    QMessageBox::critical(nullptr, "Fatal Error", QString("Item row out of bounds: ") +
                          QString::number(row) + " item count: " + QString::number(items_.size()) +
                          ". Program will abort.");
    abort();

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
