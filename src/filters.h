/*
    Copyright 2014 Ilya Zhuravlev

    This file is part of Acquisition.

    Acquisition is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Acquisition is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Acquisition.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <functional>
#include <memory>

#include "item.h"
#include "mainwindow.h"
#include "porting.h"
#include "ui_mainwindow.h"

class QLineEdit;
class QCheckBox;
class FilterData;

/*
 * Objects of subclasses of this class do the following:
 * 1) FromForm: provided with a FilterData fill it with data from form
 * 2) ToForm: provided with a FilterData fill form with data from it
 * 3) Matches: check if an item matches the filter provided with FilterData
 */
class Filter {
public:
    virtual void FromForm(FilterData *data) = 0;
    virtual void ToForm(FilterData *data) = 0;
    virtual void ResetForm() = 0;
    virtual bool Matches(const std::shared_ptr<Item> &item, FilterData *data) = 0;
    virtual ~Filter() {};
    std::unique_ptr<FilterData> CreateData();
};

struct ModFilterData {
    ModFilterData(const std::string &mod_, double min_, double max_, bool min_filled_, bool max_filled_) :
        mod(mod_),
        min(min_),
        max(max_),
        min_filled(min_filled_),
        max_filled(max_filled_)
    {}

    std::string mod;
    double min, max;
    bool min_filled, max_filled;
};

/*
 * This is used to store filter data in Search,
 * i.e. min-max values that the user has specified.
 */
class FilterData {
public:
    FilterData(Filter *filter);
    Filter *filter () { return filter_; }
    bool Matches(const std::shared_ptr<Item> item);
    void FromForm();
    void ToForm();
    // Various types of data for various filters
    // It's probably not a very elegant solution but it works.
    std::string text_query;
    double min, max;
    bool min_filled, max_filled;
    int r, g, b;
    bool r_filled, g_filled, b_filled;
    bool checked;
    std::vector<ModFilterData> mod_data;
private:
    Filter *filter_;
};

class NameSearchFilter : public Filter {
public:
    explicit NameSearchFilter(QLayout *parent);
    void FromForm(FilterData *data);
    void ToForm(FilterData *data);
    void ResetForm();
    bool Matches(const std::shared_ptr<Item> &item, FilterData *data);
    void Initialize(QLayout *parent);
private:
    QLineEdit *textbox_;
};

class MinMaxFilter : public Filter {
public:
    MinMaxFilter(QLayout *parent, std::string property);
    MinMaxFilter(QLayout *parent, std::string property, std::string caption);
    void FromForm(FilterData *data);
    void ToForm(FilterData *data);
    void ResetForm();
    bool Matches(const std::shared_ptr<Item> &item, FilterData *data);
    void Initialize(QLayout *parent);
protected:
    virtual double GetValue(const std::shared_ptr<Item> &item) = 0;
    virtual bool IsValuePresent(const std::shared_ptr<Item> &item) = 0;

    std::string property_, caption_;
private:
    QLineEdit *textbox_min_, *textbox_max_;
};

class SimplePropertyFilter : public MinMaxFilter {
public:
    SimplePropertyFilter(QLayout *parent, std::string property) :
        MinMaxFilter(parent, property) {}
    SimplePropertyFilter(QLayout *parent, std::string property, std::string caption) :
        MinMaxFilter(parent, property, caption) {}
protected:
    bool IsValuePresent(const std::shared_ptr<Item> &item);
    double GetValue(const std::shared_ptr<Item> &item);
};

// Just like SimplePropertyFilter but assumes given default value instead of excluding items
class DefaultPropertyFilter : public SimplePropertyFilter {
public:
    DefaultPropertyFilter(QLayout *parent, std::string property, double default_value) :
        SimplePropertyFilter(parent, property),
        default_value_(default_value)
    {}
    DefaultPropertyFilter(QLayout *parent, std::string property, std::string caption, double default_value) :
        SimplePropertyFilter(parent, property, caption),
        default_value_(default_value)
    {}
protected:
    bool IsValuePresent(const std::shared_ptr<Item> & /* item */) { return true; }
    double GetValue(const std::shared_ptr<Item> &item);
private:
    double default_value_;
};

class RequiredStatFilter : public MinMaxFilter {
public:
    RequiredStatFilter(QLayout *parent, std::string property) :
        MinMaxFilter(parent, property) {}
    RequiredStatFilter(QLayout *parent, std::string property, std::string caption) :
        MinMaxFilter(parent, property, caption) {}
private:
    bool IsValuePresent(const std::shared_ptr<Item> & /* item */) { return true; }
    double GetValue(const std::shared_ptr<Item> &item);
};

class ItemMethodFilter : public MinMaxFilter {
public:
    ItemMethodFilter(QLayout *parent, std::function<double(Item*)> func, std::string caption);
private:
    bool IsValuePresent(const std::shared_ptr<Item> & /* item */) { return true; }
    double GetValue(const std::shared_ptr<Item> &item);
    std::function<double(Item*)> func_;
};

class SocketsFilter : public MinMaxFilter {
public:
    SocketsFilter(QLayout *parent, std::string property) :
        MinMaxFilter(parent, property) {}
    SocketsFilter(QLayout *parent, std::string property, std::string caption) :
        MinMaxFilter(parent, property, caption) {}
    bool IsValuePresent(const std::shared_ptr<Item> & /* item */) { return true; }
    double GetValue(const std::shared_ptr<Item> &item);
};

class LinksFilter : public MinMaxFilter {
public:
    LinksFilter(QLayout *parent, std::string property) :
        MinMaxFilter(parent, property) {}
    LinksFilter(QLayout *parent, std::string property, std::string caption) :
        MinMaxFilter(parent, property, caption) {}
    bool IsValuePresent(const std::shared_ptr<Item> & /* item */) { return true; }
    double GetValue(const std::shared_ptr<Item> &item);
};

class SocketsColorsFilter : public Filter {
public:
    SocketsColorsFilter() {}
    explicit SocketsColorsFilter(QLayout *parent);
    void FromForm(FilterData *data);
    void ToForm(FilterData *data);
    void ResetForm();
    bool Matches(const std::shared_ptr<Item> &item, FilterData *data);
    void Initialize(QLayout *parent, const char* caption);
protected:
    bool Check(int need_r, int need_g, int need_b, int got_r, int got_g, int got_b, int got_w);
    QLineEdit *textbox_r_, *textbox_g_, *textbox_b_;
};

class LinksColorsFilter : public SocketsColorsFilter {
public:
    explicit LinksColorsFilter(QLayout *parent);
    bool Matches(const std::shared_ptr<Item> &item, FilterData *data);
};

class BooleanFilter : public Filter {
public:
    BooleanFilter(QLayout *parent, std::string property, std::string caption);
    void FromForm(FilterData *data);
    void ToForm(FilterData *data);
    void ResetForm();
    bool Matches(const std::shared_ptr<Item> &item, FilterData *data);
    void Initialize(QLayout *parent);
private:
    QCheckBox *checkbox_;
protected:
    std::string property_, caption_;
};

class MTXFilter : public BooleanFilter {
public:
    MTXFilter(QLayout *parent, std::string property, std::string caption):
        BooleanFilter(parent, property, caption) {}
    bool Matches(const std::shared_ptr<Item> &item, FilterData *data);
};

class AltartFilter : public BooleanFilter {
public:
    AltartFilter(QLayout *parent, std::string property, std::string caption):
        BooleanFilter(parent, property, caption) {}
    using BooleanFilter::BooleanFilter;
    bool Matches(const std::shared_ptr<Item> &item, FilterData *data);
};

class PricedFilter : public BooleanFilter {
public:
    PricedFilter(QLayout *parent, std::string property, std::string caption, const BuyoutManager &bm) :
        BooleanFilter(parent, property, caption),
        bm_(bm)
    {}
    bool Matches(const std::shared_ptr<Item> &item, FilterData *data);
private:
    const BuyoutManager &bm_;
};

class ItemlevelFilter : public MinMaxFilter {
public:
    ItemlevelFilter(QLayout *parent, std::string property) :
        MinMaxFilter(parent, property) {}
    ItemlevelFilter(QLayout *parent, std::string property, std::string caption) :
        MinMaxFilter(parent, property, caption) {}
    bool IsValuePresent(const std::shared_ptr<Item> & /* item */) { return true; }
    double GetValue(const std::shared_ptr<Item> &item);
};
