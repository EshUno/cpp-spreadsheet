#pragma once

#include "common.h"
#include "formula.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

class Impl{
public:
    virtual ~Impl() = default;
    virtual CellInterface::Value DoGetValue() const = 0;
    virtual std::string DoGetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
    virtual void ClearCache() = 0;
};

class  EmptyImpl final: public Impl{
public:
    CellInterface::Value DoGetValue() const override{
        return 0.0;
    }
    std::string DoGetText() const override{
        return "";
    }
    virtual std::vector<Position> GetReferencedCells() const override{
        return {};
    }
    void ClearCache() override {}
};

class TextImpl final: public Impl{
public:
    explicit TextImpl(std::string text): text_(std::move(text)){
    }
    CellInterface::Value DoGetValue() const override{
        return (!text_.empty() && text_.front() ==  ESCAPE_SIGN)?(text_.substr(1)):(text_);
    }
    std::string DoGetText() const override{
        return text_;
    }
    virtual std::vector<Position> GetReferencedCells() const override{
        return {};
    }
    void ClearCache() override {}
private:
    std::string text_;
};

class FormulaImpl final: public Impl{
public:
    explicit FormulaImpl(std::string text, SheetInterface& sheet): formula_(ParseFormula(std::move(text))), sheet_(sheet){
    }

    struct CallDoGetValue{
        CellInterface::Value operator()(const double value) { return value;}
        CellInterface::Value operator()(const FormulaError& err) { return err; }
    };

    CellInterface::Value DoGetValue() const override{
        if (IsCacheValid()) return cache_.value();
        auto eval = formula_->Evaluate(sheet_);
        if (auto* pval = std::get_if<double>(&eval)) {
            cache_ = *pval;
        }
        return std::visit(CallDoGetValue{}, eval);
    }

    std::string DoGetText() const override{
        return "=" + formula_->GetExpression();
    }

    bool IsCacheValid() const{
        return cache_.has_value();
    }

    void ClearCache() override {
        cache_.reset();
    }

    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }
private:
    std::unique_ptr<FormulaInterface> formula_{nullptr};
    SheetInterface& sheet_; //ссылка на лист таблицы, к которой принадлежит ячейка
    mutable std::optional<CellInterface::Value> cache_; // кэш ячейки
};


class Cell : public CellInterface {
public:
    explicit Cell(SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    [[nodiscard]] Value GetValue() const override;
    [[nodiscard]] std::string GetText() const override;
    [[nodiscard]] std::vector<Position> GetReferencedCells() const override;

    void ClearCache();

    void AddReverseReference(Cell* cell);
    void RemoveReverseReference(Cell* cell);

private:
    using Storage = std::unordered_set<Cell*>;

    std::unique_ptr<Impl> impl_;
    SheetInterface& sheet_; //ссылка на лист таблицы, к которой принадлежит ячейка
    mutable Storage reverse_order_; // список ячеек, которые на нее ссылаются (обратные зависимости)
};
