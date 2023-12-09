#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <string>
#include <algorithm>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    //return output << fe.ToString();
    return output << "#ARITHM!";
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы
    explicit Formula(const std::string expression)
: ast_(ParseFormulaAST(expression)), referenced_cells_(ast_.GetCells().begin(), ast_.GetCells().end()) {
        auto last = std::unique(referenced_cells_.begin(), referenced_cells_.end());
        referenced_cells_.erase(last, referenced_cells_.end());
    }

    struct CallGetValue{
        double operator()(const double value) { return value;}
        double operator()(const std::string& text){
            if (all_of(text.begin(),text.end(), ::isdigit)) return std::stod(text);
            throw FormulaError(FormulaError::Category::Value);
        }
        double operator()(const FormulaError& err) { throw err; }
    };

    [[nodiscard]] Value Evaluate(const SheetInterface& sheet) const override{
        try {
            auto get_sheet = [&sheet](const Position& pos) {
                auto *cell = sheet.GetCell(pos);
                if (cell == nullptr){
                    return 0.0;
                }
                return std::visit(CallGetValue{}, cell->GetValue());
            };

            return ast_.Execute(get_sheet);
        } catch (FormulaError &fe) {
            return fe;
        }
    }

    [[nodiscard]] std::string GetExpression() const override{
        std::ostringstream sst;
        ast_.PrintFormula(sst);
        return sst.str();
    }

    [[nodiscard]] virtual std::vector<Position> GetReferencedCells() const override{
        return referenced_cells_;
    }

private:
    FormulaAST ast_;
    std::vector<Position> referenced_cells_;

};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (std::exception& exc) {
        throw FormulaException(exc.what());
    }
}
