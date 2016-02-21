/*
    Copyright 2016 Baptiste Wicht

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

#include <sstream>
#include <iostream>

#include "auto_price.hpp"

namespace {

bool ends_with(const std::string& str, const std::string& end) {
    if (str.length() >= end.length()) {
        return str.compare (str.length() - end.length(), end.length(), end) == 0;
    } else {
        return false;
    }
}

bool starts_with(const std::string& str, const std::string& start){
    if (str.length() >= start.length()) {
        return str.compare (0, start.length(), start) == 0;
    } else {
        return false;
    }
}

bool is_double(const std::string& s){
    std::istringstream iss(s);
    double d;
    char c;
    return iss >> d && !(iss >> c);
}

double to_double(const std::string& s){
    std::istringstream iss(s);
    double d;
    iss >> d;
    return d;
}

bool is_int(const std::string& s){
    std::istringstream iss(s);
    int d;
    char c;
    return iss >> d && !(iss >> c);
}

int to_int(const std::string& s){
    std::istringstream iss(s);
    int d;
    iss >> d;
    return d;
}

} //end of anonymous namespace

bool is_auto_priced(const std::string& title, bool recipe){
    if(ends_with(title, "c")){
        return is_double({title.begin(), title.begin() + title.size() - 1});
    } else if(ends_with(title, "f")){
        return is_double({title.begin(), title.begin() + title.size() - 1});
    } else if(ends_with(title, "alt")){
        return is_double({title.begin(), title.begin() + title.size() - 3});
    } else if(ends_with(title, "chi")){
        return is_double({title.begin(), title.begin() + title.size() - 3});
    } else if(ends_with(title, "alc")){
        return is_double({title.begin(), title.begin() + title.size() - 3});
    } else if(ends_with(title, "ex")){
        return is_double({title.begin(), title.begin() + title.size() - 2});
    }

    // Automatically price recipes
    if(recipe){
        // Chaos and Regals Recipes
        if(starts_with(title, "C")){
            return is_int({title.begin() + 1, title.end()});
        } else if(starts_with(title, "R")){
            return is_int({title.begin() + 1, title.end()});
        }

        // Flask Crafting
        else if(starts_with(title, "F_C")){
            return title.size() == 3 || is_int({title.begin() + 3, title.end()});
        } else if(starts_with(title, "F_R")){
            return title.size() == 3 || is_int({title.begin() + 3, title.end()});
        }
    }

    return false;
}

Buyout get_auto_price(const std::string& title, bool recipe){
    if(ends_with(title, "c")){
        return {to_double({title.begin(), title.begin() + title.size() - 1}), BUYOUT_TYPE_BUYOUT, CURRENCY_CHAOS_ORB, {}};
    } else if(ends_with(title, "f")){
        return {to_double({title.begin(), title.begin() + title.size() - 1}), BUYOUT_TYPE_BUYOUT, CURRENCY_ORB_OF_FUSING, {}};
    } else if(ends_with(title, "alt")){
        return {to_double({title.begin(), title.begin() + title.size() - 3}), BUYOUT_TYPE_BUYOUT, CURRENCY_ORB_OF_ALTERATION, {}};
    } else if(ends_with(title, "chi")){
        return {to_double({title.begin(), title.begin() + title.size() - 3}), BUYOUT_TYPE_BUYOUT, CURRENCY_CARTOGRAPHERS_CHISEL, {}};
    } else if(ends_with(title, "alc")){
        return {to_double({title.begin(), title.begin() + title.size() - 3}), BUYOUT_TYPE_BUYOUT, CURRENCY_ORB_OF_ALCHEMY, {}};
    } else if(ends_with(title, "ex")){
        return {to_double({title.begin(), title.begin() + title.size() - 2}), BUYOUT_TYPE_BUYOUT, CURRENCY_EXALTED_ORB, {}};
    }

    // Automatically price recipes
    if(recipe){
        // Chaos and Regals Recipes
        if(starts_with(title, "C")){
            return {1.0, BUYOUT_TYPE_BUYOUT, CURRENCY_CHAOS_ORB, {}};
        } else if(starts_with(title, "R")){
            return {1.0, BUYOUT_TYPE_BUYOUT, CURRENCY_CHAOS_ORB, {}};
        }

        // Flask Crafting
        else if(starts_with(title, "F_C")){
            return {1.0, BUYOUT_TYPE_BUYOUT, CURRENCY_CHAOS_ORB, {}};
        } else if(starts_with(title, "F_R")){
            return {1.0, BUYOUT_TYPE_BUYOUT, CURRENCY_CHAOS_ORB, {}};
        }
    }

    std::cerr << "get_auto_price should never be called on non-auto priced tabs" << std::endl;

    return {};
}
