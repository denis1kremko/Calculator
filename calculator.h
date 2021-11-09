#include <iostream>
#include <sstream>
#include <string>
#include <memory>

class Tokenizer {
public:
    Tokenizer(std::istream *in) : in_(in) {
    }

    enum TokenType {
        kUnknown,
        kNumber,
        kSymbol,
        kEnd,
    };

    void Consume() {
        while (in_->peek() == ' ') {
            in_->ignore(1);
        }

        if (in_->peek() == '+' || in_->peek() == '-' || in_->peek() == '*' || in_->peek() == '/' ||
            in_->peek() == '(') {

            type_ = TokenType::kSymbol;
            symbol_ = in_->get();

        } else if (in_->peek() >= '0' && in_->peek() <= '9') {
            type_ = TokenType::kNumber;
            *in_ >> number_;

        } else if (in_->eof() || in_->get() == ')') {
            symbol_ = ')';
            type_ = TokenType::kEnd;
        }
    }

    TokenType GetType() const {
        return type_;
    }

    int64_t GetNumber() const {
        return number_;
    }

    char GetSymbol() const {
        return symbol_;
    }

private:
    std::istream *in_;

    TokenType type_ = TokenType::kUnknown;
    int64_t number_;
    char symbol_;
};

class Expression {
public:
    virtual ~Expression() = default;

    virtual int64_t Evaluate() = 0;
};

class Number : public Expression {
public:
    explicit Number(int64_t number) : number_(number) {
    }

    int64_t Evaluate() override {
        return number_;
    }

private:
    int64_t number_;
};

class Operation : public Expression {
public:
    explicit Operation() : operation_(' '), lhs_(nullptr), rhs_(nullptr) {
    }

    void SetOperation(char operation) {
        operation_ = operation;
    }

    void SetLHS(std::unique_ptr<Expression> lhs) {
        lhs_ = std::unique_ptr<Expression>(std::move(lhs));
    }

    void SetRHS(std::unique_ptr<Expression> rhs) {
        rhs_ = std::unique_ptr<Expression>(std::move(rhs));
    }

    int64_t Evaluate() override {
        if (rhs_ == nullptr) {
            return lhs_->Evaluate();
        }

        if (operation_ == '+') {
            return lhs_->Evaluate() + rhs_->Evaluate();
        } else if (operation_ == '-') {
            return lhs_->Evaluate() - rhs_->Evaluate();
        } else if (operation_ == '*') {
            return lhs_->Evaluate() * rhs_->Evaluate();
        } else {
            return lhs_->Evaluate() / rhs_->Evaluate();
        }
    }

private:
    char operation_;
    std::unique_ptr<Expression> lhs_, rhs_;
};

std::unique_ptr<Expression> ParseExpression(Tokenizer *tok);

std::unique_ptr<Expression> ParseUnit(Tokenizer *tok) {
    // поглощаем число или скобку
    tok->Consume();
    // левая часть выражения
    auto unit = std::make_unique<Operation>(Operation());

    int sign = 1;
    if (tok->GetType() == tok->kSymbol && tok->GetSymbol() == '-') {
        sign = -1;
        tok->Consume();
    }

    // выражение либо число, либо что-то в скобках
    if (tok->GetType() == tok->kSymbol && tok->GetSymbol() == '(') {
        // что-то в скобках
        unit->SetLHS(std::make_unique<Number>(sign));
        unit->SetOperation('*');
        unit->SetRHS(ParseExpression(tok));
    } else {
        // число
        unit->SetLHS(std::make_unique<Number>(sign));
        unit->SetOperation('*');
        unit->SetRHS(std::make_unique<Number>(tok->GetNumber()));
    }

    tok->Consume();
    return unit;
}

std::unique_ptr<Expression> ParseTerm(Tokenizer *tok) {
    auto term_expr = std::make_unique<Operation>(Operation());
    term_expr->SetLHS(ParseUnit(tok));

    auto temp_expr = std::make_unique<Operation>(Operation());
    while (tok->GetType() != tok->kEnd && tok->GetSymbol() != '+' && tok->GetSymbol() != '-') {
        term_expr->SetOperation(tok->GetSymbol());
        term_expr->SetRHS(ParseUnit(tok));

        temp_expr = std::move(term_expr);

        term_expr = std::make_unique<Operation>(Operation());
        term_expr->SetLHS(std::move(temp_expr));
    }

    return term_expr;
}

std::unique_ptr<Expression> ParseExpression(Tokenizer *tok) {
    auto main_expr = std::make_unique<Operation>(Operation());
    main_expr->SetLHS(ParseTerm(tok));

    auto temp_expr = std::make_unique<Operation>(Operation());
    while (tok->GetType() != tok->kEnd) {
        main_expr->SetOperation(tok->GetSymbol());
        main_expr->SetRHS(ParseTerm(tok));

        temp_expr = std::move(main_expr);

        main_expr = std::make_unique<Operation>(Operation());
        main_expr->SetLHS(std::move(temp_expr));
    }

    return main_expr;
}