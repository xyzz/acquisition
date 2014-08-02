#include "bucket.h"

// this is required by std::map's operator[]
Bucket::Bucket()
{}

Bucket::Bucket(const std::string &name):
    name_(name)
{}

void Bucket::AddItem(const std::shared_ptr<Item> item) {
    items_.push_back(item);
}
