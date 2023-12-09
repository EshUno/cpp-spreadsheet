#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>

using namespace std::literals;

namespace
{

enum class VertexState
{
    Unknown,
    BeingExplored,
    Visited,
};

using DFSState = std::unordered_map<Position, VertexState>;

bool HasLoop(const Position& pos, const CellInterface& cell, const SheetInterface& sheet, DFSState& states)
{
    states[pos] = VertexState::BeingExplored;
    const auto& dependencies = cell.GetReferencedCells();

    for (const auto& dependency : dependencies)
    {
        const auto depState = states[dependency];

        if (depState == VertexState::Visited)
        {
            continue;
        }

        if (depState == VertexState::BeingExplored)
        {
            return true;
        }

        const auto* dependencyCell = sheet.GetCell(dependency);
        // Если клетки по позиции не существует, то и зависимостей у нее нет
        if (dependencyCell == nullptr)
        {
            continue;
        }

        if (HasLoop(dependency, *dependencyCell, sheet, states))
        {
            return true;
        }
    }

    states[pos] = VertexState::Visited;
    return false;
}

}

void Sheet::ResizeTable(Position pos){
    auto row = pos.row;
    auto col = pos.col;
    if (table_.size() <= static_cast<std::size_t>(row)){
        table_.resize(pos.row + 1);
    }
    if (table_.at(row).size() <= static_cast<std::size_t>(col)){
        table_.at(row).resize(col + 1);
    }
}

bool Sheet::IsOnTable(Position pos){
    if (table_.size() <= static_cast<std::size_t>(pos.row)){
        return false;
    }
    if (table_.at(pos.row).size() <= static_cast<std::size_t>(pos.col)){
        return false;
    }
    return true;
}

bool Sheet::CheckHasLoop(const Position &pos, const CellInterface &startingCell) const
{
    DFSState state;
    return HasLoop(pos, startingCell, *this, state);
}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()){
        throw InvalidPositionException("wrong position in SetCell");
    }

    {
        auto tmpCell = std::make_unique<Cell>(*this);
        tmpCell->Set(text);

        if (CheckHasLoop(pos, *tmpCell))
        {
            throw CircularDependencyException("Cell creates a cyclic dependency");
        }
    }

    // auto *cell = GetCell(pos);
    // if (cell == nullptr){
    //     ResizeTable(pos);
    //     table_.at(pos.row).at(pos.col) = std::make_unique<Cell>(*this);
    //     SetCell(pos, std::move(text));
    // }
    // else{
    //     dynamic_cast<Cell*>(cell)->Set(std::move(text));

    // }


    auto [cell, existed] = GetCellOrCreateEmpty(pos);
    if (existed)
    {
        cell->ClearCache();
        const auto& dependencies = cell->GetReferencedCells();
        for (const auto& depPos : dependencies)
        {
            auto* depCell = GetCell(depPos);
            dynamic_cast<Cell*>(depCell)->RemoveReverseReference(cell);
        }
    }
    cell->Set(std::move(text));

    const auto& newDependencies = cell->GetReferencedCells();
    for (const auto& dPos : newDependencies)
    {
        auto [c, _] = GetCellOrCreateEmpty(dPos);
        c->AddReverseReference(cell);
    }

    /*if (cell == nullptr){
        ResizeTable(pos);
        table_.at(pos.row).at(pos.col) = std::make_unique<Cell>(*this);
        // assert ReferencedCells.Empty()
        // assert reverse_order_.empty()
    }
    else{
        // cell.ClearCache()
        // for r in cell.ReferencedCells(): r->RemoveReverseRef(cell)
    }

    cell = GetCell(pos);
    dynamic_cast<Cell*>(cell)->Set(std::move(text));*/

    /*const auto& newDependencies = cell->GetReferencedCells();
    for (const auto& dPos : newDependencies)
    {
        const auto* c = GetCell(dPos);
        if (c == nullptr)
        {
            ResizeTable(dPos);
            table_.at(pos.row).at(pos.col) = std::make_unique<Cell>(*this);
            c = GetCell(dPos);
        }

        //c->AddReverseReference(c);
    }*/
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()){
        throw InvalidPositionException("wrong position in GetCell");
    }
    if (!IsOnTable(pos)) {
        return nullptr;
    }
    return table_.at(pos.row).at(pos.col).get();
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()){
        throw InvalidPositionException("wrong position in ClearCell");
    }
    if (IsOnTable(pos)) {
        // CacheInvalidate
        // remove from reverse_roders_ of referenced celss
        table_.at(pos.row).at(pos.col).reset();  
    }
}

Size Sheet::GetPrintableSize() const {
    Size res;

    for (int row = 0; static_cast<std::size_t>(row) < table_.size(); ++row){
        auto col_size = table_.at(row).size();
        for (int col = 0; static_cast<std::size_t>(col) < col_size; ++col){
            if (table_[row][col] != nullptr){
                res.cols = std::max(res.cols, col + 1);
                res.rows = std::max(res.rows, row + 1);
            }
        }
    }
    return res;
}

void Sheet::PrintValues(std::ostream& output) const {
    auto size = GetPrintableSize();

    for (auto row = 0; row < size.rows; ++row){
        for (auto col = 0; col < size.cols; ++col){
            if (col != 0) {
                output << '\t';
            }
            auto *cell = GetCell({row, col});
            if (cell != nullptr){
                std::visit([&output](const auto &elem) { output << elem ; }, cell->GetValue());
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    auto size = GetPrintableSize();

    for (auto row = 0; row < size.rows; ++row){
        for (auto col = 0; col < size.cols; ++col){
            if (col != 0) {
                output << '\t';
            }
            auto *cell = GetCell({row, col});
            if (cell != nullptr){
                output << cell->GetText();
            }
        }
        output << '\n';
    }
}

std::pair<Cell *, bool> Sheet::GetCellOrCreateEmpty(const Position &pos)
{
    bool existed = true;
    auto* cell = GetCell(pos);
    if (cell == nullptr)
    {
        ResizeTable(pos);
        auto newCell = std::make_unique<Cell>(*this);
        cell = newCell.get();
        table_.at(pos.row).at(pos.col) = std::move(newCell);
        existed = false;
    }

    return { dynamic_cast<Cell*>(cell), existed };
}


std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
