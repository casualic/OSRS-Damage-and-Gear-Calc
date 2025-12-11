#pragma once
#include <iostream>
#include <string>
#include <map>

class Item {
    private:
        std::string name_;
        std::map<std::string, int> stats_;
        int price_;
    public:
        Item(std::string n);
        int fetchPrice();
        int getPrice() const { return price_; }
};
