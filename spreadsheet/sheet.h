#pragma once

#include "cell.h"
#include "common.h"
#include <functional>

#include <memory>
#include <ostream>
#include <string>
#include <vector>


class Sheet : public SheetInterface {
public:
    ~Sheet() = default;

    void SetCell(Position pos, std::string text) override;

    [[nodiscard]] const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    [[nodiscard]] Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    using Table = std::vector<std::vector<std::unique_ptr<CellInterface>>>;
private:
    // Возвращает существующую ячейку либо создает пустую, изменяя размер печатной области
    // Return:
    // Cell* - указатель на ячейку
    // bool - true если ячейка существовала
    std::pair<Cell*, bool> GetCellOrCreateEmpty(const Position& pos);
    void ResizeTable(Position pos);
    bool IsOnTable(Position pos);
    bool CheckHasLoop(const Position& pos, const CellInterface& startingCell) const;

    Table table_;
};
