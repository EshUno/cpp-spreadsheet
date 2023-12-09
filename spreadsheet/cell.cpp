#include "cell.h"

// #include <cassert>
// #include <iostream>
// #include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>


Cell::Cell(SheetInterface &sheet): sheet_(sheet){
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::~Cell() = default;

void Cell::Set(std::string text)  {
    try {
        if (text.size() > 1 && text.front() ==  FORMULA_SIGN){
            impl_ = std::make_unique<FormulaImpl>(text.substr(1), sheet_);
        }
        else if (!text.empty()){
            impl_ = std::make_unique<TextImpl>(text);
        }
        else {
            impl_ = std::make_unique<EmptyImpl>();
        }
    } catch (...) {
        throw FormulaException("FormulaException in Set()");
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->DoGetValue();
}

std::string Cell::GetText() const {
    return impl_->DoGetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

void Cell::ClearCache()
{
    impl_->ClearCache();

    for (const auto d : reverse_order_)
    {
        d->ClearCache();
    }
}

void Cell::AddReverseReference(Cell *cell)
{
    reverse_order_.insert(cell);
}

void Cell::RemoveReverseReference(Cell *cell)
{
    reverse_order_.erase(cell);
}
